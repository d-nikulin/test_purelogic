// (C)2020, Никулин Д.А., d.nikulin@sk-shd.ru

#include <cli/Cli.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


static Cli* cli = NULL;


Cli* instance(){
    return cli;
}


extern "C"
int finish(int _){
    if (!cli){
        return -1;
    }
    printf("Stoppig cli ...\n");
    cli->stop();
    delete cli;
    cli = NULL;
    exit(0);

}

void signal_handler(int sig){
    printf("cli got %d signal\n", sig);
    if (sig==SIGTERM || sig == SIGINT){
        finish( sig );
    }
}


int main(int argc, const char** argv){
    setbuf(stdout, 0);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    cli = new Cli(argc, argv);
    if (cli->start()<0){
        printf("error startig srv, exiting..\n");
        cli->stop();
        delete cli;
        return -1;
    }
    printf("\n\nexit: Ctrl-C or exit exit\nusage: <method> <filename>\nexample:\nadd ./cli.json\n\n");

    char method[256];
    char filename[256];
    do{
        if ( scanf("%s %s", method, filename) > 0 ){
            if (strncmp("exit", method, 4)==0){
                break;
            }
            if ( strlen(method) && strlen(filename) ){
                cli->request(method, filename);
            }
        }
    }while(true);

    finish(0);

    return 0;
}
