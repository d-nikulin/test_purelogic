// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#include "tlv_srv.h"
#include "tlv_srv_connections.h"
#include "tlv_thread.h"


#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>


static inline bool crit_err(int err){
    return ! ((err==EAGAIN) | (err==EWOULDBLOCK) );
}


static inline ssize_t read_socket(int fd, char* buf, size_t len){
    char* pos = buf;
    const char* end = buf + len;
    while(pos<end){
        ssize_t res = read(fd, pos, (size_t)(end-pos));
        if (res<0){
            return res;
        }else if (res==0){
            return pos-buf;
        }
        pos += (size_t) res;
    }
    return (ssize_t) len;
}


static inline ssize_t write_socket(int fd, const char* buf, size_t len){
    char* pos = (char*)buf;
    const char* end = buf + len;
    while(pos<end){
        ssize_t res = write(fd, pos, (size_t)(end-pos));
        if (res<0){
            return res;
        }else if (res==0){
            return pos-buf;
        }
        pos += (size_t) res;
    }
    return (ssize_t) len;
}


static void* tlv_worker(void* _worker){
    TlvThread* worker = (TlvThread*) _worker;
    TlvSrvHandlers* hndl = (TlvSrvHandlers*) worker->hndl;

    char buf[hndl->max_packet_len];
    worker->started = true;
    worker->notify(worker);
    while ( worker->started ){
        ssize_t len = read_socket( worker->fd, buf, hndl->header_len);
        if (len==0 || (len<0 && crit_err(errno)) ){
            break;
        }
        if ( len != (ssize_t)hndl->header_len){
            continue;
        }
        ssize_t d_len = hndl->header(worker->fd, buf, (size_t)len);
        if (d_len<=0  || (size_t)d_len+hndl->header_len > hndl->max_packet_len){
            continue;
        }
        len = read_socket( worker->fd, buf+hndl->header_len, (size_t)d_len);
        if (len==0 || (len<0 && crit_err(errno)) ){
            break;
        }
        if (len == d_len){
            hndl->data(worker->fd, buf+hndl->header_len, (size_t)d_len);
        }

    }
    hndl->disconnected( worker->fd );
    close( worker->fd );
    if (worker->started){ //it means connection lost or crit err
        if (tlv_srv_unreg_connection( worker->fd )>0){
            free_tlv_thread(worker);
        }
        pthread_exit(NULL);
        return (void*)0;
    }
    // it means stop
    printf("tvl_srv: canceled worker %d\n", worker->fd);
    worker->notify(worker);
    pthread_exit(NULL);
    return (void*)0;
}


static int open_srv_socket(const char* host, int port){
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd == -1 ){
        printf("tlv_srv socket error %d: %s\n", errno, strerror(errno));
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, host, &serv_addr.sin_addr)<=0){
        printf("tlv_srv inet_pton error: %d %s\n", errno, strerror(errno));
        return -1;
    }
    serv_addr.sin_port = htons( port );
    {
        int flags = fcntl(fd, F_GETFD);
        if (flags == -1) {
            printf("tlv_srv error retrieving flags from listen socket %d: %s\n", errno, strerror(errno));
            return -1;
        }
        flags |= FD_CLOEXEC;
        if (fcntl(fd, F_SETFD, flags) == -1) {
            printf("tlv_srv error setting O_CLOEXEC to listen socket %d: %s", errno, strerror(errno));
        }
        int optval = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0) {
           printf("tlv_srv error setting socket options %d: %s", errno, strerror(errno));
        }
    }
    int b = bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if ( b < 0 ){
        printf("tlv_srv bind error %d: %s\n", errno, strerror(errno));
        return b;
    }
    //printf("srv socket opened %s:%d\n", host, port);
    return fd;
}


static void* tlv_server(void* _srv){
    TlvThread* srv = (TlvThread*) _srv;
    TlvSrvHandlers* hndl = (TlvSrvHandlers*) srv->hndl;

    int fd = open_srv_socket(hndl->ep->host, hndl->ep->port);
    if ( fd == -1){
        srv->notify( srv );
        pthread_exit(NULL);
        return (void*)0;
    }

    const struct timeval t = {.tv_sec=hndl->ep->timeout/1000000, .tv_usec=hndl->ep->timeout%1000000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval));

    srv->started = true;
    printf("tlv_server %s %s:%d started fd=%d\n", hndl->ep->id, hndl->ep->host, hndl->ep->port, fd);
    srv->notify( srv );
    while ( srv->started ){
        int l = listen(fd, 10);
        if ( l < 0 ){
            printf("tlv_srv listen error %d: %s\n", errno, strerror(errno));
            continue;
        }
        int cli_fd = accept(fd, (struct sockaddr*)NULL, NULL);
        if ( cli_fd < 0 ){
            if (crit_err(errno)){
                printf("tlv_srv accept error %d: %s\n", errno, strerror(errno));
            }
            continue;
        }
        setsockopt(cli_fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval));
        int flags = fcntl(cli_fd, F_GETFD);
        if (flags == -1) {
            printf("tlv_srv error Retrieving flags from connection\n");
            close(cli_fd);
            continue;
        }
        flags |= FD_CLOEXEC;
        if (fcntl(cli_fd, F_SETFD, flags) == -1) {
            printf("tlv_srv error Setting FD_CLOEXEC to connection\n");
            close(cli_fd);
            continue;
        }

        TlvThread* worker = create_tlv_thread( hndl );
        if (!worker){
            printf("connection %d rejected, internal error\n", cli_fd);
            close(cli_fd);
            continue;
        }
        worker->fd = cli_fd;
        pthread_t thread;
        pthread_create(&thread, NULL, tlv_worker, worker);
        worker->wait( worker );
        if (!worker->started){
            printf("connection %d rejected, internal error\n", cli_fd);
            free_tlv_thread( worker );
            close( cli_fd );
            continue;
        }
        tlv_srv_reg_connection(cli_fd, worker);
        pthread_detach( thread );
        //reg
    }
    tlv_srv_unreg_all();
    shutdown(fd, SHUT_RDWR);
    close( fd );

    srv->notify(srv);
    printf("tlv_server %s stopped.\n", hndl->ep->id);
    pthread_exit(NULL);
    return (void*)0;
}


void* tlv_srv_start(const TlvSrvHandlers* hndl){
    TlvThread* srv = create_tlv_thread( (void*)hndl );
    if (!srv){
        printf("tlv_srv creating error\n");
        return NULL;
    }
    pthread_t thread;
    pthread_create(&thread, NULL, tlv_server, srv);
    srv->wait( srv );
    if ( !srv->started ){
        printf("tlv_srv %s don't started.\n", hndl->ep->id);
        free_tlv_thread( srv );
        return NULL;
    }
    pthread_detach( thread );
    return srv;
}


int tlv_srv_stop(void* _srv){
    TlvThread* srv = (TlvThread*) _srv;
    if (!srv){
        return -1;
    }
    if ( srv->started ){
        srv->started = false;
        srv->wait( srv );
    }
    free_tlv_thread( srv );
    return 1;
}


ssize_t tlv_srv_response(int fd, const char* buf, size_t len){
    return write_socket(fd, buf, len);
}
