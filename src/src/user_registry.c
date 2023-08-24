#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <debug.h>
#include "user_registry.h"
#include "user.h"
#include "helper.h"

typedef struct user{

   char * login_handle;
   int reference_count;
   USER * next;
   pthread_mutex_t lock;

}USER;

typedef struct user_registry {
    USER *head;
    pthread_mutex_t lock2;
}USER_REGISTRY;


USER_REGISTRY *ureg_init(void){
    printf("Initialize user registry\n");
    USER_REGISTRY *person=(USER_REGISTRY*)malloc(sizeof(USER_REGISTRY));
    (*person).head=NULL;
    pthread_mutex_init(&(person->lock2), NULL);
    return person;
}


void ureg_fini(USER_REGISTRY *ureg){
    USER * ptr,*temp;
    ptr=(*ureg).head;
    printf("Finalize user registry\n");
    while(ptr!=NULL){
            temp=(*ptr).next;
            (*ptr).next=NULL;
            user_unref(ptr,"removing ptr from user registry");
            ptr=temp;
    }
    pthread_mutex_destroy(&(ureg->lock2));
    free(ureg);
}

USER *ureg_register(USER_REGISTRY *ureg, char *handle){
    // add the user with the specific handle onto the register or find

    pthread_mutex_lock(&(ureg->lock2)); ////

    USER * ptr;
    ptr=(*ureg).head;

    if(ptr==NULL){
        // first user is being added to register
        (*ureg).head=user_create(handle);
        user_ref((*ureg).head,"returning pointer of new user from registry");
        pthread_mutex_unlock(&(ureg->lock2)); ////

        return (*ureg).head;
    }
    else{

        while(ptr!=NULL){
            if(strcmp(user_get_handle(ptr),handle)==0){
                // user already exists
                // increment refernence count
                user_ref(ptr,"returning pointer of existing user from registry");
                pthread_mutex_unlock(&(ureg->lock2));
                return ptr;
            }
            if((*ptr).next==NULL){
                break;
            }
            ptr=(*ptr).next;
        }
        // user does not exist add to the registry in the location of ptr

        (*ptr).next=user_create(handle);
        user_ref((*ptr).next,"returning pointer of new user from registry");
        pthread_mutex_unlock(&(ureg->lock2));

        return (*ptr).next;

    }
    return NULL;
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle){

    // pthread_mutex_lock(&(ureg->lock2)); ////
    // USER * ptr,*prev;
    // ptr=(*ureg).head;
    // prev=NULL;
    // if(ptr!=NULL){
    //     while((*ptr).next!=NULL){
    //         if(strcmp((*ptr).login_handle,handle)==0){
    //             if(prev==NULL){
    //                 // remove first element in the list
    //                 //set the head of the user registry to the next user
    //                 (*ureg).head=(*ptr).next;
    //                 user_unref(ptr,"removed from registry");
    //                 pthread_mutex_unlock(&(ureg->lock2)); ////
    //                 return;
    //             }
    //             else {
    //                 // remove the element
    //                 (*prev).next=(*ptr).next;
    //                 user_unref(ptr,"removed from registry");
    //                 pthread_mutex_unlock(&(ureg->lock2)); ////
    //                 return;
    //             }
    //         }
    //         prev=ptr;
    //         ptr=(*ptr).next;
    //     }
    //     if(prev==NULL){
    //         // remove the single first element in the list
    //         (*ureg).head=NULL;
    //         user_unref(ptr,"removed from registry");
    //         pthread_mutex_unlock(&(ureg->lock2)); ////
    //         return;
    //     }
    //     else{
    //         // remove the last element in the list
    //         (*prev).next=NULL;
    //         user_unref(ptr,"removed from registry");
    //         pthread_mutex_unlock(&(ureg->lock2)); ////
    //         return;
    //     }
    // }
    // pthread_mutex_unlock(&(ureg->lock2)); ////
    //  return;
}