#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

void put_value(int index, int next_cluster, char *data);
int find_file(char *data, char *in_file);

/*Opens the input disk image and memory maps it.*/
char* open_disk_image(struct stat file_stat, int file, char *in_file){
    char *data;
    
    file = open(in_file, O_RDWR); 
    data = mmap(NULL, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

    return data;
}

/*Calculates the free size of the disk.*/
int get_free_size(char *data){
    int free_size = 0;
    int index;

    int total_sector_count = data[19] + (data[20] << 8);

    for(index = 2; index < total_sector_count-31; index++){
        int value;
        if(index%2 == 0){
            //even index low four bits in 1+3n/2 and 8 bits in 3n/2
            int low = data[512 + 1+(3*index/2)] & 0x0F;
            int other = data[512 + (3*index/2)] & 0xFF;
            value = (low << 8) + other;
        } else{
            //high four bits in 3n/2 and 8 bits in 1+3n/2
            int high = data[512 + (3*index/2)] & 0xF0;
            int other = data[512 + 1+(3*index/2)] & 0xFF;
            value = (high >> 4) + (other << 4);
        }
        if(value == 0x00){
            free_size++;
        }
    }
    free_size = free_size * 512;
    return free_size;
}

/*Writes the contents of the file to the disk image and updates the FAT entries.*/
void write_from_file(int cluster, char *data, char *in_data, int file_size){
    int in_address = 0;
    while(file_size > 512){
        //Write 512 bytes to current cluster
        int address = (cluster + 31) * 512;
        int i;
        for(i = 0; i < 512; i++){
            data[address + i] = in_data[in_address];
            in_address++;

        }
        //find next free cluster
        int total_sector_count = data[19] + (data[20] << 8);
        int next_cluster = -1;
        for(i = 2; i < total_sector_count-31; i++){
            int next_cluster_value = get_value(i, data);
            if(next_cluster_value == 0x00 && i != cluster){
                next_cluster = i;
                break;
            }
        }
        //update the FAT entry at the old cluster
        put_value(cluster, next_cluster, data);
        cluster = next_cluster;
        //sub 512 from filesize
        file_size -= 512;
    }
    //Write remaining bytes to file
    int address = (cluster + 31) * 512;
    int i;
    for(i = 0; i < file_size; i++){
        data[address + i] = in_data[in_address];
        in_address++;

    }
    //Set the last FAT entry
    put_value(cluster, 0xFFF, data);
    return;
}

/*Gets the value in FAT1 for the given index.*/
int get_value(int index, char *data){
    int value;
    if(index % 2 == 0){
        //even index low four bits in 1+3n/2 and 8 bits in 3n/2
        int low = data[512 + 1+(3*index/2)] & 0x0F; //mask out the high bits 
        int other = data[512 + (3*index/2)] & 0xFF; //needed to mask out other bits as I had strange output of 0xffffff81 instead of 0x81
        //printf("Even: low %x other %x\n", low, other);
        value = (low << 8) + other; //push the 8 bits up and add the 
    } else{
        //high four bits in 3n/2 and 8 bits in 1+3n/2
        int high = data[512 + (3*index/2)] & 0xF0;
        int other = data[512 + 1+(3*index/2)] & 0xFF;
        //printf("Odd: high %x other %x\n", high, other);
        value = (high >> 4) + (other << 4);
    }
    return value;
}

/*Sets the value in FAT1 for the given index.*/
void put_value(int index, int next_cluster, char *data){
    next_cluster = next_cluster & 0xFFF;
    if(index % 2 == 0){
        //Even
        int b0 = (next_cluster >> 8) & 0x0F;
        int b1 = (next_cluster & 0xFF);
        data[512 + (3*index/2)] = b1;
        data[512 + 1+(3*index/2)] = (data[512 + 1+(3*index/2)] & 0xFF) + (b0 & 0x0F);
    } else{
        //Odd
        int b0 = (next_cluster >> 4) & 0xFF;
        int b1 = (next_cluster << 4) & 0xF0;
        data[512 + (3*index/2)] = data[512 + (3*index/2)] + b1;
        data[512 + 1+(3*index/2)] = b0;
    }
    return;
}

/*Creates a root directory entry for the new file.*/
int create_directory_entry(char *data, char *file){
    //convert filename to uppercase
    char in_file[100];
    strcpy(in_file, file);
    int i = 0;
    while(in_file[i]) {
        in_file[i] = toupper(in_file[i]);
        i++;
    }

    //Check if the file is already in the root
    if(find_file(data, in_file)){
        printf("File already exists in root.\n");
        exit(1);
    }

    int offset = 0x2600;
    int address = offset;
    while(address < 33*512){
        if(data[address] == 0x00 || data[address] == 0xE5){
            //this is a free entry   
            int i;
            int extension = -1;
            for(i = 0; i < 8; i++){
                if(in_file[i] == '.'){
                    extension = i;
                    data[address + i] = ' ';
                    continue;
                } else if(extension != -1){
                    data[address + i] = ' ';
                } else{
                    data[address + i] = in_file[i];
                }
            }  
            if(extension == -1){ //need to find where the extension starts
                for(i = 0; i < 100; i++){
                    if(in_file[i] == '.'){
                        extension = i;
                    }
                }
            }
            int no_ext = 0;
            if(extension == -1){ // no extension
                no_ext = 1;
            }
            extension++;
            for(i = 0; i < 3; i++){
                data[address + 8 + i] = in_file[extension + i];
            }  
            break;
        }
        address += 32;
    }
    if(address >= 33*512){
        return -1;
    }else {
        return address;
    }
}

/*Sets the file size in the root directory entry.*/
void set_file_size(char *data, int address, int size){
    data[address + 28] = size & 0xFF;
    data[address + 28 + 1] = (size >> 8) & 0xFF;
    data[address + 28 + 2] = (size >> 16) & 0xFF; 
    data[address + 28 + 3] = (size >> 24) & 0xFF;
    return;
}

/*Searches the root directory for the given file.*/
int find_file(char *data, char *in_file){
    //search root directory for filename matching the string
    int offset = 0x2600;
    int address = offset;
    while(address < 33*512){
        char filename[12];
        //char extension[4];
        filename[11] = '\0';
        //extension[4] = '\0';
        char file_type;
        int j;
        int k = 0;
        for(j = 0; j < 8; j++){
            if(data[address + j] == ' '){
                j++;
                continue;
            }
            filename[j] = data[address + j];
            k++;
        }
        filename[k] = '.';
        k = 8-k;
        
        for(j = 8; j < 12; j++){
            filename[j+1-k] = data[address + j];
        }
        int attribute = data[address + 11];
        if(attribute == 0x0F || filename[0] == 0x00 || filename[0] == 0xE5 || (attribute & 0x08) == 0x08){
            address += 32;
            continue;
        }

        if(strcmp(filename, in_file) == 0){ //Found the file
            return address;
        }

        address += 32;
    }
    return 0;
}

/*Copies the contents of FAT1 to FAT2 after writing the new file.*/
void copy_fat(char *data){
    int total_sector_count = data[19] + (data[20] << 8);
    int i;
    int j;
    for(j = 1; j < 10; j++){
        for(i = 0; i < 512; i++){
            data[(j+9)*512 + i] = data[j*512 + i];
        }
    }
    return;
}

int main(int argc, char *argv[]) {
    if(argv[1] == NULL){
        printf("Please input a disk image,\n");
        exit(1);
    }

    if(argv[2] == NULL){
        printf("Please input a file.\n");
        exit(1);
    }

    struct stat file_stat;

    if (stat(argv[1], &file_stat) == -1) {
        perror("stat");
        exit(1);
    }

    int file;
    char *data = open_disk_image(file_stat, file, argv[1]);

    struct stat input_file_stat;

    if (stat(argv[2], &input_file_stat) == -1) {
        perror("stat");
        exit(1);
    }

    //check if there is enough space 
    if(input_file_stat.st_size > get_free_size(data)){
        printf("Error: Not enough free space on disk.\n");
        exit(1);
    }

    int input_file;
    char *in_data = open_disk_image(input_file_stat, input_file, argv[2]);

    //Create directory listing in root and add the filename
    int address = create_directory_entry(data, argv[2]);
    if(address == -1){
        printf("No free entries in root.\n");
        exit(1);
    }

    //Update the filesize in directory entry 
    set_file_size(data, address, input_file_stat.st_size);

    //Find first free cluster in FAT and update root
    int total_sector_count = data[19] + (data[20] << 8);
    int first_cluster = -1;
    int i;
    for(i = 0; i < total_sector_count-31; i++){
        int first_cluster_value = get_value(i, data);
        if(first_cluster_value == 0x00){
            first_cluster = i;
            break;
        }
    }

    //Update the root entry with first cluster
    data[address + 26] = (first_cluster) & 0xFF;
    data[address + 27] = (first_cluster >> 8) & 0xFF;

    //Copy the file into the image
    write_from_file(first_cluster, data, in_data, input_file_stat.st_size);

    //Copy file updating the FAT table
    copy_fat(data);

    //Make sure to update the second FAT table by copying the contents of table 1 to table 2
    munmap(in_data, input_file_stat.st_size);
    munmap(data, file_stat.st_size);
}