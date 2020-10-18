// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#pragma once

#include "common/tlv_protocol/tlv_protocol.h"
#include "common/tlv_transfer/tlv_cli.h"
#include "cli/cli_settings.h"


class Cli{

public:
    Cli(int argc, const char** argv);
    ~Cli();

    int start();
    int stop();

    bool regClient(int fd);
    bool unregClient(int fd);

    void setConnected();
    void setDisconnected();
    void response(const char* data);
    ssize_t request(const char* method, const char* filename);


private:
    CliSettings settings;
    TlvEndpoint ep;
    TlvCliHandlers chndl;
    void* cli;
    bool connected;
    unsigned request_id;

    void initTlvCliHandlers();
    ssize_t hash(const char* filename, char* hash);
};


Cli* instance();
int finish();

