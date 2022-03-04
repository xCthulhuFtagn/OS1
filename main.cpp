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
// tar {options} archive.tar {sources} [file1 file2 file3 ...] | dir
//sources: -d = directories / -f = files
// options:  -create / -extract

int file_copy(char * source, char * dest) {
	int src_desc, dst_desc, in, out;
	if ((src_desc = open(source, O_RDONLY)) == -1) {
        char buf[PATH_MAX + 29] = "Could not open source file ";
        strcat(buf, source);
        strcat(buf, "\n");
        perror(buf);
		return -1;
	}
	if ((dst_desc = open(dest, O_WRONLY|O_CREAT|O_TRUNC , S_IRWXU)) == -1) {
		close(src_desc);
        char buf[PATH_MAX + 34] = "Could not open destination file ";
        strcat(buf, source);
        strcat(buf, "\n");
        perror(buf);
		return -1;
	}
    char buf[BUFSIZ];
    bool fail = false;
    ssize_t read_status, write_status;
    while ((read_status = read(src_desc, buf, BUFSIZ)) > 0) {
        if ((write_status = write(dst_desc, buf, BUFSIZ)) == -1) {
            fail = true;
            break;
        }
    }
    if (read_status == -1) fail = true;
	close(src_desc);
	close(dst_desc);

	if (fail) {
        perror("Copying error occured");
        return -1;
    }
    else return 0;
}

void CutAdress(char* adr){
    char* slash = strrchr(adr, '/');
    if (slash && slash + 1 != adr + strlen(adr)) {
        *(slash + 1) = '\0';
    }
}

//recursive for first realisation
void dir_copy(char* from_ch, char* to_ch, int log_desc){
    struct stat st;
    dirent *obj;
    DIR *origin, *subdir;
    char buf[20];
    if ((origin = opendir(from_ch)) == NULL) {
        char buf[PATH_MAX + 26] = "Could not open source directory ";
        strcat(buf, from_ch);
        perror(buf);
        return;
    }
    if (chdir(to_ch) < 0) {
        char buf[PATH_MAX + 26] = "Could chdir to destination directory ";
        strcat(buf, to_ch);
        perror(buf);
        return;
    }
    /*
        логика: 
        гуляем при помощи chdir по архивным файлам, чтобы можно было без пота создавать новые объекты
        гуляем при помощи opendir и ПОЛНЫХ адресов по архивируемым директориям
    */
    strcat(from_ch, "/");
    while ((obj = readdir(origin)) != NULL) {
        strcat(from_ch, obj->d_name);
        if(lstat(from_ch, &st) == -1) perror("lstat broken");
        if (S_ISDIR(st.st_mode)) {
            if (strcmp(obj->d_name, ".") != 0 && strcmp(obj->d_name, "..") != 0) {
                // found directory
                if (mkdir(obj->d_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
                    //https://techoverflow.net/2013/04/05/how-to-use-mkdir-from-sysstat-h/
                    char buf[PATH_MAX + 19] = "Could not copy ";
                    strcat(buf, obj->d_name);
                    strcat(buf, " dir");
                    perror(buf);
                }
                else {
                   dir_copy(from_ch, obj->d_name, log_desc);
                }
            }
        }
        else {
            //copy file
            if (file_copy(from_ch, obj->d_name) == 0) {
                //if the copy is successfull -> print into log
                write(log_desc, "File \"", 1);
                write(log_desc, obj->d_name, strlen(obj->d_name));
                write(log_desc, "\", size: ", 10);
                fstat(log_desc, &st);
                sprintf(buf, "%ld", st.st_size);
                write(log_desc, buf, strlen(buf));
                write(log_desc, "\n", 2);
            }
            else {
                char buf[PATH_MAX + 30] = "File ";
                strcat(buf, obj->d_name);
                strcat(buf, " could not not be copied");
                perror(buf);
            }
        }
        CutAdress(from_ch);
    }
    from_ch[strlen(from_ch)-1] = '\0';
    CutAdress(from_ch);
    chdir("..");
    closedir(origin);
}

int main(int argc, char* argv[]){
    bool create_flag;
    char path[PATH_MAX];
    DIR *from;
    if (getcwd(path, PATH_MAX) == NULL) {
        perror("Current working directory is unavailable");
        return -1;
    }
    if (argc < 3) {
        perror("Not enough arguments");
        return -1;
    }
    if (strcmp(argv[1], "-create") == 0) {
        int i;
        if ((i = mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0) {
            //Read + Write + Execute: S_IRWXU (User), S_IRWXG (Group), S_IRWXO (Others)
            perror("Could not create archive directory");
            return -1;
        }
        if (chdir(argv[2]) < 0) perror("Can't change directory");
        int log_desc = open("log.txt", O_RDWR || O_APPEND || O_CREAT, S_IRWXU);
        //read - write || write to end || create it if needed, write/read/execute
        // date + ls
        time_t t = time(NULL);
        struct tm d = *localtime(&t);
        char buf[20];
        write(log_desc, "Date and hour : ", 17);
        sprintf(buf, "%d", d.tm_mday);
        write(log_desc, buf, strlen(buf));
        write(log_desc, "/", 1);   
        sprintf(buf, "%d", d.tm_mon);
        write(log_desc, buf, strlen(buf));
        write(log_desc, "/", 1);
        sprintf(buf, "%d", d.tm_year);
        write(log_desc, buf, strlen(buf));
        write(log_desc, " ", 1);
        sprintf(buf, "%d", d.tm_hour);
        write(log_desc, buf, strlen(buf));
        write(log_desc, ":", 1);
        sprintf(buf, "%d", d.tm_min);
        write(log_desc, buf, strlen(buf));
        write(log_desc, ":", 1);
        sprintf(buf, "%d", d.tm_sec);
        write(log_desc, buf, strlen(buf));
        write(log_desc, "\n", 2);
        chdir("..");
        if (strcmp(argv[3], "-d") == 0) { // archive whole directory
            strcat(path, "/");
            strcat(path, argv[4]);
            //inputting full adress of input
            dir_copy(path, argv[2], log_desc);
        }
        else if (strcmp(argv[3], "-f") == 0) {
            //list of files
        }
        else {
            perror("Wrong parameter for input");
            return -1;
        }
    }
    else if (strcmp(argv[1], "-extract") == 0){

    }
    else{
        perror("Wrong command parameters");
        return -1;
    }
    return 0;
}