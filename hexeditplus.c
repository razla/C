#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

char* globalBuffer;
char* src_file;
unsigned char* output;
int size=0;
int location;
int length;
int val;

void setFileName() {
    printf("Enter a filename:\n");
    scanf("%s", globalBuffer);
}

void setUnitSize() {
	int tempSize;
    printf("Enter a unit size:\n");
    scanf("%d", &tempSize);
    if (tempSize!=1 && tempSize!=2 && tempSize!=4){
        printf("Error: given size should be 1/2/4\n");
    }
    else {
        size=tempSize;
    }
}

void fileDisplay() {
	if (globalBuffer==NULL){
    	printf("Error: filename is null\n");
    }
    else {
    	int fileD = open(globalBuffer, O_RDONLY);
    	if (fileD==-1){
    		printf("Error: name is not valid\n");
    		return;
    	}
    	else {
			printf("Please enter <location> <length>\n");
			scanf("%x%d", &location, &length);
			output = malloc (size * length);
			if(lseek (fileD, location, SEEK_SET)==-1){
				return;
			}
			if(read (fileD, output, size*length)==-1){
				return;
			}
			close (fileD);
			printf("Hexadecimal Representation:\n");
			for (int i=0; i < length; i++){
				if (size==1){
					printf("%02x ",((char*)output)[i]);
				}
				if (size==2){
					printf("%04x ",((short*)output)[i]);
				}
				if (size==4){
					printf("%08x ",((int*)output)[i]);
				}
			}
			printf("\n");
			printf("Decimal Representation:\n");
			for (int i=0; i < length; i++){
				if (size==1){
					printf("%d ",((char*)output)[i]);
				}
				if (size==2){
					printf("%d ",((short*)output)[i]);
				}
				if (size==4){
					printf("%d ",((int*)output)[i]);
				}
			}
   			printf("\n");
   			free (output);
		}
	}
}

void fileModify() {
	if (size==0){
		printf("Error: size hasn't been initialized\n");
    		return;
	}
	else {
		printf("Please enter <location> <val>\n");
	    scanf("%x%x", &location, &val);
	    if (globalBuffer==NULL){
	    	printf("Error: filename is null\n");
	    	return;
	    }
	    else {
	    	int fileD = open(globalBuffer, O_WRONLY);
	    	if (fileD==-1) {
	    		printf("Error: name is not valid\n");
	    		return;
	    	}
	    	else {
				if(lseek (fileD, location, SEEK_SET)==-1){
					printf("Error: location is not valid\n");
					return;
				}
				write (fileD, &val, size);
				close (fileD);
			}
		}
	}
}

void copyFromFile() {
	int src_offset, dst_offset;
	src_file = malloc(100);
	printf("Please enter <src_file> <src_offset> <dst_offset> <length>\n");
	scanf("%s %x %x %d", src_file, &src_offset, &dst_offset, &length);
	if (globalBuffer==NULL){
		printf("Error: filename is null\n");
	}
	else {
	    int fileD = open(globalBuffer, O_RDWR);
	    if (fileD==-1) {
	    	printf("Error: destination file is not valid\n");
	    	return;
	    }
	    else {
	    	int fileS = open(src_file, O_RDONLY);
	    	if (fileS==-1){
	    		printf("Error: source file is not valid\n");
	    		return;
	    	}
	    	else {
	    		output = malloc (size * length);
	    		if (lseek (fileS, src_offset, SEEK_SET)==-1){
	    			printf("Invalid lseek\n");
	    			return;
	    		}
	    		if (lseek(fileD, dst_offset, SEEK_SET)==-1){
	    			printf("Invalid lseek\n");
	    			return;
	    		}
	    		if (read (fileS, output, length)==-1){
	    			printf("Invalid read\n");
	    			return;
	    		}
	    		if (write (fileD, output, length)==-1){
	    			printf("Invalid write\n");
	    			return;
	    		}
	    		close(fileS);
	    		close(fileD);
	    		free(src_file);
	    		printf("Loaded %d bytes from %s to %s\n", length, src_file, globalBuffer);
	    		free(output);
	    	}
			
		}
	}
}

void quit() {
	free (globalBuffer);
    exit (0);
}


int main(int argc, char **argv) {
    globalBuffer = malloc (sizeof(char) * 100);
    void(*options[6]) ();
    options[0]=setFileName;
    options[1]=setUnitSize;
    options[2]=fileDisplay;
    options[3]=fileModify;
    options[4]=copyFromFile;
    options[5]=quit;
    int choose;
    while(1){
        printf("%s", "Choose action:\n");
        printf("%s", "1-Set File Name\n");
        printf("%s", "2-Set Unit Size\n");
        printf("%s", "3-File Display\n");
        printf("%s", "4-File Modify\n");
        printf("%s", "5-Copy From File\n");
        printf("%s", "6-Quit\n");
        scanf("%d", &choose);
        options[choose-1]();
    }
    
    return 0;
}