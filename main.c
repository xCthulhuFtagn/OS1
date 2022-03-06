#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

//wow, bool in c? yeah
#include <stdbool.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

// for files & dir operations
// tar {options} archive.tar [src1 src2 src3 ...] | dir
// options:  -p = pack / -u = unpack
// if pack, then sources (directories|files)
// if unpack, then ONLY directory

struct node{
    short let;
    short left, right;
};

struct trio{
    short let;
    uint64_t freq;
    short ind;
};

struct cmap{
    struct trio freqlet[256];
    struct node letters[1 << 16];
} hash_tree;

int cmp_trio(struct trio a, struct trio b){
    return b.freq - a.freq;
}

void init_hash_tree(){
    for (int i = 0; i < 256; ++i) {
        hash_tree.freqlet[i].let = i;
        hash_tree.freqlet[i].freq = 0;
        hash_tree.freqlet[i].ind = i;
        hash_tree.letters[i].let = i;
        hash_tree.letters[i].left = -1;
        hash_tree.letters[i].right = -1;
    }
}

void sort_hash_tree(){
    uint64_t sum = 0;
    qsort(hash_tree.freqlet, 256, sizeof(struct trio), cmp_trio);
    for (int i = 0; i < 256; ++i) {
        printf("%h - %lu\n", hash_tree.freqlet[i].let, hash_tree.freqlet[i].freq);
        sum += hash_tree.freqlet[i].freq;
    }
    if (sum > 256 * 16) {
        puts("There is reason to archive");
    }
    else {
        puts("There is no reason to archive");
    }
    //
    for (int i = 255, j = 0; i > 0; --i) {
        if (hash_tree.freqlet[i].freq == 0) continue;
        hash_tree.letters[j].let = -1;
        hash_tree.letters[j].left = hash_tree.freqlet[i - 1].ind;
        hash_tree.letters[j].right = hash_tree.freqlet[i].ind;
        hash_tree.freqlet[i - 1].ind = j;
        hash_tree.freqlet[i - 1].freq += hash_tree.freqlet[i].freq;
    }
}

int archive_desc;
int log_desc;

void CutAdress(char* adr){
    char* slash = strrchr(adr, '/');
    if (slash && slash + 1 != adr + strlen(adr)) {
        *(slash + 1) = '\0';
    }
}

void format_output(char* output, char lvl){
    for (char i = 0; i < lvl; ++i) output[i] =  '\t';
    output[lvl] = '\0';
}

int pack(char* from_ch, char lvl){
    struct stat st;
    char buf[PATH_MAX + 200];
    if (lstat(from_ch, &st) == -1) {
        perror("lstat broke");
        printf("current from_ch = %s", from_ch);
        return -1;
    }
    char output[BUFSIZ] = "";
    bool failure = false;
    if (S_ISDIR(st.st_mode)) {
        struct dirent *obj;
        DIR *origin, *subdir;
        format_output(output, lvl);
        strcat(output, "Dir ");
        strcat(output, strrchr(from_ch, '/') + 1);
        strcat(output, "{\n");
        strcat(from_ch, "/");
        write(log_desc, output, strlen(output));
        if ((origin = opendir(from_ch)) == NULL) {
            char buf1[PATH_MAX + 26] = "Could not open source directory ";
            strcat(buf1, from_ch);
            perror(buf1);
            format_output(output, lvl);
            strcpy(output, "} FAILURE;");
            write(log_desc, output, strlen(output));
            return -1;
        }
        write(archive_desc, &lvl, sizeof(lvl));
        write(archive_desc, "d", 1);
        write(archive_desc, from_ch, strlen(from_ch));
        while ((obj = readdir(origin)) != NULL) {
            strcat(from_ch, obj->d_name);
            if (strcmp(obj->d_name, ".") != 0 && strcmp(obj->d_name, "..") != 0) {
                // found directory
                if(pack(from_ch, lvl + 1) != 0) failure = true;
            }
            CutAdress(from_ch);
        }
        from_ch[strlen(from_ch)-1] = '\0';
        CutAdress(from_ch);
        closedir(origin);
        format_output(output, lvl);
        if(failure) strcat(output, "} FAIL\n");
        else strcat(output, "} SUCCESS\n");
        write(log_desc, output, strlen(output));
    }
    else { //file found
        bool lstat_broken = false;
        format_output(output, lvl);
        strcat(output, "File ");
        strcat(output, strrchr(from_ch, '/') + 1);
        strcat(output, " (");
        //mb %d
        if (lstat(from_ch, &st) == -1) {
            perror("lstat broke");
            printf("current from_ch = %s\n", from_ch);
            lstat_broken = true;
        }
        if (lstat_broken) strcat(output, "SIZE_ERROR");
        else sprintf(output + strlen(output), "%ld", st.st_size);
        strcat(output, "): ");
        write(archive_desc, &lvl, sizeof(lvl));
        write(archive_desc, "f", 1);
        write(archive_desc, from_ch, strlen(from_ch));
        int src_desc, dst_desc, in, out;
        if ((src_desc = open(from_ch, O_RDONLY)) == -1) {
            char buf[PATH_MAX + 29] = "Could not open source file ";
            strcat(buf, from_ch);
            strcat(buf, "\n");
            perror(buf);
            failure = true;
        }
        else {
            char buf[BUFSIZ];
            ssize_t read_status, write_status;
            while ((read_status = read(src_desc, buf, BUFSIZ)) > 0) {
                if ((write_status = write(archive_desc, buf, BUFSIZ)) == -1) {
                    failure = true;
                    perror("Copying error occured");
                    break;
                }
            }
            if (read_status == -1) {
                failure = true;
                perror("Copying error occured");
            }
            close(src_desc);
        }
        if (failure) strcat(output, "FAIL\n");
        else strcat(output, "SUCCESS\n");
        write(log_desc, output, strlen(output));
    }
    if (failure) return -1;
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
    size_t lenpath = strlen(path);
    if (argc < 3) {
        perror("Not enough arguments");
        return -1;
    }
    char buf[PATH_MAX];
    log_desc = open("log.txt", O_RDWR | O_APPEND | O_CREAT, S_IRWXU | S_IRGRP);
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
    write(log_desc, "Hour and date : ", 17);
    strftime(buf, 400, "%X %d %B %Y\n", &d);
    write(log_desc, buf, strlen(buf));
    if (strcmp(argv[1], "-p") == 0) {
        chdir("..");
        for (int i = 3; i < argc; ++i) {
            sprintf(path + lenpath, "/%s", argv[i]);
            pack(path, 0);
        }
    }
    else if (strcmp(argv[1], "-u") == 0) {
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
    if (close(log_desc) == -1) {
        perror("log.txt doesn't close ");
        return -1;
    }
    return 0;
}