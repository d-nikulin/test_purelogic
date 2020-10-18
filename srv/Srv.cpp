//(С)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#include "common/constants.h"
#include "common/json/bson_parser.h"
#include "common/tlv_protocol/tlv_protocol.h"
#include "srv/srv_settings.h"
#include "srv/Srv.hpp"

// Disable 'sign-conversion' warning produced by included header because this
// kind of warning is not allowed with project configuration (treated as an error)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <bson.h>
#pragma GCC diagnostic pop

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>


extern "C"
ssize_t signal_header(int fd, const char* buf, size_t len){
    HEADER* header = (HEADER*)buf;
    char packet_type = header->packet_type;
    if (packet_type==CONNECT_PACKET){
        instance()->regClient( fd, header->id );
        return 0;
    }
    if (packet_type==REQUEST_PACKET){
        return (ssize_t) ntohs(header->len);
    }
    return -1;
}

extern "C"
ssize_t signal_data(int fd, const char* buf, size_t len){
    instance()->request( fd, buf );
    return (ssize_t) len;
}


extern "C"
int signal_disconnected(int fd){
    instance()->unregClient( fd );
    return 1;
}



Srv::Srv(int argc, const char** argv){
    init_srv_settings( &settings );
    settings.parse(&settings, argc>1 ? argv[1] : "./srv.json");
    initTlvSrvHandlers();
    srv = NULL;
    printf("srv created..\n");
}


Srv::~Srv(){
    settings.clear(&settings);
    printf("destroyed srv\n");
}


int Srv::start(){
    load();
    srv = tlv_srv_start( &shndl );
    if (!srv){
        return -1;
    }
    printf("srv %s started\n", settings.id);
    return 1;
}


int Srv::stop(){
    tlv_srv_stop( srv );
    printf("srv %s stoped.\n", settings.id);
    save();
    return 1;
}


void Srv::initTlvSrvHandlers(){
    ep.host = settings.ep.host;
    ep.port = settings.ep.port ;
    ep.timeout = settings.ep.timeout;
    ep.id = settings.id;
    shndl.ep = &ep;
    shndl.max_packet_len = settings.max_packet_len;
    shndl.header_len = HEADER_LEN;
    shndl.header = signal_header;
    shndl.data = signal_data;
    shndl.disconnected = signal_disconnected;
}


bool Srv::regClient(int fd, const char* id){
    pthread_mutex_lock(&clients_mtx);
    auto i = clients.insert( fd );
    pthread_mutex_unlock(&clients_mtx);
    if (i.second){
        printf("registered %d %s\n", fd, id);
    }
    return i.second;
}


bool Srv::unregClient(int fd){
    pthread_mutex_lock(&clients_mtx);
    clients.erase( fd );
    pthread_mutex_unlock(&clients_mtx);
    printf("unregistered %d\n", fd);
    return true;
}


bool Srv::connected(int fd){
    pthread_mutex_lock(&clients_mtx);
    auto i = clients.find( fd );
    bool res = i!=clients.end();
    pthread_mutex_unlock(&clients_mtx);
    return res;
}


void Srv::request(int fd, const char* data){
    printf("got request: %s\n", data);
    void* bson = parse_bson( data );
    bool found_id;
    size_t id = (size_t)get_long_bson(bson, "id", &found_id);
    const char* method = get_string_bson(bson, "method");
    void* params = get_bson(bson, "params");
    const char* filename = get_string_bson(params, "filename");
    const char* hash = get_string_bson(params, "hash");
    const size_t reply_len = (size_t)(settings.max_packet_len-HEADER_LEN);
    char reply[ reply_len ];
    bzero(reply, sizeof(reply));
    const char* err_pattern = "{\"id\":%lu, \"error\":{\"code\":32000, \"message\":\"%s\"}}";
    const char* res_pattern = "{\"id\":%lu, \"result\":\"%s\"}";
    if (!found_id || !method || !filename || !hash){
        snprintf(reply, sizeof(reply), err_pattern, "error parsing request\n");
    }else{
        MethodId m_id = parse_method( method );
        switch(m_id){
        case Method_Add:
        {
            bool res = add(filename, hash);
            snprintf(reply, sizeof(reply), res ? res_pattern : err_pattern, id, res ? "ok" : "add into collection fault");
            break;
        }
        case Method_Check:
        {
            bool res = check(filename);
            snprintf(reply, sizeof(reply), res_pattern, id, res ? "found" : "not found");
            break;
        }
        case Method_Delete:
        {
            bool res = erase(filename);
            snprintf(reply, sizeof(reply), res ? res_pattern : err_pattern, id, res ? "ok" : "remove from collection fault");
            break;
        }
        case Method_Unknown:
        default:
            snprintf(reply, sizeof(reply), err_pattern, id, "unknown method");
            break;
        }
    }
    response(fd, reply);
    free_bson( params );
    free_bson( bson );
}


bool Srv::add(const char* filename, const char* hash){
    pthread_mutex_lock(&cache_mtx);
    auto i = cache.insert( {std::string(filename), std::string(hash)} );
    pthread_mutex_unlock(&cache_mtx);
    return i.second;
}


bool Srv::check(const char* filename){
    pthread_mutex_lock(&cache_mtx);
    auto i = cache.find( std::string(filename) );
    pthread_mutex_unlock(&cache_mtx);
    return i!=cache.end();
}


bool Srv::erase(const char* filename){
    pthread_mutex_lock(&cache_mtx);
    const auto i = cache.find( std::string(filename) );
    bool res = i != cache.end();
    if (res){
        cache.erase( i );
    }
    pthread_mutex_unlock(&cache_mtx);
    return res;
}


ssize_t Srv::response(int fd, const char* reply){
    char packet[ settings.max_packet_len ];
    bzero(packet, sizeof(packet));
    HEADER* hdr = (HEADER*) packet;
    hdr->packet_type = RESPONSE_PACKET;
    size_t len = strlen(reply);
    hdr->len = htons( (uint16_t) ++len );
    snprintf(packet+HEADER_LEN, len, "%s", reply);
    printf("response: %s\n", packet+HEADER_LEN);
    if (!connected(fd)){
        printf( "error sending response - connection lost\n");
        return -1;
    }
    ssize_t res = tlv_srv_response(fd, packet, HEADER_LEN+(size_t)len);
    if (res==-1){
        printf( "i/o error sending response\n");
    }
    return res;
}


bool Srv::save(){
    bson_t* bson = bson_new();
    char key[32];
    size_t index=0;
    pthread_mutex_lock(&cache_mtx);
    for (auto i = cache.begin(); i!=cache.end(); i++ ){
        bson_t* item = bson_new();
        bson_append_utf8 (item, "filename", -1, i->first.c_str(), -1);
        bson_append_utf8 (item, "hash", -1, i->second.c_str(), -1);
        snprintf(key, sizeof(key), "%lu", index++);
        bson_append_document(bson, key, -1, item);
        bson_destroy( item );
    }
    pthread_mutex_unlock(&cache_mtx);

    char* json = bson_as_json(bson, NULL);
    bson_destroy( bson );
    if (!json){
        printf("error saving hash collection: serialize fault\n");
        return false;
    }
    const char* filename = "./cache_dump.json";
    int fd = open(filename, O_WRONLY|O_CREAT, 0777);
    if (fd<0){
        printf("error saving hash collection (%d) %s\n", errno, strerror(errno));
        bson_free( json );
        return false;
    }
    bool result = true;
    char* pos = json;
    char* end = json + strlen(json);
    while(pos<end){
        int res = write(fd, pos, (size_t)(end-pos));
        if (res<0){
            printf("error saving hash collection (%d) %s\n", errno, strerror(errno));
            break;
        }
        pos += (size_t)res;
    }
    close(fd);
    bson_free( json );
    if (result){
        printf("saved %lu items of hash collection to %s\n", index, filename);
    }
    return result;
}


bool Srv::load(){
    const char* filename = "./cache_dump.json";
    int fd = open(filename, O_RDONLY);
    if (fd<0){
        printf("error loading hash collection (%d) %s\n", errno, strerror(errno));
        return false;
    }
    ssize_t len = lseek(fd, 0, SEEK_END);
    if (len<0){
        printf("error loading hash collection (%d) %s\n", errno, strerror(errno));
        close(fd);
        return false;
    }
    lseek(fd, 0, SEEK_SET);

    char* json = (char*) calloc (1, (size_t)len+1);
    if (!json){
        printf("error loading hash collection (%d) %s\n", errno, strerror(errno));
        close(fd);
        return false;
    }
    char* pos = json;
    char* end = json + len;
    while(pos<end){
        int res = read(fd, pos, (size_t)(end-pos));
        if (res<0){
            printf("error loading hash collection (%d) %s\n", errno, strerror(errno));
            close(fd);
            free(json);
            return false;
        }
        pos += (size_t)res;
    }
    close(fd);

    bson_error_t berr;
    bson_t* bson = bson_new_from_json((const uint8_t*)json, -1, &berr);
    if (!bson){
        printf("error loading hash collection %s\n", berr.message);
        free( json );
        return false;
    }
    free( json );

    size_t index = 0;
    bson_iter_t iter;
    if (!bson_iter_init (&iter, bson)) {
        printf("error loading hash collection\n");
        bson_destroy( bson );
        return false;
    }
    pthread_mutex_lock(&cache_mtx);
    while (bson_iter_next (&iter)) {
        //const char* key = bson_iter_key(&iter);
        const bson_value_t* val = bson_iter_value(&iter);
        if (val && val->value_type==BSON_TYPE_DOCUMENT){
            uint32_t len = 0;
            const uint8_t *params = NULL;
            bson_iter_document(&iter, &len, &params);
            bson_t* item = bson_new_from_data(params, len);
            const char* filename = get_string_bson(item, "filename");
            const char* hash = get_string_bson(item, "hash");
            if (filename && hash){
                auto i = cache.insert( {std::string(filename), std::string(hash)} );
                if (i.second){
                    index++;
                }
            }
            bson_destroy( item );
        }
    }
    pthread_mutex_unlock(&cache_mtx);
    bson_destroy( bson );
    printf("loading %lu items of hash collection to %s\n", index, filename);
    return true;
}


