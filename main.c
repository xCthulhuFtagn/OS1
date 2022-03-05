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

// for files & dir operations
// tar {options} archive.tar {sources} [file1 file2 file3 ...] | dir
// sources: -d = directories / -f = files
// options:  -pack / -unpack

// struct {
//     uint64_t freq[256];
// } BRUH;

int archive_desc;
int log_desc;

int file_pack(char * source, char lvl){
    write(log_desc, "Packaging file ", 15);
    write(log_desc, source, strlen(source));
    write(log_desc, "...\n", 4);
    write(archive_desc, &lvl, sizeof(lvl));
    write(archive_desc, "f", 1);
    write(archive_desc, source, strlen(source));
	int src_desc, dst_desc, in, out;
	if ((src_desc = open(source, O_RDONLY)) == -1) {
        char buf[PATH_MAX + 29] = "Could not open source file ";
        strcat(buf, source);
        strcat(buf, "\n");
        perror(buf);
		return -1;
	}
    char buf[BUFSIZ];
    char fail = 0;
    ssize_t read_status, write_status;
    while ((read_status = read(src_desc, buf, BUFSIZ)) > 0) {
        if ((write_status = write(archive_desc, buf, BUFSIZ)) == -1) {
            fail = 1;
            break;
        }
    }
    if (read_status == -1) fail = 1;
	close(src_desc);

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
int dir_pack(char* from_ch, char lvl){
    struct stat st;
    struct dirent *obj;
    DIR *origin, *subdir;
    char buf[PATH_MAX + 200];
    write(log_desc, "Packaging directory ", 21);
    write(log_desc, from_ch, strlen(from_ch));
    write(log_desc, "...\n", 4);
    if ((origin = opendir(from_ch)) == NULL) {
        char buf1[PATH_MAX + 26] = "Could not open source directory ";
        strcat(buf1, from_ch);
        perror(buf1);
        return -1;
    }
    write(archive_desc, &lvl, sizeof(lvl));
    write(archive_desc, "d", 1);
    write(archive_desc, from_ch, strlen(from_ch));
    /*
        логика:
        гуляем при помощи chdir по архивным файлам, чтобы можно было без пота создавать новые объекты
        гуляем при помощи opendir и ПОЛНЫХ адресов по архивируемым директориям
    */
    strcat(from_ch, "/");
    while ((obj = readdir(origin)) != NULL) {
        strcat(from_ch, obj->d_name);
        if (lstat(from_ch, &st) == -1) {
            perror("lstat broke");
            return -1;
        }
        char output[BUFSIZ] = "";
        strnset(output, '\t', lvl);
        if (S_ISDIR(st.st_mode)) {
            if (strcmp(obj->d_name, ".") != 0 && strcmp(obj->d_name, "..") != 0) {
                // found directory
                strcat(output, "Dir ");
                strcat(output, obj->d_name);
                strcat(output, ": ");
                if (dir_pack(from_ch, lvl + 1) == 0) {
                    strcat(output, "SUCCESS\n");
                } else {
                    strcat(output, "FAIL\n");
                }
            }
        }
        else {
            //copy file
            strcat(output, "File ");
            strcat(output, obj->d_name);
            strcat(output, " (");
            //mb %d
            sprintf(output + strlen(output), "%ld", st.st_size);
            strcat(output, "): ");
            if (file_pack(from_ch, lvl + 1) == 0) {
                strcat(output, "SUCCESS\n");
            } else {
                strcat(output, "FAIL\n");
            }
        }
        write(log_desc, output, strlen(output));
        CutAdress(from_ch);
    }
    from_ch[strlen(from_ch)-1] = '\0';
    CutAdress(from_ch);
    //chdir("..");
    closedir(origin);
    return 0;
}

int main(int argc, char* argv[]){
    char create_flag;
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
            perror("Could not create archive");
            return -1;
        }
        if (chdir(argv[2]) < 0) perror("Can't change directory");
        char bruh[PATH_MAX];
        strcpy(bruh, path);
        log_desc = open(strcat(bruh, "/log.txt"), O_RDWR | O_APPEND | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
        archive_desc = open(argv[2], O_RDWR | O_APPEND | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
        if (log_desc == -1) {
            perror("log.txt doesn't open ");
            return -1;
        }
        if (archive_desc == -1) {
            perror("Could not create archive file");
            return -1;
        }
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
            dir_pack(path, 0);
        }
        else if (strcmp(argv[3], "-f") == 0) {
            //list of files
        }
        else {
            perror("Wrong parameter for input");
            return -1;
        }
        if (close(log_desc) == -1) {
            perror("log.txt doesn't close ");
            return -1;
        }
    }
    else if (strcmp(argv[1], "-extract") == 0) {
        //
    }
    else {
        perror("Wrong command parameters");
        return -1;
    }
    if (close(archive_desc) == -1) {
        perror("Archive file didn't close");
        return -1;
    }
    return 0;
}