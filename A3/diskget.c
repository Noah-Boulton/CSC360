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

/*Reads the file size from the root directory entry.*/
int get_file_size(char *data, int address){
	//Get file size
    int b0 = ((data[address+28] & 0XF0 >> 4) + (data[address+28] & 0x0F << 4));
   	int b1 = ((data[address+28+1] & 0XF0 >> 4) + (data[address+28+1] & 0x0F << 4)) << 8; 
    int b2 = ((data[address+28+2] & 0XF0 >> 4) + (data[address+28+2] & 0x0F << 4)) << 16;
    int b3 = (data[address+28+3] & 0XF0 >> 4) + (data[address+28+3] & 0x0F << 4) << 24;

    return b0 + b1 + b2 + b3;
}

/*Writes the contents of the input file to the disk image and updates the FAT entries.*/
void write_to_file(int cluster, char *data, char *in_file, int file_size){
	int next_cluster = get_value(cluster, data);
	FILE *out_file;
   	out_file = fopen(in_file, "wb");
	while(0xfff != next_cluster){
		//read 512 bytes from the cluster
		int address = (cluster + 31) * 512;
		int i;
		for(i = 0; i < 512; i++){
			fputc(data[address + i], out_file);
		}
		cluster = next_cluster;
		next_cluster = get_value(cluster, data);
	}

	//read the remaining bytes from last cluster
	int remaining_bytes = file_size - (file_size/512)*512;
	int address = (cluster + 31) * 512;
	int i;
	for(i = 0; i < remaining_bytes; i++){
		fputc(data[address + i], out_file);
	}
	fclose(out_file);
	return;
}

/*Gets the value in FAT1 for the given index.*/
int get_value(int index, char *data){
   	int value;
   	if(index % 2 == 0){
   		//even index low four bits in 1+3n/2 and 8 bits in 3n/2
    	int low = data[512 + 1+(3*index/2)] & 0x0F; //mask out the high bits 
    	int other = data[512 + (3*index/2)] & 0xFF; //needed to mask out other bits as I had strange output of 0xffffff81 instead of 0x81
    	value = (low << 8) + other; 
    } else{
    	//high four bits in 3n/2 and 8 bits in 1+3n/2
    	int high = data[512 + (3*index/2)] & 0xF0;
    	int other = data[512 + 1+(3*index/2)] & 0xFF;
    	value = (high >> 4) + (other << 4);
    }
    return value;
}

/*Searches the root directory for the given file.*/
int find_file(char *data, char *in_file){
	int offset = 0x2600;
    int address = offset;
	while(address < 33*512){
    	char filename[12];
    	filename[11] = '\0';
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

int main(int argc, char *argv[]) {
	if(argv[1] == NULL){
        printf("Please input a disk image,\n");
        exit(1);
    }

	//Calculate the size in the last cluster to know how much to read from it.
	struct stat file_stat;

	if (stat(argv[1], &file_stat) == -1) {
        perror("stat");
        exit(1);
    }

	int file;
	char *data = open_disk_image(file_stat, file, argv[1]);

	if(argv[2] == NULL){
		printf("Please input a file to retreive.\n");
		exit(1);
	}

	//convert filename to uppercase
	char *in_file = argv[2];
	int i = 0;
	while(in_file[i]) {
      in_file[i] = toupper(in_file[i]);
      i++;
   	}
   
   	int found = 0;
   	found = find_file(data, in_file);
    if(found == 0){
    	printf("File not found.\n");
    }else {
    	//get the first cluster then rely on FAT to find the rest
    	int first_logical_cluster = data[found + 26] + (data[found + 27] << 8) & 0xFF;
    	int file_size = get_file_size(data, found);
    	write_to_file(first_logical_cluster, data, in_file, file_size);
    }

    munmap(data, file_stat.st_size);
}