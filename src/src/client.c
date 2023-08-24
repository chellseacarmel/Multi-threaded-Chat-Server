#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "protocol.h"
#include "client_registry.h"
#include  "user.h"
#include "client.h"
#include "globals.h"
#include "mailbox.h"


typedef struct client{

   int file_d;
  // int client_id; // loc in client registry
   int state;//0--logged out 1--logged in
   USER * curr_user;
   int reference_count;
   MAILBOX * curr_mailbox;
   pthread_mutex_t lock;

}CLIENT;



CLIENT *client_create(CLIENT_REGISTRY *creg, int fd){

    CLIENT *pc=(CLIENT*)malloc(sizeof(CLIENT));
    pc->file_d=fd;
    //pc->client_id=i;
    pc->state=0;
    pc->reference_count=0;
    pc->curr_user=NULL;
    pc->curr_mailbox=NULL;
    pthread_mutex_init(&(pc->lock), NULL);
    client_ref(pc,"Creating a new client");

    return pc;
}

CLIENT *client_ref(CLIENT *client, char *why){

    pthread_mutex_lock(&(client->lock)); /////

    client->reference_count= client->reference_count+1;
    fprintf(stdout,"Increase reference count on client %p (%d -> %d) for %s\n",client,client->reference_count-1,client->reference_count,why);

    pthread_mutex_unlock(&(client->lock)); ////

    return client;
}

void client_unref(CLIENT *client, char *why){

    pthread_mutex_lock(&(client->lock)); ////

    client->reference_count= client->reference_count-1;
    fprintf(stdout,"Decrease reference count on client %p (%d -> %d) for %s\n",client,client->reference_count+1,client->reference_count,why);
    if(client->reference_count==0){
        if(client->curr_user!=NULL){
        user_unref(client->curr_user,"removing refernce from client"); /// was giving valgrind errors
        }
        if(client->curr_mailbox!=NULL){
        mb_unref(client->curr_mailbox,"removing reference from client"); /// was giving valgrind errors
        }
        printf("Freeing client at %p",client);
        pthread_mutex_unlock(&(client->lock));
        pthread_mutex_destroy(&(client->lock));

        free(client);
        return;

    }
    pthread_mutex_unlock(&(client->lock)); ////

}

USER *client_get_user(CLIENT *client, int no_ref){
    //not sure what to do if no_ref is -ve
    if(client->state==0){
        return NULL;
    }
    else{
        USER * person= client->curr_user;
        if(no_ref==0){
        //increment ref count while returning
        user_ref(person,"Getting a user");
        }
        return person;
    }

}

MAILBOX *client_get_mailbox(CLIENT *client, int no_ref){
    // not sure what to do if no_ref is -ve
    if(client->state==0){
        return NULL;
    }
    else{
        MAILBOX * mail= client->curr_mailbox;
        if(no_ref==0){
        //increment ref count while returning
        mb_ref(mail,"Getting a mailbox");
        }
        return mail;
    }
}

int client_get_fd(CLIENT *client){
    int fd=client->file_d;
    return fd;
}

int client_login(CLIENT *client, char *handle){
    // is locking needed here???
    // make a call to ureg_register and add the user to the client
    // make a call to mb_init and add the mailbox to the client
    // if client status ie is logged in then error
    // if another client with same handle exist then error ***

    if((*client).state==1){
        //client already logged in
        return -1;
    }
    else{
        CLIENT ** arr = creg_all_clients(client_registry);
        int i=0;
        int exists=0;
        while(*(arr+i)!= NULL){
            if((*(arr+i))->curr_user!=NULL){
                if(strcmp(handle,user_get_handle((*(arr+i))->curr_user))==0){
                exists=1;
                break;
                }
            }
            i++;
        }
        i=0;
        while(*(arr+i)!= NULL){
            client_unref(*(arr+i),"removing refernce from creg list");
            i++;
        }
        free(arr);

        if(exists!=1){
            USER * temp= ureg_register(user_registry,handle);
            MAILBOX * mail= mb_init(handle);
            if(temp==NULL || mail ==NULL){
            return -1;
            }
            else{
            (*client).curr_user=temp;
            (*client).curr_mailbox=mail;
            (*client).state=1;
            // add a print statement here
            fprintf(stdout,"Login client %p as user %p [%s] with mailbox %p\n",client,temp,user_get_handle(temp),mail);
            }
        }
        else{
            return -1;
        }
    }
    return 0;
}

int client_logout(CLIENT *client){
    //clear the ptr to user and mailbox
    // check the status if already logged out return -1
    if((*client).state==0){
        //client already logged out
        return -1;
    }
    else{
        user_unref((*client).curr_user,"reference being removed from logged out client");
        (*client).curr_user=NULL;
        mb_unref((*client).curr_mailbox,"reference being removed from logged out client");
        mb_shutdown((*client).curr_mailbox);
        (*client).curr_mailbox=NULL;
        (*client).state=0;
    }
    return 0;
}


int client_send_packet(CLIENT *user, CHLA_PACKET_HEADER *pkt, void *data){

    pthread_mutex_lock(&(user->lock));

	int status=proto_send_packet(user->file_d,pkt,data);

    pthread_mutex_unlock(&(user->lock));
    return status;
}

int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen){
    // create a ack packet and send it by calling send_packet
    CHLA_PACKET_HEADER *pkt=(CHLA_PACKET_HEADER*)calloc(1,sizeof(CHLA_PACKET_HEADER));

    (*pkt).msgid=htonl(msgid);
    (*pkt).payload_length=htonl(datalen);
    (*pkt).type=(htons(CHLA_ACK_PKT))>>8;

    int status=client_send_packet(client,pkt,data);

    free(pkt);
    return status;
}

int client_send_nack(CLIENT *client, uint32_t msgid){

    CHLA_PACKET_HEADER *pkt=(CHLA_PACKET_HEADER*)calloc(1,sizeof(CHLA_PACKET_HEADER));

    (*pkt).msgid=htonl(msgid);
    (*pkt).payload_length=htonl(0);
    (*pkt).type=(htons(CHLA_NACK_PKT))>>8;

    int status=client_send_packet(client,pkt,NULL);

    free(pkt);
    return status;
}