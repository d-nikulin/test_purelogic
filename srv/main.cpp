// (C)2020, Никулин Д.А., dan-gubkin@mail.ru

#include <srv/Srv.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


static Srv* srv = NULL;


Srv* instance(){
    return srv;
}


extern "C"
int finish(int _){
    printf("Stoppig srv ...\n");
    if ( srv ){
        srv->stop();
        delete srv;
        srv = NULL;
    }
    exit(0);

}

void signal_handler(int sig){
    printf("srv got %d signal\n", sig);
    if (sig==SIGTERM || sig == SIGINT){
        finish( sig );
    }
}


int main(int argc, const char** argv){
    setbuf(stdout, 0);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    srv = new Srv(argc, argv);
    if (srv->start()<0){
        printf("error startig srv, exiting..\n");
        srv->stop();
        delete srv;
        return -1;
    }

    char str[126];
    do{
        if (scanf("%s",str)>0){
            if (strncmp("exit", str, 4)==0) break;
        }
    }while(true);

    finish(0);

    return 0;
}
