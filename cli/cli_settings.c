// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#include "common/settings/abstract_settings.h"
#include "common/settings/settings_parser.h"
#include "cli/cli_settings.h"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


static const char* default_path = "./srv.json";


static void default_settings(CliSettings* self){
    printf("Using default settings:\n");
    self->print(self, stdout);
}


static ssize_t parse_cli_settings(void* _self, const char* path){
    CliSettings* self = (CliSettings*) _self;
    char json[max_file_len+1];
    ssize_t r = read_settings(path, json);
    if ( r < 0 ){
        default_settings(self);
        self->write(self, path ? path: default_path, self->print);
        return -1;
    }
    SettingsParser* parser = create_settings_parser();
    if (!parser){
        printf("Error creating parser\n");
        default_settings(self);
        return -1;
    }
    void* obj = parser->parse(json);
    if (obj==NULL){
        printf("Error parsing config: not well formed\n");
        default_settings(self);
        return -1;
    }
    sets(self->id, parser->get_string(obj, "id"));
    void* ep = parser->get(obj, "endpoint");
    if ( !ep ){
        printf("srv.ep key not found\n");
    }else{
        sets(self->ep.host, parser->get_string(ep, "host") );
        self->ep.port  = (int)parser->get_long(ep, "port", NULL);
        self->ep.timeout = parser->get_long(ep, "timeout", NULL);
    }
    self->max_packet_len = (size_t)parser->get_long(obj, "max_packet_len", NULL);
    parser->free( ep );
    parser->free( obj );
    free_settings_parser( parser );
    printf("Read settings:\n");
    self->print(self, stdout );
    return r;
}


static ssize_t print_cli_settings(void* _self, FILE* fd){
    CliSettings* self = (CliSettings*) _self;
    fprintf(fd, "{\n");
    fprintf(fd, "    \"id\": \"%s\",\n", self->id);
    fprintf(fd, "    \"endpoint\":{\n");
    fprintf(fd, "        \"host\": \"%s\",\n", self->ep.host);
    fprintf(fd, "        \"port\": %d,\n", self->ep.port);
    fprintf(fd, "        \"timeout\": %ld\n", self->ep.timeout);
    fprintf(fd, "    },\n");
    fprintf(fd, "    \"max_packet_len\": %lu\n", self->max_packet_len);
    fprintf(fd, "}\n");
    return 1;
}


static void clear_cli_settings(void* _){
}


void init_cli_settings(CliSettings* self){
    sets(self->id, "cli_0");
    sets(self->ep.host, "127.0.0.1");
    self->ep.port = 9000;
    self->ep.timeout = 1000000;
    self->max_packet_len = 2048;
    self->clear = clear_cli_settings;
    self->parse = parse_cli_settings;
    self->print = print_cli_settings;
    self->write = write_settings;
}


