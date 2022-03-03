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
    //maybe "/"'s in adress should be doubled, I don't know
	int src_desc, dst_desc, in, out;
	if ((src_desc = open(source, O_RDONLY)) == -1) {
		return -1;
	}
	if ((dst_desc = open(dest, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU)) == -1) {
		close(src_desc);
		return -1;
	}
    char buf[BUFSIZ];
    bool fail = false;
    ssize_t read_status, write_status;
    while ((read_status = read(src_desc, buf, BUFSIZ)) == 0) {
        if ((write_status = write(dst_desc, buf, BUFSIZ)) == -1) {
            fail = true;
            break;
        }
    }
    if (read_status == -1) {
        fail = true;
    }
	
	close(src_desc);
	close(dst_desc);

	if (fail) {
        perror("Copying error occured");
        return -1;
    }
    else return 0;
}

//recursive for first realisation
void dir_copy(char* from_ch, char* to_ch, int log_desc){
    struct stat st;
    dirent *obj;
    DIR *origin, *subdir;
    char buf[20];
    if ((origin = opendir(from_ch)) == NULL) {
        char buf[PATH_MAX + 26] = "Could not open directory ";
        strcat(buf, from_ch);
        perror(buf);
        return;
    }
    if (chdir(to_ch) < 0) {
        char buf[PATH_MAX + 26] = "Could not open directory ";
        strcat(buf, to_ch);
        perror(buf);
        return;
    }
    /*
        логика: 
        гуляем при помощи chdir по архивным файлам, чтобы можно было без пота создавать новые объекты
        гуляем при помощи opendir и ПОЛНЫХ адресов по архивируемым директориям
    */
    while ((obj = readdir(origin)) != NULL) {
        lstat(obj->d_name, &st);
        if (S_ISDIR(st.st_mode)) {
            if (strcmp(obj->d_name, ".") || strcmp(obj->d_name, "..")) {
                //need 2 copy them? yes!
            } else {
                // found directory
                if (mkdir(obj->d_name, O_RDWR || O_APPEND || O_CREAT || S_IRWXU) < 0) {
                    char buf[PATH_MAX + 19] = "Could not copy ";
                    strcat(buf, obj->d_name);
                    strcat(buf, " dir");
                    perror(buf);
                }
                else {
                    char from_subdir[PATH_MAX];
                    strcpy(from_subdir, from_ch);
                    strcat(from_subdir, "/");
                    strcat(from_subdir, obj->d_name);
                    dir_copy(from_subdir, obj->d_name, log_desc);   
                }             
            }
        } else {
            //copy file
            if (file_copy(strcat(strcat(from_ch, "/"), obj->d_name), obj->d_name) == 0) {
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
    }
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
    if (strcmp(argv[1], "-create") == 0) create_flag = true;
    else if (strcmp(argv[1], "-extract") == 0) create_flag = false;
    else {
        perror("Wrong command parameters");
        return -1;
    }
    if (create_flag) {
        //char tmp[PATH_MAX]; strcpy(tmp, path); strcat(tmp, "/");
        //creating ./
        int i;
        //need to check if it works without such bullshit as in comments
        if ((i = mkdir(argv[2]/*strcat(tmp, argv[2])*/, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) < 0) {
            //Read + Write + Execute: S_IRWXU (User), S_IRWXG (Group), S_IRWXO (Others)
            perror("Could not create archive directory");
            return -1;
        }
        if (strlen(argv[2]) < 2) {
            perror("Wrong output parameter");
            return -1;
        }
        if(chdir(argv[2]) < 0) perror("can't change dir");
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
            //here was segmentation fault (fixed)
            strcat(path, "/");
            strcat(path, argv[4]);
            //inputting full adress of input
            printf("call of dir_copy\n");
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
    free(path);
    return 0;
}