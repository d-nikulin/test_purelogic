// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#include "common/tlv_transfer/tlv_thread.h"

#include <pthread.h>
#include <stdlib.h>

/*
 *
 */
static void wait(TlvThread* self){
    if (!self || self->state){
        return;
    }
    pthread_mutex_lock( &self->mutex );
    self->state = true;
    while ( self->state ){
        pthread_cond_wait( &self->cond, &self->mutex);
        self->state = false;
    }
    pthread_mutex_unlock( &self->mutex );
}


/*
 *
 */
static void notify(TlvThread* self){
    if (!self){
        return;
    }
    bool f = true;
    while (f){
        pthread_mutex_lock( &self->mutex );
        if ( self->state ){
            pthread_cond_signal(&self->cond);
            f = false;
        }
        pthread_mutex_unlock( &self->mutex );
    }
}


void init_tlv_thread(TlvThread* self, void* hndl){
    if (!self){
        return;
    }
    pthread_mutex_init ( &self->mutex, NULL);
    pthread_cond_init ( &self->cond, NULL);
    self->state = false;
    self->started = false;
    self->wait = wait;
    self->notify = notify;
    self->hndl = hndl;
    self->fd = -1;
}


void destroy_tlv_thread(TlvThread* self){
    if (!self){
        return;
    }
    pthread_mutex_destroy ( &self->mutex );
    pthread_cond_destroy ( &self->cond );
    self->state = false;
    self->started = false;
    self->wait = wait;
    self->notify = notify;
    self->hndl = NULL;
    self->fd = -1;
}


TlvThread* create_tlv_thread( void* hndl ){
    TlvThread* self = (TlvThread*) malloc ( sizeof(TlvThread) );
    init_tlv_thread( self, hndl );
    return self;
}


void free_tlv_thread( TlvThread* self ){
    destroy_tlv_thread( self );
    free( self );
}
