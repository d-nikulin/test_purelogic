// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#include "tlv_cli.h"
#include "tlv_thread.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>


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



static void* tlv_cli_worker(void* args){
    TlvThread* worker = (TlvThread*) args;
    TlvCliHandlers* hndl = (TlvCliHandlers*) worker->hndl;

    int epfd = epoll_create(1);
    if (epfd==-1){
        printf("tlv_cli error creating epoll (%d) %s", errno, strerror(errno) );
        pthread_exit(NULL);
        return (void*)0;
    }

    TlvEndpoint* ep = hndl->ep;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons( ep->port );
    if(inet_pton(AF_INET, ep->host , &serv_addr.sin_addr)<=0){
        printf("tlv_cli inet_pton error: (%d) %s\n", errno, strerror(errno));
        close(epfd);
        pthread_exit(NULL);
        return (void*)0;
    }

    worker->started = true;
    worker->notify(worker);
    const size_t MAXEVENTS = 5;
    const int epoll_timeout = hndl->ep->timeout/1000; // micro -> mili
    const size_t packet_len = hndl->max_packet_len;
    while ( worker->started ){
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0 ){
            printf("tlv_cli error creating socket: (%d) %s", errno, strerror(errno));
            close(epfd);
            worker->started = false;
            pthread_exit(NULL);
            return (void*)0;
        }
        bool connected = false;
        while ( worker->started ){
            int res = connect( fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            if (res<0 ){
                if (errno!=ECONNREFUSED){
                    printf("tlv_cli error connecting: (%d) %s", errno, strerror(errno));
                    close(fd);
                    close(epfd);
                    worker->started = false;
                    pthread_exit(NULL);
                    return (void*)0;
                }
                usleep( ep->timeout );
            }
            if ( res == 0 ){
                struct epoll_event ev;
                ev.events = EPOLLHUP|EPOLLRDHUP|EPOLLIN;
                ev.data.fd = fd;
                epoll_ctl( epfd, EPOLL_CTL_ADD, fd, &ev); //todo:
                worker->fd = fd;
                hndl->connected( ep->id );
                connected = true;
                break;
            }
            printf("tlv_cli connecting to %s:%d failed, reason: (%d) %s\n",
                    hndl->ep->host, hndl->ep->port, errno, strerror(errno));
        }
        while ( worker->started && connected){
            struct epoll_event events[MAXEVENTS];
            int nfds = epoll_wait(epfd, events, MAXEVENTS, epoll_timeout);
            if ( nfds==-1 ){
                continue;
            }
            for (int j=0; j<nfds; j++){
                if ( (events[j].events&EPOLLHUP) == EPOLLHUP
                        ||(events[j].events&EPOLLRDHUP) == EPOLLRDHUP ){
                    printf("hup event %d\n", events[j].events);
                    worker->fd = -1;
                    connected = false;
                    hndl->disconnected( ep->id );
                    struct epoll_event ev;
                    ev.events = EPOLLHUP|EPOLLRDHUP|EPOLLIN;
                    ev.data.fd = events[j].data.fd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[j].data.fd, &ev);
                    close( events[j].data.fd );
                    usleep( ep->timeout );
                }else if ((events[j].events&EPOLLIN) == EPOLLIN){
                    char buf[packet_len];
                    ssize_t len = read_socket(events[j].data.fd, buf, hndl->header_len);
                    ssize_t d_len = -1;
                    if ( len > 0 && (size_t)len == hndl->header_len){

                        d_len = hndl->header(ep->id, buf, (size_t)len);
                    }
                    if (d_len >0 && (size_t)d_len+hndl->header_len<=hndl->max_packet_len){
                        len = read_socket(events[j].data.fd, buf+hndl->header_len, (size_t)d_len);
                        if ( len > 0 && len == d_len){
                            hndl->data(ep->id, buf+hndl->header_len, (size_t)len);
                        }
                    }
                //}else if ((events[j].events&EPOLLOUT) == EPOLLOUT){
                //    printf("out event %d\n", events[j].events);
                }
            }
        }
    }
    close( epfd );
    worker->notify(worker);
    pthread_exit(NULL);
    return (void*)0;
}


void* tlv_cli_start(const TlvCliHandlers* hndl){
    TlvThread* worker = create_tlv_thread( (void*)hndl );
    if ( !worker ){
        printf("tlv_cli creating error\n");
        return NULL;
    }
    pthread_t thread;
    pthread_create(&thread, NULL, tlv_cli_worker, worker);
    worker->wait( worker );
    if ( !worker->started ){
        printf("tlv_cli don't started.\n");
        free_tlv_thread( worker );
        return NULL;
    }
    printf("tlv_cli started..\n");
    pthread_detach( thread );
    return worker;
}


int tlv_cli_stop(void* _worker){
    TlvThread* worker = (TlvThread*) _worker;
    if (!worker){
        return -1;
    }
    if (worker->started){
        worker->started = false;
        worker->wait( worker );
        printf("tlv cli stopped ..\n");
    }
    free_tlv_thread( worker );
    return 1;
}


ssize_t tlv_cli_request(void* _worker, const char* buf, size_t len){
    TlvThread* worker = (TlvThread*) _worker;
    if (!worker || worker->fd==-1){
        return -1;
    }
    return write_socket(worker->fd, buf, len);
}
