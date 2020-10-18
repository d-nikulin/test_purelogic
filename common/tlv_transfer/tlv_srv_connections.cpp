// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#include "common/tlv_transfer/tlv_srv_connections.h"
#include "common/tlv_transfer/tlv_thread.h"

#include "pthread.h"
#include <map>


static std::map<int, TlvThread*> pool;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static bool exiting = false;


int tlv_srv_reg_connection(int fd, TlvThread* con){
    pthread_mutex_lock(&mtx);
    const auto res = pool.insert( {fd, con} );
    pthread_mutex_unlock(&mtx);
    return res.second ? 1 : -1;
}


int tlv_srv_unreg_connection(int fd){
    if (exiting){
        return -1;
    }
    pthread_mutex_lock(&mtx);
    int res=-1;
    auto i = pool.find( fd );
    if ( i != pool.end() ){
        pool.erase(i);
        res = 1;
    }
    pthread_mutex_unlock(&mtx);
    return res;
}


int tlv_srv_unreg_all(){
    exiting = true;
    pthread_mutex_lock(&mtx);
    while (pool.size()>0){
        for (auto i = pool.begin(); i!=pool.end(); i++){
            TlvThread* worker = (*i).second;
            if (worker->started){
                worker->started = false;
                worker->wait(worker);
            }
            pool.erase(i);
            destroy_tlv_thread(worker);
            free(worker);
            break;
        }
    }
    pthread_mutex_unlock(&mtx);
    exiting = false;
    return 1;
}
