// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#ifndef TLV_SRV_CONNECTIONS_H_
#define TLV_SRV_CONNECTIONS_H_


#include "common/tlv_transfer/tlv_thread.h"


#ifdef __cplusplus
extern "C" {
#endif


int tlv_srv_reg_connection(int fd, TlvThread* worker);


int tlv_srv_unreg_connection(int fd);


int tlv_srv_unreg_all();


#ifdef __cplusplus
}
#endif


#endif //TLV_SRV_CONNECTIONS_H_
