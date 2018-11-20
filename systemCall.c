#include "lab4_util.h"
 #define SYS_READ 0
 #define SYS_WRITE 1
 #define SYS_OPEN 2
 #define SYS_CLOSE 3
 #define SYS_EXIT 1
 #define STDOUT 1
 #define STDERR 2
 #define SYS_LSEEK 8
 #define SYS_GETDENTS 78
 #define BUF_SIZE 8192
 #define DT_DIR 4


 extern int system_call();

struct linux_dirent {
   long           d_ino;
   unsigned long  d_off;
   unsigned short d_reclen;
   char           d_name[];
};

char *my_strcat(char *strg1, char *strg2)
{
    char *start = strg1;
    while(*strg1 != '\0')
    {
        strg1++;
    }
    while(*strg2 != '\0')
    {
        *strg1 = *strg2;
        strg1++;
        strg2++;
    }
    *strg1 = '\0';
    return start;
}

int cmdDone=0;

void search(int fd, char* path, char* fileName, char* command){
    /*system_call(SYS_WRITE, STDOUT, path, simple_strlen(path));*/
    int nread;
    char buf[BUF_SIZE];        
    struct linux_dirent *d;
    int bpos=0;
    char d_type;
    char fullPath[1024];
    fullPath[0]='\0';
    my_strcat(fullPath, path);
    if (fd == 0)
        system_call(SYS_EXIT,0x55);
    if (fd == -1)
        system_call(SYS_EXIT,0x55);
    for ( ; ; ) {
        my_strcat(fullPath, "/");
        nread = system_call(SYS_GETDENTS, fd, buf, BUF_SIZE);
        if (nread == -1)
            system_call(SYS_EXIT,0x55);
        if (nread == 0)
           break;
        while (bpos < nread) {
            d = (struct linux_dirent *) (buf + bpos);
            d_type = *(buf + bpos + d->d_reclen - 1);  
            if (simple_strcmp(d->d_name, ".") == 0 || simple_strcmp(d->d_name, "..") == 0){
                /* do nothing */
            }
            else{     
                if (d_type == DT_DIR){
                    int lastSize = simple_strlen(fullPath);
                    my_strcat(fullPath, d->d_name);
                    if ((simple_strcmp(fileName, "")==0 && simple_strcmp(command, "")==0) || (simple_strcmp(fileName, d->d_name)==0 && simple_strcmp(command, "")==0)) {
                        system_call(SYS_WRITE, STDOUT, fullPath, simple_strlen(fullPath));
                        system_call(SYS_WRITE, STDOUT, "\n", 1);
                    }
                    fd = system_call(SYS_OPEN, fullPath, 0,0);
                    search(fd, fullPath, fileName, command);
                    fullPath[lastSize]='\0';
                }
                else{
                    if ((simple_strcmp(fileName,"")!=0 && simple_strcmp(d->d_name, fileName)==0 && simple_strcmp(command, "")==0) || 
                        (simple_strcmp(fileName, "")==0 && simple_strcmp(command, "")==0)){
                        system_call(SYS_WRITE, STDOUT, fullPath, simple_strlen(fullPath));
                        system_call(SYS_WRITE, STDOUT, d->d_name, simple_strlen(d->d_name));
                        system_call(SYS_WRITE, STDOUT, "\n", 1);
                    }
                    else {
                        if ((simple_strcmp(command, "")!=0 &&
                         simple_strcmp(d->d_name, fileName)==0)){
                            my_strcat(command, " ");
                            my_strcat(fullPath, d->d_name);
                            my_strcat(command, fullPath);
                            simple_system(command);
                            cmdDone=1;
                            break;
                        }
                        /* do nothing */
                    }
                }
            }
            bpos += d->d_reclen;
        }
    }
}

int main(int argc, char **argv)
{
    int fd;
    fd = system_call(SYS_OPEN, ".", 0,0);
    if (argc > 1 && simple_strcmp(argv[1], "-n")==0){
        search(fd, ".", argv[2], "");
    }
    else {
        if (argc > 3 && simple_strcmp(argv[1], "-e")==0){
            search(fd, ".", argv[2], argv[3]);
        }
        else {
            search(fd, ".", "", "");
        }
    }
    char error[1024];
    if (argc >3  && simple_strcmp(argv[3], "")!=0 && simple_strcmp(argv[2], "")!=0 && cmdDone==0){
    	my_strcat(error, "The File '");
    	my_strcat(error, argv[2]);
    	my_strcat(error, "'");
    	my_strcat(error, " Does not exist.");
    	system_call(SYS_WRITE, STDOUT, error, simple_strlen(error));
    	system_call(SYS_WRITE, STDOUT, "\n", 1);
    }
    system_call(SYS_CLOSE, fd);
    return 0;
}       