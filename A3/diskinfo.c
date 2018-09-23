#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

/*Opens the input disk image and memory maps it.*/
char* open_disk_image(struct stat file_stat, int file, char *in_file){
	char *data;
	file = open(in_file, O_RDWR); 
	data = mmap(NULL, file_stat.st_size, PROT_READ, MAP_SHARED, file, 0);

	return data;
}

/*Reads the operating system name from boot sector*/
char* get_os(char *data, char *os_name){
	int i;
	for(i = 0; i < 8; i++){
		os_name[i] = data[i+3];
	}
	os_name[8] = '\0';

	return os_name;
}

/*Reads the disk label from root directory.*/
char* get_label_root(char *data, char *label){
    int address = 0x2600;
    while(address < 33*512){
    	int attribute = data[address + 11];
    	if((attribute) == 0x08){
    		int i;
    		for(i = 0; i < 11; i++){
    			label[i] = data[address + i];
    		}
    		break;
    	}
    	address += 32;
    }
    label[11] = '\0';
    return label;
}

/*Reads the disk label from boot sector and then from root directory if not found in boot.*/
char* get_label(char *data, char *label){
	int i;
	for(i = 0; i < 11; i++){
		label[i] = data[i+43];
	}
	label[11] = '\0';

	if(label[0] == ' '){
		get_label_root(data, label);
	}

	return label;
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

/*Counts the number of files in the root directory not including subdirectories.*/
int get_num_files(char *data){
	int num_files = 0;
    //Go thorugh each 32 byte block and read the file name. If its valid then increase num_files.
    int address = 0x2600;
    while(address < 33*512){
    	char filename[9];
    	filename[8] = '\0';
    	int j;
    	for(j = 0; j < 8; j++){
    		filename[j] = data[address + j];
    	}
    	int attribute = data[address + 11];
    	if(attribute == 0x0F || filename[0] == 0x00 || filename[0] == 0xE5 || (attribute & 0x08) == 0x08){
    		address += 32;
    		continue;
    	}
    	num_files++;
    	address += 32;
    }
    return num_files;
}

int main(int argc, char *argv[]) {
	if(argv[1] == NULL){
        printf("Please input a disk image,\n");
        exit(1);
    }

	struct stat file_stat;

	if (stat(argv[1], &file_stat) == -1) {
        perror("stat");
        exit(1);
    }

	int file;
	char *data = open_disk_image(file_stat, file, argv[1]);

	char os_name[9];
	get_os(data, os_name);
	
	int total_size = file_stat.st_size;

	char label[12];
	get_label(data, label);

	char num_fats = data[16];
    
    int sectors = data[22] + (data[23] << 8);

    int free_size = get_free_size(data);

    int num_files = get_num_files(data);

	printf("OS Name: %s\n", os_name);
	printf("Label of the disk: %s\n", label);
	printf("Total size of the disk: %d bytes\n", total_size);
	printf("Free size of the disk: %d bytes\n", free_size);
	printf("==============\n");
	printf("The number of files in the root directory (not including subdirectories): %d\n\n", num_files);
	printf("=============\n");
	printf("Number of FAT copies: %d\n", num_fats);
	printf("Sectors per FAT: %d\n", sectors);

	munmap(data, file_stat.st_size);

	return 0;
}