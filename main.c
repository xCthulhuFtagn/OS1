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
    int32_t left, right;
};

struct trio{
    short let;
    uint64_t freq;
    int32_t ind;
};

struct{
    struct trio freqlet[256];
    struct node letters[1 << 16];
} hash_tree;

struct{
    char m[256 * 19];
} tree_table;

int cmp_trio(const void * a, const void * b){
    struct trio x, y;
    x = *(struct trio *)a;
    y = *(struct trio *)b;
    return y.freq - x.freq;
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

struct q_node{
    int32_t ind;
    int32_t code;
    char len;
    struct q_node * next;
};

void sort_hash_tree(){
    uint64_t sum = 0;
    // сортировка "мапы" 
    qsort(hash_tree.freqlet, 256, sizeof(struct trio), cmp_trio);
    for (int i = 0; i < 256; ++i) {
        printf("%hd - %lu\n", hash_tree.freqlet[i].let, hash_tree.freqlet[i].freq);
        sum += hash_tree.freqlet[i].freq;
    }
    if (sum > 256 * 19) {
        puts("There is reason to archive");
    }
    else {
        puts("There is no reason to archive");
    }
    for (int i = 255, j = 0; i > 0; --i) {
        if (hash_tree.freqlet[i].freq == 0) continue;
        // создание вершины в дереве с ссылками
        hash_tree.letters[j].let = -1;
        hash_tree.letters[j].left = hash_tree.freqlet[i - 1].ind;
        hash_tree.letters[j].right = hash_tree.freqlet[i].ind;
        // изменение частоты "объединённой буквы"
        // и расположение этих данных по порядку
        // в массиве
        hash_tree.freqlet[i - 1].ind = j;
        hash_tree.freqlet[i - 1].freq += hash_tree.freqlet[i].freq;
        struct trio tmp = hash_tree.freqlet[i - 1];
        for (int k = i - 1; k > 0; --k) {
            if (tmp.freq > hash_tree.freqlet[k - 1].freq)
                hash_tree.freqlet[k] = hash_tree.freqlet[k - 1];
            else
                break;
        }
        ++j;
    }
    struct q_node * head, * tail;
    head = (struct q_node*)malloc(sizeof(struct q_node));
    tail = (struct q_node*)malloc(sizeof(struct q_node));
    head->len = 0;
    tail->len = 0;
    head->code = 0;
    tail->code = 1;
    head->next = tail;
    tail->next = NULL;
    head->ind = hash_tree.freqlet[0].ind;
    tail->ind = hash_tree.freqlet[1].ind;
    short i_l = 0;
    short l, r;
    char y, z;
    while (head != NULL) {
        if (hash_tree.letters[head->ind].let == -1) {
            l = hash_tree.letters[head->ind].left;
            r = hash_tree.letters[head->ind].right;
            tail->next = (struct q_node*)malloc(sizeof(struct q_node));
            tail->next->ind = l;
            tail = tail->next;
            tail->code <<= 1;
            tail->code |= 0;
            tail->len++;
            tail->next = (struct q_node*)malloc(sizeof(struct q_node));
            tail->next->ind = r;
            tail = tail->next;
            tail->code <<= 1;
            tail->code |= 1;
            tail->len++;
            tail->next = NULL;
        }
        else {
            tree_table.m[i_l] = hash_tree.letters[head->ind].let;
            ++i_l;
            tree_table.m[i_l] = tail->len;
            ++i_l;
            for (y = 0; y < (tail->len + 7) / 8; ++y, ++i_l)
                tree_table.m[i_l] = tail->code & (255 << (y * 8));
        }
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
    char* fname = strrchr(from_ch, '/') + 1;
    char fnamen[PATH_MAX + 1];
    sprintf(fnamen, "%s\n", fname);
    char buf[PATH_MAX + 200];
    if (lstat(from_ch, &st) == -1) {
        perror("lstat broke");
        char tmp[BUFSIZ] = "";
        format_output(tmp, lvl);
        strcat(tmp, "Object ");
        strcat(tmp, fname);
        strcat(tmp, " : NOT FOUND\n");
        write(log_desc, tmp, strlen(tmp));
        return -1;
    }
    char output[BUFSIZ] = "";
    bool failure = false;
    if (S_ISDIR(st.st_mode)) {
        struct dirent *obj;
        DIR *origin, *subdir;
        format_output(output, lvl);
        strcat(output, "Dir ");
        strcat(output, fname);
        strcat(output, "{\n");
        strcat(from_ch, "/");
        write(log_desc, output, strlen(output));
        if ((origin = opendir(from_ch)) == NULL) {
            char buf1[PATH_MAX + 26] = "Could not open directory ";
            strcat(buf1, from_ch);
            perror(buf1);
            format_output(output, lvl);
            strcpy(output, "} FAILURE;");
            write(log_desc, output, strlen(output));
            return -1;
        }
        write(archive_desc, &lvl, sizeof(lvl));
        write(archive_desc, &st, sizeof(st));
        write(archive_desc, fnamen, strlen(fnamen));
        while ((obj = readdir(origin)) != NULL) {
            strcat(from_ch, obj->d_name);
            if (strcmp(obj->d_name, ".") != 0 && strcmp(obj->d_name, "..") != 0) {
                // found directory
                if (pack(from_ch, lvl + 1) != 0) failure = true;
            }
            CutAdress(from_ch);
        }
        from_ch[strlen(from_ch)-1] = '\0';
        CutAdress(from_ch);
        closedir(origin);
        format_output(output, lvl);
        if (failure) strcat(output, "} FAILURE\n");
        else strcat(output, "} SUCCESS\n");
        write(log_desc, output, strlen(output));
    }
    else { //file found
        format_output(output, lvl);
        strcat(output, "File ");
        strcat(output, fname);
        strcat(output, " (");
        //mb %d
        sprintf(output + strlen(output), "%ld", st.st_size);
        strcat(output, "): ");
        write(archive_desc, &lvl, sizeof(lvl));
        write(archive_desc, &st, sizeof(st));
        write(archive_desc, fnamen, strlen(fnamen));
        int src_desc, dst_desc, in, out;
        if ((src_desc = open(from_ch, O_RDONLY)) == -1) {
            char buf[PATH_MAX + 29] = "Could not open source file ";
            strcat(buf, from_ch);
            perror(buf);
            failure = true;
        }
        else {
            char buf[BUFSIZ];
            ssize_t read_status, write_status;
            while ((read_status = read(src_desc, buf, BUFSIZ)) > 0) {
                if ((write_status = write(archive_desc, buf, read_status)) == -1) {
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
        if (failure) strcat(output, "FAILURE\n");
        else strcat(output, "SUCCESS\n");
        write(log_desc, output, strlen(output));
    }
    if (failure) return -1;
    return 0;
}

int unpack(char *to){
    bool failure = false;
    struct stat st;
    char lvl, buf[BUFSIZ];
    char pr_lvl;
    char fname[PATH_MAX];
    chdir(to);
    ssize_t rs = 0;
    do {
        printf("b %s\n", to);
        int i = 0;
        rs = read(archive_desc, &lvl, sizeof(lvl));
        if (rs == 0) break;
        read(archive_desc, &st, sizeof(st));
        for (; (rs = read(archive_desc, fname + i, sizeof(char))) > 0; ++i) {
            if (fname[i] == '\n') {
                fname[i] = '\0';
                break;
            }
        }
        if (fname[i] != 0) {
            perror("Can't read information from archive");
            return -1;
        }
        while (lvl < pr_lvl) {
            chdir("..");
            //mb NULL
            *strrchr(to, '/') = '\0';
        }
        if (S_ISDIR(st.st_mode)) {
            format_output(buf, lvl);
            strcat(buf, "Dir ");
            strcat(buf, fname);
            strcat(buf, " {\n");
            printf("d %s\n", to);
            write(log_desc, buf, strlen(buf));
            if (mkdir(fname, st.st_mode) == -1) {
                if (EEXIST == errno) {
                    puts("Directory already exists");
                }
                else {
                    char buf[40] = "Can't create directory ";
                    strcat(buf, fname);
                    perror(buf);
                    failure = true;
                    break;
                }
            }
            chdir(fname);
            sprintf(to + strlen(to), "/%s", fname);
            format_output(buf, lvl);
            if (failure) {
                strcat(buf, "} FAILURE\n");
            }
            else { 
                strcat(buf, "} SUCCESS\n");
            }
        }
        else {
            format_output(buf, lvl);
            sprintf(buf + strlen(buf), "File %s (%d) : ", fname, st.st_size);
            //parse file
            if (lvl > pr_lvl)
                sprintf(to + strlen(to), "/%s", fname);
            else
                sprintf(strrchr(to, '/') + 1, fname);
            int src_desc;
            printf("f %s\n", to);
            if ((src_desc = open(to, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, st.st_mode)) == -1) {
                if (errno == EEXIST) {
                    puts("File already exists");
                    failure = true;
                    break;
                }
                else {
                    char buf[PATH_MAX + 29] = "Cannot open source file ";
                    puts(to);
                    strcat(buf, fname);
                    perror(buf);
                    failure = true;
                    break;
                }
            }
            else {
                ssize_t read_status, write_status;
                char buf[BUFSIZ];
                ssize_t rest = st.st_size;
                while ((read_status = read(archive_desc, buf, (BUFSIZ < rest ? BUFSIZ : rest))) > 0) {
                    rest -= read_status;
                    if ((write_status = write(src_desc, buf, read_status)) == -1 || read_status != write_status) {
                        failure = true;
                        perror("Copying error occured");
                        break;
                    }
                }
                if (read_status == -1) {
                    failure = true;
                    perror("Copying error occured");
                }
            }
            close(src_desc);
            //status is ready, parsing is done
            if (failure) strcat(buf, "FAILURE\n");
            else strcat(buf, "SUCCESS\n");
            write(log_desc, buf, strlen(buf));
        }
        pr_lvl = lvl;
    } while (true);
    if (failure) return -1;
    return 0;
}

int main(int argc, char* argv[]){
    char create_flag;
    char path[PATH_MAX];
    DIR *from;
    init_hash_tree();
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
    if (log_desc == -1) {
        perror("log.txt didn't open ");
        return -1;
    }
    //read - write || write to end || create it if needed, write/read/execute
    // date + ls
    time_t t = time(NULL);
    struct tm d = *localtime(&t);
    write(log_desc, "Hour and date : ", 17);
    strftime(buf, 356, "%X %d %B %Y\n", &d);
    write(log_desc, buf, strlen(buf));
    if (strcmp(argv[1], "-p") == 0) {
        sprintf(buf, "%s.arc", argv[2]);
        archive_desc = open(buf, O_WRONLY | O_APPEND | O_CREAT, S_IRWXO | S_IRWXG | S_IRWXU);
        if (archive_desc == -1) {
            perror("Could not create archive file");
            return -1;
        }
        for (int i = 3; i < argc; ++i) {
            sprintf(path + lenpath, "/%s", argv[i]);
            pack(path, 0);
        }
    }
    else if (strcmp(argv[1], "-u") == 0) {
        sprintf(buf, "%s.arc", argv[2]);
        archive_desc = open(buf, O_RDONLY, S_IRWXO | S_IRWXG | S_IRWXU);
        if (archive_desc == -1) {
            perror("Could not open archive file");
            return -1;
        }
        sprintf(path + lenpath, "/%s", argv[3]);
        if (mkdir(path, S_IRWXU | S_IRGRP | S_IWGRP) == -1) {
            if (EEXIST == errno) {
                puts("This directory already exists");
            }
            else {
                perror("Can't create directory");
                return -1;
            }
        }
        unpack(path);
    }
    else {
        printf("Wrong command parameters");
        return -1;
    }
    if (close(archive_desc) == -1) {
        perror("Archive file didn't close");
        return -1;
    }
    if (close(log_desc) == -1) {
        perror("log.txt didn't close ");
        return -1;
    }
    return 0;
}