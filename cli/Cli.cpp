//(С)2020, Никулин Д.А., dan-gubkin@mail.ru

#include "common/constants.h"
#include "common/tlv_protocol/tlv_protocol.h"
#include "cli/cli_settings.h"
#include "cli/Cli.hpp"

#include <openssl/sha.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netinet/in.h>



extern "C"
ssize_t signal_header(const char* id, const char* buf, size_t len){
    HEADER* header = (HEADER*)buf;
    return header->packet_type == RESPONSE_PACKET ? (ssize_t) ntohs(header->len) : -1;
}


extern "C"
ssize_t signal_data(const char* id, const char* buf, size_t len){
    instance()->response( buf );
    return (ssize_t)len;
}


extern "C"
int signal_connected(const char* id){
    instance()->setConnected( );
    return 1;
}


extern "C"
int signal_disconnected(const char* id){
    instance()->setDisconnected( );
    return 1;
}


Cli::Cli(int argc, const char** argv){
    init_cli_settings( &settings );
    settings.parse(&settings, argc>1 ? argv[1] : "./cli.json");
    initTlvCliHandlers();
    cli = NULL;
    connected = false;
    request_id = 0;
    printf("cli created..\n");
}


Cli::~Cli(){
    settings.clear(&settings);
    printf("destroyed cli\n");
}


int Cli::start(){
    cli = tlv_cli_start( &chndl );
    if (!cli){
        return -1;
    }
    printf("cli %s started\n", settings.id);
    return 1;
}


int Cli::stop(){
    tlv_cli_stop( cli );
    connected = false;
    printf("cli %s stoped.\n", settings.id);
    return 1;
}


void Cli::initTlvCliHandlers(){
    ep.host = settings.ep.host;
    ep.port = settings.ep.port ;
    ep.timeout = settings.ep.timeout;
    ep.id = settings.id;
    chndl.ep = &ep;
    chndl.max_packet_len = settings.max_packet_len;
    chndl.header_len = HEADER_LEN;
    chndl.header = signal_header;
    chndl.data = signal_data;
    chndl.connected = signal_connected;
    chndl.disconnected = signal_disconnected;
}


void Cli::setConnected(){
    char hdr[HEADER_LEN];
    bzero(hdr, sizeof(hdr));
    HEADER* h = (HEADER*)hdr;
    h->packet_type = CONNECT_PACKET;
    h->len = 0;
    snprintf( h->id, sizeof( h->id ), "%s", settings.id);
    if ( tlv_cli_request(cli, hdr, HEADER_LEN) ==-1){
        printf("error sending CONNECT_PACKET\n");
        return;
    }
    connected = true;
    printf("%s connected\n", settings.id);
}


void Cli::setDisconnected(){
    connected = false;
    printf("%s disconnected\n", settings.id);
}


ssize_t Cli::hash(const char* filename, char* hash){
    int fd = open(filename, O_RDONLY);
    if (fd<0){
        printf("error opening file %s: (%d) %s\n", filename, errno, strerror(errno));
        return -1;
    }
    char data[1024];
    size_t len = 0;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    while ( ssize_t res = read(fd, data, sizeof(data) ) ){
        if (res<0){
            close(fd);
            printf("error reading file %s: (%d) %s\n", filename, errno, strerror(errno));
            return res;
        }
        SHA256_Update(&sha256, data, (size_t)res);
        len += (size_t) res;
        //
    }
    close(fd);
    unsigned char buf[SHA256_DIGEST_LENGTH];
    SHA256_Final((unsigned char*)buf, &sha256);
    for(size_t i = 0; i < SHA256_DIGEST_LENGTH; i++){
        sprintf(hash+i*2, "%02x", buf[i]);
    }
    return (ssize_t) len;
}


ssize_t Cli::request(const char* method, const char* filename){
    MethodId m_id = parse_method( method );
    if (m_id==Method_Unknown){
        printf("unknown method %s\n", method);
        return -1;
    }
    char buf[2*SHA256_DIGEST_LENGTH+1];
    bzero(buf, sizeof(buf));
    if (m_id==Method_Add){
        if ( hash(filename, buf)==-1 ){
            return -1;
        }
    }
    const char* pattern = "{\"id\":%lu, \"method\":\"%s\", \"params\":{\"filename\":\"%s\", \"hash\":\"%s\"}}";
    char packet[ settings.max_packet_len ];
    bzero(packet, settings.max_packet_len);
    HEADER* hdr = (HEADER*) packet;
    hdr->packet_type = REQUEST_PACKET;
    //snprintf(hdr->id, sizeof(hdr->id), "%s", settings.id); no-need for req
    int len = snprintf(packet+HEADER_LEN, sizeof(packet)-HEADER_LEN, pattern, request_id++, method, filename, buf);
    if (len<0){
        printf("i/o error: (%d) %s\n", errno, strerror(errno));
        return -1;
    }
    hdr->len = htons( (uint16_t) ++len );
    printf("request: %s\n", packet+HEADER_LEN);
    if (!connected){
        printf("cli not connected: request hasn't been sent\n");
        return -1;
    }
    ssize_t res = tlv_cli_request(cli, packet, HEADER_LEN+(size_t)len);
    if (res==-1){
        printf("error sending request\n");
    }
    return res;
}


void Cli::response(const char* data){
    printf("response: %s\n", data);
}
