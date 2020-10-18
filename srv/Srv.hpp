// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#pragma once

#include "common/tlv_protocol/tlv_protocol.h"
#include "common/tlv_transfer/tlv_srv.h"
#include "srv/srv_settings.h"

#include <set>
#include <map>
#include <string>


class Srv{

public:
    Srv(int argc, const char** argv);
    ~Srv();

    int start();
    int stop();

    bool regClient(int fd, const char* id);
    bool unregClient(int fd);
    void request(int fd, const char* data);

private:
    SrvSettings settings;
    TlvEndpoint ep;
    TlvSrvHandlers shndl;
    void* srv;
    std::set<int> clients;
    pthread_mutex_t clients_mtx = PTHREAD_MUTEX_INITIALIZER;
    std::map<std::string, std::string> cache;
    pthread_mutex_t cache_mtx = PTHREAD_MUTEX_INITIALIZER;

    void initTlvSrvHandlers();
    bool connected(int fd);
    bool add(const char* filename, const char* hash);
    bool check(const char* filename);
    bool erase(const char* filename);
    ssize_t response(int fd, const char* reply);
    bool save();
    bool load();

};


Srv* instance();
int finish();

