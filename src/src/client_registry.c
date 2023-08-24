#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "client_registry.h"
#include "client.h"
#include "globals.h"

typedef struct client_registry{
    CLIENT * registry [MAX_CLIENTS];
    int count;
    pthread_mutex_t lock;
    pthread_mutex_t lock2;
}CLIENT_REGISTRY;


CLIENT_REGISTRY *creg_init(){
    // create an array of clients
    printf("Initialize client registry\n");
    client_registry = (CLIENT_REGISTRY *) calloc(1,sizeof(CLIENT_REGISTRY));
    pthread_mutex_init(&(client_registry->lock), NULL);
    pthread_mutex_init(&(client_registry->lock2), NULL);
    client_registry->count=0;
    client_registry->registry[0]=NULL;
    return client_registry;

}


void creg_fini(CLIENT_REGISTRY *cr){
    // free the array of clients
    printf("Finalize client registry\n");
    pthread_mutex_lock(&(cr->lock)); ////
    // for(int i=0;i<MAX_CLIENTS;i++){
    //     //unref the client
    //     if(cr->registry[i]!=NULL){
    //     client_unref(cr->registry[i],"removing reference from clients list");    //this gives valgrind errors i guess its not need if unregister is called
    //     }
    // }
    pthread_mutex_unlock(&(cr->lock));
    pthread_mutex_destroy(&(cr->lock));
    pthread_mutex_destroy(&(cr->lock2));
    free(client_registry);
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd){

    pthread_mutex_lock(&(cr->lock));

    CLIENT * ptr=cr->registry[0];

    int i=0;
    while(ptr!=NULL){
        i++;
        ptr=cr->registry[i];
    }
    cr->registry[i]=client_create(cr,fd);
    if(cr->registry[i]==NULL){
        pthread_mutex_unlock(&(cr->lock));
        return NULL;
    }
    else{
    cr->count=(cr->count)+1;
    client_ref(cr->registry[i]," returning pointer of client from registry");
    pthread_mutex_unlock(&(cr->lock));
    return cr->registry[i];
    }

    return NULL;
}


int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client){

    pthread_mutex_lock(&(cr->lock));

    for(int i=0; i<cr->count;i++){
        if(cr->registry[i]==client){
            // remove the client and shift the elements after that
            client_unref(client," removing from client list ");
            for(int j=i;j<cr->count;j++){
                cr->registry[j]=cr->registry[j+1];
            }
            cr->registry[(cr->count)-1]=NULL;
            cr->count=(cr->count)-1;

            pthread_mutex_unlock(&(cr->lock));
            return 0;
        }
    }
    pthread_mutex_unlock(&(cr->lock));
    return -1;

}

CLIENT **creg_all_clients(CLIENT_REGISTRY *cr){

    pthread_mutex_lock(&(cr->lock));
    CLIENT **temp;
    temp = calloc(1,((cr->count)+1)*sizeof(CLIENT*));

    for(int i=0; i<cr->count;i++){
        temp[i]=cr->registry[i];
        client_ref(cr->registry[i],"increment to add to clients list");
    }
    temp[cr->count]=NULL;

    pthread_mutex_unlock(&(cr->lock));

    return temp;

}

void creg_shutdown_all(CLIENT_REGISTRY *cr){

    for(int i=0; i<cr->count;i++){
        shutdown(client_get_fd(cr->registry[i]),SHUT_RDWR);
    }

    while(cr->count!=0){
    }

}