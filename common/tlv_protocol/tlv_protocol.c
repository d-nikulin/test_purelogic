// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#include "common/tlv_protocol/tlv_protocol.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>


static const char* methods[] = { "ADD", "CHECK", "DELETE", "UNKNOWN"};


const char* method_to_string(MethodId id){
    return id<0 ? NULL : methods[id];
}


MethodId parse_method(const char* method){
    for (MethodId i=Method_Add; i<Method_Unknown; i++){
        if (strncasecmp(methods[i], method, strlen(method))==0 ){
            return (MethodId)i;
        }
    }
    return Method_Unknown;
}



static ssize_t check_packet(const char* packet, size_t len){
    if (len < HEADER_LEN){
        return -1;
    }
    HEADER* hdr = (HEADER*) packet;
    switch (hdr->packet_type){
        case CONNECT_PACKET:
        case REQUEST_PACKET:
        case RESPONSE_PACKET:
        {
            uint16_t mlen = ntohs( hdr->len );
            return (len == mlen + HEADER_LEN) ? (ssize_t)mlen : -1;
        }
        default:
            return -1;
    }
    return -1;
}


bool parse_packet(const char* packet, size_t len, TlvMessage* msg){
    if ( !check_packet(packet, len) ){
        return false;
    }
    char* target = (char*) malloc( len );
    if (!target){
        return false;
    }
    memcpy(target, packet, len);
    msg->header = target;
    msg->data = target + HEADER_LEN;
    msg->h_len = HEADER_LEN;
    msg->d_len = ntohs( ((HEADER*)msg->header)->len );
    return true;
}


void to_string_message(TlvMessage* msg, char* packet, size_t* len){
    memcpy(msg->header, packet, msg->h_len+msg->d_len);
}


void clear_message(TlvMessage* msg){
    if (msg){
        free( msg->header );
    }
}


