// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#ifndef TLV_CLI_H_
#define TLV_CLI_H_


#include "tlv_types.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>


typedef struct{
    TlvEndpoint* ep;
    size_t max_packet_len;
    size_t header_len;
    int (*connected)(const char* id);
    int (*disconnected)(const char* id);
    ssize_t (*header)(const char* id, const char* packet, size_t len);
    ssize_t (*data)(const char* id, const char* packet, size_t len);
}TlvCliHandlers;


#ifdef __cplusplus
extern "C" {
#endif


void* tlv_cli_start(const TlvCliHandlers* hndl);


int tlv_cli_stop(void*);


ssize_t tlv_cli_request(void*, const char* packet, size_t len);


#ifdef __cplusplus
}
#endif


#endif //TLV_CLI_H_
