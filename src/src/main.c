#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "server.h"
#include "globals.h"

#include "csapp.h"
#include "helper.h"

int signal_set=0;

static void terminate(int);

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    char * port;
    int poption=0;

    if (argc<=1){
        /* No argument given   */
        printf("No argument given");
        exit(0);
    }
    else if(argc>1){
        int counter=argc;
        int i=1;
        while(i!=counter){
            if(strcmp(*(argv+i),"-p")==0){
                if(i+1==counter){
                    // value not given
                    printf("Port num not provided");
                    exit(0);
                }
                else{
                    // value given retrieve port num
                    port=*(argv+i+1);
                    //port=strtol(p,&p,10);
                    poption=1;
                    i++;
                }
            }
            else{
                printf("invalid option");
                exit(0);
            }
            i++;
        }
        if(poption==0){
            //port not specified
            printf("Port option not specified");
            exit(0);
        }
    }
    printf("port num server %s\n",port);

    // Perform required initializations of the client_registry and
    // player_registry.
    user_registry = ureg_init();
    client_registry = creg_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    int server_socket = open_listenfd(port);

    int * connection_socket;
    socklen_t clientlen;
    struct sockaddr_storage connection_addr;
    pthread_t id;

    char client_hostname[100];
    char client_port[100];

    struct sigaction sigact;
    sigact.sa_handler=sigaction_handler;
    sigact.sa_flags=0;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGHUP,&sigact,0);


    while(1){

        clientlen=sizeof(struct sockaddr_storage);
        connection_socket= malloc(sizeof(int));
        *connection_socket=accept(server_socket,(SA*)&connection_addr,&clientlen);
        if(signal_set!=0){
            break;
        }
        Getnameinfo((SA*)&connection_addr,clientlen,client_hostname,100,client_port,100,0);
        fprintf(stdout,"connected to client on host %s and port %s\n",client_hostname,client_port);

        Pthread_create(&id,NULL,chla_client_service,connection_socket);

    }

    //fprintf(stderr, "You have to finish implementing main() "
	   // "before the server will function.\n");

    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}

void sigaction_handler(){
    printf("inside sighup handler\n");
    signal_set=1;
}