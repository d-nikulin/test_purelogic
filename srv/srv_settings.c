// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#include "common/settings/abstract_settings.h"
#include "common/settings/settings_parser.h"
#include "srv_settings.h"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


static const char* default_path = "./srv.json";


static void default_settings(SrvSettings* self){
    printf("Using default settings:\n");
    self->print(self, stdout);
}


ssize_t parse_srv_settings(void* _self, const char* path){
    SrvSettings* self = (SrvSettings*) _self;
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
    sets(self->dump_name, parser->get_string(obj, "dump_name"));
    parser->free( ep );
    parser->free( obj );
    free_settings_parser( parser );
    printf("Read settings:\n");
    self->print(self, stdout );
    return r;
}


ssize_t print_srv_settings(void* _self, FILE* fd){
    SrvSettings* self = (SrvSettings*) _self;
    fprintf(fd, "{\n");
    fprintf(fd, "    \"id\": \"%s\",\n", self->id);
    fprintf(fd, "    \"endpoint\":{\n");
    fprintf(fd, "        \"host\": \"%s\",\n", self->ep.host);
    fprintf(fd, "        \"port\": %d,\n", self->ep.port);
    fprintf(fd, "        \"timeout\": %ld\n", self->ep.timeout);
    fprintf(fd, "    },\n");
    fprintf(fd, "    \"max_packet_len\": %lu,\n", self->max_packet_len);
    fprintf(fd, "    \"dump_name\": %s\n", self->dump_name);
    fprintf(fd, "}\n");
    return 1;
}


static void clear_srv_settings(void* _){
}


void init_srv_settings(SrvSettings* self){
    sets(self->id, "srv_0");
    sets(self->ep.host, "0.0.0.0");
    self->ep.port = 9000;
    self->ep.timeout = 1000000;
    self->max_packet_len = 2048;
    sets(self->dump_name, "./hash_dump.json");

    self->clear = clear_srv_settings;
    self->parse = parse_srv_settings;
    self->print = print_srv_settings;
    self->write = write_settings;
}


