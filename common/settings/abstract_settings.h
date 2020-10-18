// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#ifndef ABSTRACT_SETTINGS_H_
#define ABSTRACT_SETTINGS_H_

#include "common/constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define max_str_len HASH_SIZE
#define max_file_len 16*1024 // максимальная длина конф. файла


typedef struct{
    char host[max_str_len];   // max ipv4 len
    int port;
    long timeout;
}EndpointSett;


typedef struct{
    char prefix[max_str_len];
    char host[max_str_len];
    int port;
    char dbname[max_str_len];
}DBSett;



int sets(char* dst, const char* src);

ssize_t read_settings(const char* path, char* settings);

ssize_t write_settings(void*, const char* path, ssize_t (*print) (void*, FILE*));


#endif //ABSTRACT_SETTINGS_H_
