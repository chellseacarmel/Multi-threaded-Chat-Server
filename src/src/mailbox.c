#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "mailbox.h"

/// mailbox is a linked list queue

typedef struct mail MAIL;

typedef struct mail{
    MAILBOX_ENTRY * entry;
    MAIL* next;
}MAIL;

typedef struct mailbox{
    char * mhandle;
    MAIL *queue; // start of the queue
    MAILBOX_DISCARD_HOOK * hook;
    int qsize;
    int state;//0 if functional and 1 if not
    pthread_mutex_t lock;
    pthread_mutex_t lock2; ///used only for ref unref
    pthread_mutex_t lock3; /// used in mb_next_entry
    sem_t mutex;
    int reference_count;
} MAILBOX;


void mb_ref(MAILBOX *mb, char *why){

    pthread_mutex_lock(&(mb->lock2));

    mb->reference_count= mb->reference_count+1;
    fprintf(stdout,"Increase reference count on mailbox %p (%d -> %d) for %s\n",mb,mb->reference_count-1,mb->reference_count,why);

    pthread_mutex_unlock(&(mb->lock2));
}

void mb_unref(MAILBOX *mb, char *why){
    pthread_mutex_lock(&(mb->lock2));

    mb->reference_count= mb->reference_count-1;
    fprintf(stdout,"Decrease reference count on mailbox %p (%d -> %d) for %s\n",mb,mb->reference_count+1,mb->reference_count,why);
    if(mb->reference_count==0){

        MAIL * ptr;
        ptr=(mb->queue);
        while(ptr!=NULL){
            if(ptr->entry!=NULL){
                if(mb->hook !=NULL){
                (*(mb->hook))(ptr->entry);
                 // need to also call mb_unref
                }
                if((ptr->entry)->type==MESSAGE_ENTRY_TYPE){
                free((ptr->entry)->content.message.body);
                }
            free(ptr->entry);
            //free(ptr);
            }
            ptr=ptr->next;
        }
        free((*mb).mhandle);
        //free(mb->queue);
        pthread_mutex_unlock(&(mb->lock2));
        pthread_mutex_destroy(&(mb->lock));
        pthread_mutex_destroy(&(mb->lock2));
        pthread_mutex_destroy(&(mb->lock3));
        sem_destroy(&(mb->mutex));
        free(mb);
        return;
    }
    pthread_mutex_unlock(&(mb->lock2));
}

void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK * hook){
    pthread_mutex_lock(&(mb->lock));
    mb->hook=hook;
    pthread_mutex_unlock(&(mb->lock));
}

void mb_shutdown(MAILBOX *mb){
    pthread_mutex_lock(&(mb->lock));
    mb->state=1;
    sem_post(&(mb->mutex));
    pthread_mutex_unlock(&(mb->lock));
}

MAILBOX *mb_init(char *handle){
    if(handle==NULL){
        return NULL;
    }
    else{
    MAILBOX *mail=(MAILBOX*)malloc(sizeof(MAILBOX));
    char * userhandle=malloc(strlen(handle)+1);
    strcpy(userhandle,handle);
    mail->mhandle=userhandle;
    mail->qsize=0;
    mail->reference_count=0;
    mail->queue=NULL;
    mail->hook=NULL;
    mail->state=0;
    pthread_mutex_init(&(mail->lock), NULL);
    pthread_mutex_init(&(mail->lock2), NULL);
    pthread_mutex_init(&(mail->lock3), NULL);
    sem_init(&(mail->mutex), 1, 0);
    mb_ref(mail,"Creating a new mailbox");
    return mail;
    }
}

char *mb_get_handle(MAILBOX *mb){
    char * handle=mb->mhandle;
    return handle;
}

void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length){
    pthread_mutex_lock(&(mb->lock));

    MAILBOX_ENTRY * msg = (MAILBOX_ENTRY*)malloc(sizeof(MAILBOX_ENTRY));
    msg->type=MESSAGE_ENTRY_TYPE;
    ((msg->content).message).msgid=msgid;
    ((msg->content).message).from=from;
    ((msg->content).message).body=body;
    ((msg->content).message).length=length;

    // add the msg to the mailbox

    if(mb->queue==NULL){
        //add first element
        // create a mail object
        MAIL * m=(MAIL*)malloc(sizeof(MAIL));
        m->entry=msg;
        m->next=NULL;
        mb->queue=m;
        if(mb!=from){
            //increment ref to senders mailbox;
            mb_ref(from,"message added to mailbox");
        }
        mb->qsize=(mb->qsize)+1;
        sem_post(&(mb->mutex));
        pthread_mutex_unlock(&(mb->lock));
        return;
    }
    else{

        MAIL * ptr;
        ptr=(mb->queue); ////
        while(ptr!=NULL){
            if(ptr->next==NULL){
                break;
            }
        ptr=ptr->next;
        }
        //insert the msg ptr in the mailbox at ptr
        MAIL * m=(MAIL*)malloc(sizeof(MAIL));
        m->entry=msg;
        m->next=NULL;
        ptr->next=m;
        if(mb!=from){
            //increment ref to senders mailbox;
            mb_ref(from,"message added to mailbox");
       }
        mb->qsize=(mb->qsize)+1;
        sem_post(&(mb->mutex));
        pthread_mutex_unlock(&(mb->lock));
        return;

    }
    pthread_mutex_unlock(&(mb->lock));
}


void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid){
    pthread_mutex_lock(&(mb->lock));

    MAILBOX_ENTRY * msg = (MAILBOX_ENTRY*)malloc(sizeof(MAILBOX_ENTRY));
    msg->type=NOTICE_ENTRY_TYPE;
    ((msg->content).notice).type=ntype;
    ((msg->content).notice).msgid=msgid;

    // add the msg to the mailbox

    if(mb->queue==NULL){
        //add first element
        // create a mail object
        MAIL * m=(MAIL*)malloc(sizeof(MAIL));
        m->entry=msg;
        m->next=NULL;
        mb->queue=m;
        mb->qsize=(mb->qsize)+1;
        sem_post(&(mb->mutex));
        pthread_mutex_unlock(&(mb->lock));
        return;
    }
    else{

        MAIL * ptr;
        ptr=(mb->queue); ////
        while(ptr!=NULL){
            if(ptr->next==NULL){
                break;
            }
        ptr=ptr->next;
        }
        //insert the msg ptr in the mailbox at ptr
        MAIL * m=(MAIL*)malloc(sizeof(MAIL));
        m->entry=msg;
        m->next=NULL;
        ptr->next=m;
        mb->qsize=(mb->qsize)+1;
        sem_post(&(mb->mutex));//
        pthread_mutex_unlock(&(mb->lock));
        return;

    }
    pthread_mutex_unlock(&(mb->lock));
}

MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb){

    while(mb->qsize==0 && mb->state!=1){
           sem_wait(&(mb->mutex));
    }
    if(mb->state==1){
        return NULL;
    }
    else{
       //remove an entry from the beginning of the list
        pthread_mutex_lock(&(mb->lock3));
        MAILBOX_ENTRY * temp;
       if(mb->qsize==1){
           temp=(mb->queue)->entry;
           free(mb->queue);
           mb->queue=NULL;
           mb->qsize=(mb->qsize)-1;
           if(temp->type==MESSAGE_ENTRY_TYPE && mb!=(temp->content).message.from){
           // decrease ref of from
           mb_unref((temp->content).message.from,"removing reference from mailbox");
           }
            pthread_mutex_unlock(&(mb->lock3));
            return temp;
       }
       else if(mb->qsize>1){
           temp=(mb->queue)->entry;
           MAIL * ptr=(mb->queue)->next;
           free(mb->queue);
           mb->queue=ptr;
           mb->qsize=(mb->qsize)-1;
           if(temp->type==MESSAGE_ENTRY_TYPE && mb!=(temp->content).message.from){
           // decrease ref of from
           mb_unref((temp->content).message.from,"removing reference from mailbox");
           }
           pthread_mutex_unlock(&(mb->lock3));
           return temp;
       }
    }
    //printf("Its reaching this null beware \n");
    return NULL;
}