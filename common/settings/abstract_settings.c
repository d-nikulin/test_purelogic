// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#include "common/settings/abstract_settings.h"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>



int sets(char* dst, const char* src){
    return (!src || !dst) ? -1: snprintf(dst, max_str_len, "%s", src);
}


ssize_t read_settings(const char* path, char* settings){
    if (!path){
        return -1;
    }
    int fd = open(path, O_RDONLY);
    if ( fd < 0 ){
        printf("Ошибка открытия конфигурационного файла %s: (%d) %s\n", path, errno, strerror(errno));
        return -1;
    }
    size_t len = max_file_len;
    ssize_t res = 0;
    char* pos = (char*) settings;
    do{
        ssize_t r = read(fd, pos, len);
        if (r==0){ //eof
            break;
        }
        if (r==-1){
            printf("Ошибка чтения конфигурационного файла %s: (%d) %s\n", path, errno, strerror(errno));
            res = -1;
        }
        pos += (size_t) r;
        len -= (size_t) r;
    }while( len>0 );
    close(fd);
    *pos = '\0';
    return res == -1 ? -1 : pos-settings;
}


ssize_t write_settings(void* self, const char* path, ssize_t (*print)(void*, FILE* ) ){
    FILE *fd = fopen(path, "w");
    if ( !fd ){
        printf("Ошибка открытия %s: (%d) %s", path, errno, strerror(errno));
        return -1;
    }
    print( self, fd );
    fclose(fd);
    return 1;
}
