// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#ifndef TLV_SRV_H_
#define TLV_SRV_H_

#include "tlv_types.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


typedef struct{
    TlvEndpoint* ep;
    size_t max_packet_len;
    size_t header_len;
    int (*disconnected)(int fd);
    ssize_t (*header)(int fd, const char* buf, size_t len);
    ssize_t (*data)(int fd, const char* buf, size_t len);
} TlvSrvHandlers;


#ifdef __cplusplus
extern "C" {
#endif


void* tlv_srv_start(const TlvSrvHandlers*);


int tlv_srv_stop(void*);

ssize_t tlv_srv_response(int fd, const char* buf, size_t len);


#ifdef __cplusplus
}
#endif


#endif //TLV_SRV_H_
