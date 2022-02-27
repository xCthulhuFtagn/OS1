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
	char buf[8192];
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
    while((read_status = read(src_desc, buf, BUFSIZ)) == 0){
        if((write_status = write(dst_desc, buf, BUFSIZ)) == -1){
            fail = true;
            break;
        }
    }
    if(read_status == -1){
        fail = true;
    }
	
	close(src_desc);
	close(dst_desc);

	if(fail) {
        perror("Copying error occured");
        return -1;
    }
    else return 0;
}

//recursive for first realisation
void dir_copy(char* from_ch, char* to){
    struct stat st;
    dirent *obj;
    DIR *origin, *subdir;
    char buf[20];
    if((origin = opendir(from_ch)) == NULL){
        char buf[PATH_MAX + 26] = "Could not open directory ";
        strcat(buf, from_ch);
        perror(buf);
        return;
    }
    while ((obj = readdir(origin)) != NULL){
        //file_copy(./file, ./arch/file)
        lstat(obj->d_name, &st);
        if(S_ISDIR(st.st_mode)){
            if(strcmp(obj->d_name, ".") || strcmp(obj->d_name, "..")) {
                //need 2 copy them? yes!
            } else{
                // enter directory
                char to_subdir[PATH_MAX];
                strcpy(to_subdir, to);
                strcat(to_subdir, obj->d_name);
                dir_copy(obj->d_name, to_subdir);                
            }
        } else{
            //copy file
            if (file_copy(strcat("./", obj->d_name), strcat(strcat(to, "/"), obj->d_name)) == 0){
                //if the copy is successfull -> print into log
                write(log_desc, "\"", 1);
                write(log_desc, obj->d_name, strlen(obj->d_name));
                write(log_desc, "\", size: ", 10);
                fstat(log_desc, &st);
                sprintf(buf, "%ld", st.st_size);
                write(log_desc, buf, strlen(buf));
                write(log_desc, "\n", 2);
            }
            else{
                printf("File %s could not not be copied", obj->d_name);
                perror("");
            }
        }
    }
    chdir("..");
    closedir(origin);
}

int log_desc;

int main(int argc, char* argv[]){
    bool create_flag;
    char path[PATH_MAX] = "";
    DIR *from;
    if (getcwd(path, PATH_MAX) == NULL) {
        perror("Current working directory is unavailable\n");
        return -1;
    }
    if (argc < 3) {
        perror("Not enough arguments\n");
        return -1;
    }
    if (strcmp(argv[1], "-create") == 0) create_flag = true;
    else if (strcmp(argv[1], "-extract") == 0) create_flag = false;
    else {
        perror("Wrong command parameters\n");
        return -1;
    }
    if(create_flag){
        //char tmp[PATH_MAX]; strcpy(tmp, path); strcat(tmp, "/");
        //creating ./
        int i;
        //need to check if it works without such bullshit as in comments
        if((i = mkdir(argv[2]/*strcat(tmp, argv[2])*/, S_IRWXO || S_IRWXG || S_IRWXU)) < 0){
            //Read + Write + Execute: S_IRWXU (User), S_IRWXG (Group), S_IRWXO (Others)
            perror("Could not create archive directory\n");
            return -1;
        }
        if (strlen(argv[2]) < 2) {
            perror("Wrong output parameter\n");
            return -1;
        }
        chdir(strcat("/", argv[2]));
        log_desc = open("log.txt", O_RDWR || O_APPEND || O_CREAT, S_IRWXU);
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
        if(argv[3] == "-d"){ // archive whole directory
            //write(log_desc, "Those files were copied:\n", 28);
            dir_copy(argv[4], strcat(strcat(path, "/"), argv[2]));
        }
        else if(argv[3] == "-f"){
            /*
            struct stat st;
            while ((file = readdir(arch)) != NULL){
                //file_copy(./file, ./arch/file)
                bool found = false;
                for(size_t i = 4; i + 1 < argc; ++i){
                    if(strcmp(file->d_name, argv[i]) == 0){
                        found = true;
                        break;
                    }
                }
                if (found && file_copy(strcat("./", file->d_name), strcat(strcat(strcat("./", argv[2]), "/"), file->d_name)) == 0){
                    //if file is in the list => if the copy is successfull -> print into log
                    write(log_desc, "\"", 1);
                    write(log_desc, file->d_name, strlen(file->d_name));
                    write(log_desc, "\" size: ", 9);
                    fstat(log_desc, &st);
                    sprintf(buf, "%ld", st.st_size);
                    auto m = st.st_mode;
                    write(log_desc, buf, strlen(buf));
                    write(log_desc, "\n", 2);
                }
                else{
                    printf("%s could not not be copied\n", file->d_name);
                }
            }
            */
        }
        else{
            perror("Wrong parameter for input\n");
            return -1;
        }
    }
}