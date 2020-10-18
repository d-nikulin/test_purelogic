// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#pragma once

#include "common/settings/abstract_settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct{
     void (*clear)(void* self);

     ssize_t (*parse)(void* self, const char* path);
     ssize_t (*print)(void* self, FILE* fd);
     ssize_t (*write)(void* self, const char* path, ssize_t (*print) (void*, FILE*));

     char id[max_str_len];
     EndpointSett ep;
     size_t max_packet_len;
}CliSettings;


#ifdef __cplusplus
extern "C" {
#endif


void init_cli_settings(CliSettings* self);


#ifdef __cplusplus
}
#endif
