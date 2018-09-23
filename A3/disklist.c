#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/mman.h>

#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry

/* A sample code on how to display creation time and date 
   authored by Huan Wang
 */
void print_date_time(char * directory_entry_startPos){
	
	int time, date;
	int hours, minutes, day, month, year;
	
	time = *(unsigned short *)(directory_entry_startPos + timeOffset);
	date = *(unsigned short *)(directory_entry_startPos + dateOffset);
	
	//the year is stored as a value since 1980
	//the year is stored in the high seven bits
	year = ((date & 0xFE00) >> 9) + 1980;
	//the month is stored in the middle four bits
	month = (date & 0x1E0) >> 5;
	//the day is stored in the low five bits
	day = (date & 0x1F);
	
	printf("%d-%02d-%02d ", year, month, day);
	//the hours are stored in the high five bits
	hours = (time & 0xF800) >> 11;
	//the minutes are stored in the middle 6 bits
	minutes = (time & 0x7E0) >> 5;
	
	printf("%02d:%02d\n", hours, minutes);
	
	return ;	
}

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

/*Prints out F/D, file size, filename, and creation date and time for root directory files.*/
void print_files(char *data){
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
    	if((attribute & 0x10) == 0x10){
    		file_type = 'D';
    	} else{
    		file_type = 'F';
    	}

    	int file_size = get_file_size(data, address);

		printf("%c %10d %20s ", file_type, file_size, filename);

		print_date_time(&data[address]);

   		address += 32;
    }
    return;
}

int main(int argc, char *argv[]){
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

	print_files(data);

    munmap(data, file_stat.st_size);
  }