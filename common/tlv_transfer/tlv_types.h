// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#ifndef TLV_TYPES_H_
#define TLV_TYPES_H_

#include <stdio.h>


typedef struct{
    const char* id;    // id for tlv_server container
    const char* host;  // ipv4 for server socket
    int port;          // port for server socket
    long timeout;      // timeout for socket operations (microseconds)
}TlvEndpoint;


#endif //TLV_TYPES_H_
