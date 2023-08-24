#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "csapp.h"
#include "protocol.h"
#include "globals.h"
#include "helper.h"

void * chla_client_service(void *arg){

    pthread_detach(pthread_self());
    int fd = *((int*)arg);
    printf("Starting client service for fd: %d\n",fd);
    // create a client
    CLIENT * client= creg_register(client_registry,fd);
    CHLA_PACKET_HEADER *pkt=(CHLA_PACKET_HEADER*)calloc(1,sizeof(CHLA_PACKET_HEADER));
    void *payload[200]; ////
    payload[199]=NULL;
    pthread_t id;

    while (1){
        int status=-1;
        //while(status!=0){
        status=proto_recv_packet(fd,pkt,payload);
        printf("status %d\n",status);
        //}
        if(pkt->type==CHLA_LOGIN_PKT && status==0){
            printf("LOGIN\n");
            char * handle=strtok((char*)(*payload),"\r\n");
            printf("%s\n",handle);
            int clogin=client_login(client,handle);
            // may need to create another thread and call mailbox service ****

            client_ref(client,"reference passed to mailbox service thread");
            Pthread_create(&id,NULL,mailbox_service,(void*)client);

            if(clogin==0){
                //login success
                client_send_ack(client,ntohl(pkt->msgid),payload,ntohs(pkt->payload_length));
            }
            else{
                // login failed
                client_send_nack(client,ntohl(pkt->msgid));
            }
            free(*payload);
        }
        else if(pkt->type==CHLA_LOGOUT_PKT || status !=0){
            printf("LOGOUT\n");
            int clogout=client_logout(client);
            if(clogout!=0 && status==-1){
                //unregister client
                //free pkt
                break;
            }
            // shutdown mailbox service thread by calling pthread join
            printf("Waiting for mailbox service thread to terminate\n");

            pthread_join(id,NULL);

            client_unref(client,"remove reference passed to mailbox service thread");

            if(clogout==0){
                //logout success
                if(status==-1){break;}
                client_send_ack(client,ntohl(pkt->msgid),payload,ntohs(pkt->payload_length));

            }
            else{
                // logout failed
                client_send_nack(client,ntohl(pkt->msgid));
            }
        }
        else if(pkt->type==CHLA_USERS_PKT){
            printf("USERS\n");
            // format the user registry and send it to the client
            // get clients list from creg and get the users then their handles concat them and send
            CLIENT ** arr = creg_all_clients(client_registry);
            int i=0;
            //int j=0;
            int size=0;
            USER * user;
            char * users=(char*)calloc(1,700*sizeof(char));
            //users=strdup("");
            while(*(arr+i)!= NULL){
                user=client_get_user(*(arr+i),0);
                if(user!=NULL){
                strcat(users,user_get_handle(user));
                strcat(users,"\r\n");
                size=size+2+strlen(user_get_handle(user));
                //j++;
                user_unref(user,"reference no longer needed");
                }
                i++;
            }
            i=0;
            while(*(arr+i)!= NULL){
            client_unref(*(arr+i),"removing reference from clients list");
            i++;
            }
            if(*arr!=NULL){
            free(arr);
            }
            client_send_ack(client,ntohl(pkt->msgid),users,size);
            free(users);
        }
        else if(pkt->type==CHLA_SEND_PKT){
            printf("SEND\n");
            //get the clients from creg find the mailbox of the client to send
            // add the message in the mailbox of that client accordingly call send message and send notice

            char * msg=strstr((char*)(*payload),"\n");
            msg=strtok(msg,"\n");
            char * tohandle=strtok((char*)(*payload),"\r\n");
            printf("msg:%s\n",msg);
            CLIENT ** arr = creg_all_clients(client_registry);
            int i=0;
            int found=0;
            MAILBOX * tomail;
            while(*(arr+i)!= NULL){
                tomail=client_get_mailbox(*(arr+i),0);
                if(tomail!=NULL){
                    if(strcmp(mb_get_handle(tomail),tohandle)==0){
                        found=1;
                        break;
                    }
                    mb_unref(tomail,"reference no longer needed");
                }
                i++;
            }
            i=0;
            while(*(arr+i)!= NULL){
            client_unref(*(arr+i),"removing reference from clients list");
            i++;
            }
            if(found==1){
                MAILBOX * temp = client_get_mailbox(client,0);
                if(temp!=NULL){
                mb_add_message(tomail,ntohl(pkt->msgid),temp,msg,strlen(msg));
                mb_unref(tomail,"removing reference");
                mb_unref(temp,"message added ");
                client_send_ack(client,ntohl(pkt->msgid),NULL,0);
                }
                else{
                    client_send_nack(client,ntohl(pkt->msgid));
                }
            }
            else{
                //send nack if handle doesnt exist
                client_send_nack(client,ntohl(pkt->msgid));
            }
            if(*arr!=NULL){
            free(arr);
            }

        }
        else{
            //invalid type i think client handles this
        }

    }
    free(pkt);
    client_unref(client,"removing reference from terminating client");
    creg_unregister(client_registry,client);
    printf("Ending client service for fd:%d\n",fd);
    free(arg);
    return NULL;
}

void * mailbox_service(void * c){
    CLIENT * client = (CLIENT*)c;
    MAILBOX * mb=client_get_mailbox(client,0);
    printf("in mailbox service\n");
    MAILBOX_ENTRY * entry = mb_next_entry(mb);
    while(entry!=NULL){
        if(entry->type==MESSAGE_ENTRY_TYPE){
            // message entry
            // deliver the message
            // if delievery successful
            // send notice to sender accordingly
            printf("Process message\n");
            // send message to client using client send
            CHLA_PACKET_HEADER *pkt=(CHLA_PACKET_HEADER*)calloc(1,sizeof(CHLA_PACKET_HEADER));
            (*pkt).msgid=htonl(((entry->content).message).msgid);
            (*pkt).payload_length=htonl((((entry->content).message).length) + 2 + strlen(mb_get_handle(((entry->content).message).from)));
            (*pkt).type=(htons(CHLA_MESG_PKT))>>8;

            char * payload;
            payload=(char *)calloc(1,100*(sizeof(char)));
            strcat(payload,mb_get_handle(((entry->content).message).from));
            strcat(payload,"\r\n");
            strcat(payload,(char *)(((entry->content).message).body));
            printf("message %s\n",payload);
            int status=client_send_packet(client,pkt,payload);
            if(status==0){
                // send notice that message has been received by the client
                mb_add_notice(((entry->content).message).from,RRCPT_NOTICE_TYPE,((entry->content).message).msgid);
            }
            else{
                //send notice that client did not receive the message
                mb_add_notice(((entry->content).message).from,BOUNCE_NOTICE_TYPE,((entry->content).message).msgid);
            }
            free(payload);
            free(entry);
            free(pkt);
        }
        else if (entry->type==NOTICE_ENTRY_TYPE){
            // notice entry
            printf("Process notice\n");
            // send notice to the client using client send
            CHLA_PACKET_HEADER *pkt=(CHLA_PACKET_HEADER*)calloc(1,sizeof(CHLA_PACKET_HEADER));
            if(((entry->content).notice).type == RRCPT_NOTICE_TYPE){
                (*pkt).msgid=htonl(((entry->content).notice).msgid);
                (*pkt).payload_length=htonl(0);
                (*pkt).type=(htons(CHLA_RCVD_PKT))>>8;
            }
            else {
                //bounce notice
                (*pkt).msgid=htonl(((entry->content).notice).msgid);
                (*pkt).payload_length=htonl(0);
                (*pkt).type=(htons(CHLA_BOUNCE_PKT))>>8;
            }
            client_send_packet(client,pkt,NULL);
            free(entry);
            free(pkt);
        }
        else{
            //invalid entry type
            printf("Invalid entry type\n");
        }
        entry=mb_next_entry(mb);
    }
    //entry is null which means shutdown
    mb_unref(mb,"refernce no longer needed in mailbox service thread");
    printf("Terminating mailbox service thread\n");
    return NULL;
}