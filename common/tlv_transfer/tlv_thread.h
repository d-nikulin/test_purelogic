// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#ifndef TLV_THREAD_H_
#define TLV_THREAD_H_


#include <pthread.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct TlvThread TlvThread;

struct TlvThread{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool state;
    bool started;
    void (*wait)(TlvThread* self);
    void (*notify)(TlvThread* self);
    //void (*request)(TlvThread* self, int)
    void* hndl;
    int fd;
};


void init_tlv_thread(TlvThread*, void*);


void destroy_tlv_thread(TlvThread*);


TlvThread* create_tlv_thread( void* hndl );


void free_tlv_thread( TlvThread* self );


#ifdef __cplusplus
}
#endif


#endif //TLV_THREAD_H_
