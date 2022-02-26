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

int file_copy(char * source, char * dest) {
	int src_desc, dst_desc, in, out;
	char buf[8192];
	if ((src_desc = open(source, O_RDONLY)) == -1) {
		return -1;
	}
    //0660 => force create mode
	if ((dst_desc = open(dest, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU/*0660*/)) == -1) {
		close(src_desc);
		return -1;
	}
	
	if ((in = read(src_desc, buf, 8192)) <= 0) return -1;
    if ((out = write(dst_desc, buf, in)) <= 0) return -1;

	close(src_desc);
	close(dst_desc);

	return 0;
}

int main(int argc, char* argv[]){
    char options;
    char *buf;
    DIR* arch;
    dirent* file;
    if (getcwd(buf, 0) == NULL) {
        printf("Current working directory is unavailable\n");
        return -1;
    }
    if (argc < 3) {
        printf("Not enough arguments\n");
        return -1;
    }
    if (argv[1] == "-create") options = 1;
    else if (argv[1] == "-extract") options = 0;
    else {
        printf("Wrong command parameters\n");
        return -1;
    }
    if(mkdir(argv[2], S_IFDIR) < 0){
        printf("Could not create archive directory\n");
        return -1;
    }
    if (strlen(argv[2]) < 2) {
        printf("Wrong output parameter\n");
        return -1;
    }
    if ((arch = opendir(argv[2])) == NULL) {
        printf("Cannot create output dir\n");
        return -1;
    }
    int file_desc = open(strcat(strcat("./", argv[2]),"/log"), O_RDWR || O_APPEND || O_CREAT, S_IRWXU);
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
    write(file_desc, "Archived files:\n", 28);
    struct stat st;
    while ((file = readdir(arch)) != NULL){
        //file_copy(./file, ./arch/file)
        if (file_copy(strcat("./", file->d_name), strcat(strcat(strcat("./", argv[2]), "/"), file->d_name)) == 0){
            //if the copy is successfull -> print into log
            write(file_desc, "\"", 1);
            write(file_desc, file->d_name, strlen(file->d_name));
            write(file_desc, "\" size: ", 9);
            fstat(file_desc, &st);
            sprintf(buf, "%d", st.st_size);
            write(file_desc, buf, strlen(buf));
            write(file_desc, "\n", 2);
        }
    }
}