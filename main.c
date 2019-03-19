#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdbool.h>

enum LengthFormat{
    SHORT,
    LONG
};

enum ColumnFormat{
    SINGLECOL,
    MULTICOL
};

enum TimeForSort{
    MODIFIED,
    CHANGED
};

enum UserAndGroupFormat{
    NUMBER,
    NAME
};

struct file{
    char file_mode[11];
    long file_link;
    char file_user[30];
    char file_group[30];
    char file_size[30];
    char file_block_size[20];
    long file_inode;
    int file_month;
    int file_day;
    int file_min;
    int file_hour;
    char file_name[60];
    char output[200];
    bool is_exec;
};

struct settings{
    enum LengthFormat length_format;
    enum ColumnFormat column_format;
    enum TimeForSort time_for_sort;
    bool is_show_user_and_group_name;
    bool is_show_dot;
    bool is_show_format_char;
    bool is_show_inode_number;
    bool is_show_block_size;
    bool is_show_size_in_readable;
} cmd_settings;

struct file * files[50];
int file_count;
int max_link_length = 0;
int max_user_length = 0;
int max_group_length = 0;
int max_size_length = 0;
int max_block_size_length = 0;
int max_month_length = 0;
int max_day_length = 0;
int max_name_length = 0;
char * total_file_size;
char * output_string;

void add_output_prefix();

void init_file(struct file *f);

void modify_file_attributes();

/// 初始化输出设置
void init_settings();

/// 生成文件数组
void generate_files() ;

/// 设置文件各属性的最大长度，用于保持对齐
void set_attribute_output_length() ;

/// 设置输出长度格式
void set_basic_output() ;

/// 设置输出列数格式
void set_column_format() ;

/// 设置option
void set_opt(int argc, char **argv) ;

int main(int argc, char** argv) {
    init_settings();
    set_opt(argc, argv);
    output_string = (char *)malloc(1000);
    memset(output_string, 0, 1000);
    total_file_size = (char *)malloc(30);
    memset(total_file_size, 0, 30);
    generate_files();
    modify_file_attributes();

    set_attribute_output_length();
    set_basic_output();
    add_output_prefix();
    set_column_format();

    printf("%s \n", output_string);
}

void init_settings() {
    cmd_settings.column_format = MULTICOL;
    cmd_settings.length_format = SHORT;
    cmd_settings.time_for_sort = MODIFIED;
    cmd_settings.is_show_dot = false;
    cmd_settings.is_show_format_char = false;
    cmd_settings.is_show_inode_number = false;
    cmd_settings.is_show_block_size = false;
    cmd_settings.is_show_user_and_group_name = true;
    cmd_settings.is_show_size_in_readable = false;
}

void set_opt(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "AacCdFfhiklnqRrSstuwx1")) != -1){
        switch (opt){
            case 'l':
                cmd_settings.length_format = LONG;
                cmd_settings.column_format = SINGLECOL;
                break;
            case 'n':
                cmd_settings.length_format = LONG;
                cmd_settings.column_format = SINGLECOL;
                cmd_settings.is_show_user_and_group_name = false;
                break;
            case 'A':
                cmd_settings.is_show_dot = false;
                break;
            case 'a':
                cmd_settings.is_show_dot = true;
                break;
            case 'c':
                cmd_settings.time_for_sort = CHANGED;
                break;
            case 'F':
                cmd_settings.is_show_format_char = true;
                break;
            case 'i':
                cmd_settings.is_show_inode_number = true;
                break;
            case '1':
                cmd_settings.column_format = SINGLECOL;
                break;
            case 's':
                cmd_settings.is_show_block_size = true;
                break;
            case 'h':
                cmd_settings.is_show_size_in_readable = true;
            default:
                ;
        }
    }
}


/// 生成文件数组
void generate_files() {
    char * path;
    char * file_mode;
    char * parent_path;
    int path_length;
    struct stat file_state;
    struct timespec ts;
    struct tm * time;
    struct dirent* entry;
    DIR * dir;

    dir = opendir(".");
    path = (char *)malloc(50);
    memset(path, 0, 50);
    file_mode = (char *)malloc(11);
    memset(file_mode, 0, 11);
    parent_path = ".";
    path_length = (int)strlen(parent_path);
    file_count = 0;


    while ((entry = readdir(dir)) != NULL ){
        if (!cmd_settings.is_show_dot && (strncmp(entry->d_name, ".", 1) == 0))
            continue;

        sprintf(path, "%s/%s", parent_path, entry->d_name);
        stat(path, &file_state);

        // 设置filemode
        switch (file_state.st_mode & S_IFMT){
            // 还有几个文件类型没写
            case S_IFREG:
                file_mode[0] = '-';
                break;
            case S_IFCHR:
                file_mode[0] = 'c';
                break;
            case S_IFDIR:
                file_mode[0] = 'd';
                break;
            case S_IFLNK:
                file_mode[0] = 'l';
                break;
            case S_IFSOCK:
                file_mode[0] = 's';
                break;
            case S_IFIFO:
                file_mode[0] = 'p';
                break;
            case S_IFBLK:
                file_mode[0] = 'b';
                break;
            default:
                file_mode[0] = '?';
        }
        file_mode[1] = (file_state.st_mode & S_IRUSR) ? 'r' : '-';
        file_mode[2] = (file_state.st_mode & S_IWUSR) ? 'w' : '-';
        file_mode[3] = (file_state.st_mode & S_IXUSR) ? 'x' : '-';
        file_mode[4] = (file_state.st_mode & S_IRGRP) ? 'r' : '-';
        file_mode[5] = (file_state.st_mode & S_IWGRP) ? 'w' : '-';
        file_mode[6] = (file_state.st_mode & S_IXGRP) ? 'x' : '-';
        file_mode[7] = (file_state.st_mode & S_IROTH) ? 'r' : '-';
        file_mode[8] = (file_state.st_mode & S_IWOTH) ? 'w' : '-';
        file_mode[9] = (file_state.st_mode & S_IXOTH) ? 'x' : '-';
        file_mode[10] = '\0';

        // 设置时间

        switch (cmd_settings.time_for_sort){
            case MODIFIED:
                ts = file_state.st_mtim;
                break;
            case CHANGED:
                ts = file_state.st_ctim;
                break;
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        time = localtime(&ts.tv_sec);

        // 向file数组中添加file
        struct file *f = malloc(sizeof(struct file));
        long bs =  (file_state.st_size % 1024 == 0) ? (file_state.st_size / 1024) : (file_state.st_size / 1024 +1);
        init_file(f);
        strcpy(f->file_mode, file_mode);
        strcpy(f->file_name, entry->d_name);
        sprintf(f->file_user, "%d", file_state.st_uid);
        sprintf(f->file_group, "%d", file_state.st_gid);
        sprintf(f->file_size, "%ld", file_state.st_size);
        sprintf(f->file_block_size, "%ld", bs);
        f->file_inode = file_state.st_ino;
        f->file_link = file_state.st_nlink;
        f->is_exec = (bool)( S_IEXEC & file_state.st_mode);
        f->file_month = time->tm_mon;
        f->file_day = time->tm_mday;
        f->file_min = time->tm_min;
        f->file_hour = time->tm_hour;
        files[file_count] = f;
        file_count++;
        sprintf(total_file_size, "%ld", atol(total_file_size) + bs);

        file_mode[0] = '\0';
        path[0] = '\0';
    }
    free(path);
    free(file_mode);
}

void modify_file_attributes() {
    if(cmd_settings.is_show_format_char || cmd_settings.is_show_user_and_group_name || cmd_settings.is_show_size_in_readable){

        for (int i = 0; i < file_count; ++i) {
            struct file * f = files[i];

            if (cmd_settings.is_show_format_char){
                switch (f->file_mode[0]){
                    // 还有几个文件类型没写
                    case 'd':
                        strcat(f->file_name, "/");
                        break;
                    case 'l':
                        strcat(f->file_name, "@");
                        break;
                    case 's':
                        strcat(f->file_name, "=");
                        break;
                    case 'p':
                        strcat(f->file_name, "|");
                        break;
                    default:
                        ;
                }
                if (f->is_exec && f->file_mode[0] != 'd')
                    strcat(f->file_name, "*");
            }

            if (cmd_settings.is_show_user_and_group_name){
                struct passwd *pwd;
                struct group *group;
                pwd = getpwuid(atoi(f->file_user));
                group = getgrgid(atoi(f->file_group));
                strcpy(f->file_user, pwd->pw_name);
                strcpy(f->file_group, group->gr_name);
            }

            if (cmd_settings.is_show_size_in_readable){
                long size = atol(f->file_size);
                float readable_size = 0;
                if (size >> 30){
                    readable_size = (float)size / (1024*1024*1024);
                    sprintf(f->file_size, "%.1fG", readable_size);
                }
                else if (size >> 20) {
                    readable_size = (float)size / (1024*1024);
                    sprintf(f->file_size, "%.1fM", readable_size);
                }
                else if (size >> 10){
                    readable_size = (float)size / 1024;
                    sprintf(f->file_size, "%.1fK", readable_size);
                }
                else {
                    sprintf(f->file_size, "%ldB", size);
                }

                sprintf(f->file_block_size, "%sK", f->file_block_size);
            }
        }

    }

    if (cmd_settings.is_show_size_in_readable){
        long size = atol(total_file_size);
        float readable_size = 0;
        if (size >> 20){
            readable_size = (float)size / (1024*1024);
            sprintf(total_file_size, "%.1fG", readable_size);
        }
        else if (size >> 10){
            readable_size = (float)size / 1024;
            sprintf(total_file_size, "%.1fM", readable_size);
        }
        else{
            sprintf(total_file_size, "%sK", total_file_size);
        }
    }

}

/// 设置文件各属性的最大长度，用于保持对齐
void set_attribute_output_length() {
    char * buf = (char *)malloc(50);
    memset(buf, 0, 50);

    for (int i = 0; i < file_count; ++i) {
        struct file *f = files[i];

        sprintf(buf, "%ld", f->file_link);
        if (strlen(buf) > max_link_length)
            max_link_length = (int)strlen(buf);

        if (strlen(f->file_user) > max_user_length)
            max_user_length = (int)strlen(f->file_user);

        if (strlen(f->file_group) > max_group_length)
            max_group_length = (int)strlen(f->file_group);

        if (strlen(f->file_size) > max_size_length)
            max_size_length = (int)strlen(f->file_size);

        if (strlen(f->file_name) > max_name_length)
            max_name_length = (int)strlen(f->file_name);

        if (strlen(f->file_block_size) > max_block_size_length)
            max_block_size_length = (int)strlen(f->file_block_size);
    }
    free(buf);
}

/// 设置输出长度格式
void set_basic_output() {
    if (cmd_settings.length_format == LONG){
        for (int j = 0; j < file_count; ++j) {
            struct file *f = files[j];

            sprintf(f->output, "%s %-*ld %-*s %-*s %-*s %-*d月 %-*d日 %d:%d %s", f->file_mode,
                    max_link_length, f->file_link, max_user_length, f->file_user, max_group_length, f->file_group,
                    max_size_length, f->file_size, max_month_length, f->file_month, max_day_length, f->file_day,
                    f->file_hour, f->file_min, f->file_name);
        }
    }
    if (cmd_settings.length_format == SHORT){
        for (int i = 0; i < file_count; ++i) {
            struct file *f = files[i];

            sprintf(f->output, "%s", f->file_name);
        }
    }
}

void add_output_prefix() {
    if (cmd_settings.is_show_block_size || cmd_settings.is_show_inode_number){
        for (int i = 0; i < file_count; ++i) {
            struct file * f = files[i];
            char prefix[30] = {0};

            if (cmd_settings.is_show_inode_number){
                sprintf(prefix, "%ld", f->file_inode);
            }

            if (cmd_settings.is_show_block_size){

                if (strlen(prefix) == 0)
                    sprintf(prefix, "%-*s", max_block_size_length, f->file_block_size);
                else
                    sprintf(prefix, "%s %-*s", prefix, max_block_size_length, f->file_block_size);
            }

            char * buffer = (char *)malloc(200);
            memset(buffer, 0, 200);
            strcpy(buffer, f->output);
            sprintf(f->output, "%s %s", prefix, buffer);
            free(buffer);
        }

    }

}

/// 设置输出列数格式
void set_column_format() {
    if (cmd_settings.column_format == MULTICOL && cmd_settings.length_format == SHORT){
        struct winsize size;
        int col_num;
        int count;

        // 获取窗口宽度并计算列数目
        ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
        int max_output_length = 0;
        for (int l = 0; l < file_count; ++l) {
            int length = (int)strlen(files[l]->output);
            if (length > max_output_length)
                max_output_length = length;

        }
        col_num = size.ws_col / max_output_length > 0 ? size.ws_col / max_output_length : 1;

        // 确定每列的宽度
        int length_for_each_column[col_num];
        for (int j = 0; j < col_num; ++j) {
            length_for_each_column[j] = 0;
        }
        for (int i = 0; i < file_count; ++i) {
            int index = i % col_num;
            if (strlen(files[i]->output) > length_for_each_column[index])
                length_for_each_column[index] = (int)strlen(files[i]->output);
        }

        count = 0;
        for (int k = 0; k < file_count; ++k) {
            struct file * f = files[k];

            if (count == 0)
                sprintf(output_string, "%s%-*s", output_string, length_for_each_column[count], f->output);
            else
                sprintf(output_string, "%s  %-*s", output_string, length_for_each_column[count], f->output);

            if (++count == col_num){
                strcat(output_string, "\n");
                count = 0;
            }
        }
        // 如果最后一行的每一列都有内容，删掉最后多余的换行符
        if (count == 0){
            output_string[strlen(output_string) - 1] = '\0';
        }
    }
    else if (cmd_settings.column_format == SINGLECOL && cmd_settings.length_format == LONG){
        sprintf(output_string, "总大小: %s", total_file_size);
        for (int k = 0; k < file_count; ++k) {
            struct file * f = files[k];
            sprintf(output_string, "%s\n%s", output_string, f->output);
            }
        }
}


void init_file(struct file *f) {
    f->output[0] = '\0';
    f->file_name[0] = '\0';
    f->file_mode[0] = '\0';
    f->file_user[0] = '\0';
    f->file_group[0] = '\0';
}








