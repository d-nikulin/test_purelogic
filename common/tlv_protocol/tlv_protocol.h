// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#ifndef PROTOCOL_PARSER_H_
#define PROTOCOL_PARSER_H_

#include "common/constants.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


const char CONNECT_PACKET = 'c';
const char REQUEST_PACKET = 'q';
const char RESPONSE_PACKET = 'p';


typedef struct{
    char packet_type;         // packet_type, see defines
    uint16_t len;             // packet length in bytes, for CONNECT_PACKET is 0
    char id[HASH_SIZE];   // id of requester only for CONNECT_PACKET
}HEADER;


#define HEADER_LEN sizeof(HEADER)


typedef struct{
    char* header;
    char* data;
    size_t h_len;
    size_t d_len;
}TlvMessage;


typedef enum{
    Method_Add = 0,
    Method_Check = 1,
    Method_Delete = 2,
    Method_Unknown = 3
}MethodId;


#ifdef __cplusplus
extern "C" {
#endif


bool parse_packet(const char* packet, size_t len, TlvMessage* msg);


void to_string_message(TlvMessage* msg, char* packet, size_t* len);


void clear_message(TlvMessage* msg);



const char* method_to_string(MethodId id);

MethodId parse_method(const char* method);

#ifdef __cplusplus
}
#endif


#endif //PROTOCOL_PARSER_H_
