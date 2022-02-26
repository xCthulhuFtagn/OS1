#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <dirent.h>
#include <unistd.h>
//for files & dir operations

using namespace std;

// tar options archive.tar [file1 file2 file3 ...] | dir
// options -- -create -extract

int main(int argc, char* argv[]){
    char options;
    char *buf;
    if (getcwd(buf, 0) == NULL) {
        printf("Pososix)))");
        return 0;
    }
    if (argc < 3) {
        printf("Sosi Pososix\n");
        return 0;
    }
    if (argv[1] == "-create") options = 1;
    else if (argv[1] == "-extract") options = 0;
    else {
        printf("Pososix)))\n");
        return 0;
    }
    int status = mkdir(argv[2], S_IFDIR);
    if(strlen(argv[2]) < 2){
        printf("Wrong output parameter!\n");
        return -1;
    }
    DIR* arch;
    if((arch = opendir(argv[2])) == NULL){
        printf("Cannot create output dir!\n");
        return -1;
    }
    dirent* file;
    int file_desc = open("./log", O_RDWR || O_APPEND || O_CREAT, S_IRWXU);
    //read - write || write to end || create it if needed, write/read/execute

    // date + ls
    time_t t = time(NULL);
    struct tm d = *localtime(&t);
    char buf[20];
    write(file_desc, "Date and hour : ", 17);
    sprintf(buf, "%d", d.tm_mday);
    write(file_desc, buf, strlen(buf));
    write(file_desc, "/", 1);   
    sprintf(buf, "%d", d.tm_mon);
    write(file_desc, buf, strlen(buf));
    write(file_desc, "/", 1);
    sprintf(buf, "%d", d.tm_year);
    write(file_desc, buf, strlen(buf));
    write(file_desc, " ", 1);
    sprintf(buf, "%d", d.tm_hour);
    write(file_desc, buf, strlen(buf));
    write(file_desc, ":", 1);
    sprintf(buf, "%d", d.tm_min);
    write(file_desc, buf, strlen(buf));
    write(file_desc, ":", 1);
    sprintf(buf, "%d", d.tm_sec);
    write(file_desc, buf, strlen(buf));
    write(file_desc, "\n", 2);
    write(file_desc, "Those files were archived:\n", 28);
    struct stat st;\
    while ((file = readdir(arch)) != NULL){
        write(file_desc, "\"", 1);
        write(file_desc, file->d_name, strlen(file->d_name));
        write(file_desc, "\" size: ", 9);
        fstat(file_desc, &st);
        sprintf(buf, "%d", st.st_size);
        write(file_desc, buf, strlen(buf));
        write(file_desc, "\n", 2);
    }
}