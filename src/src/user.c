#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "user.h"

typedef struct user{

   char * login_handle;
   int reference_count;
   USER * next;
   pthread_mutex_t lock;

}USER;


USER *user_ref(USER *user, char *why){

    pthread_mutex_lock(&(user->lock));

    user->reference_count= user->reference_count+1;
    fprintf(stdout,"Increase reference count on user %p (%d -> %d) for %s\n",user,user->reference_count-1,user->reference_count,why);

    pthread_mutex_unlock(&(user->lock));

    return user;
}

void user_unref(USER *user, char *why){

    pthread_mutex_lock(&(user->lock));

    user->reference_count= user->reference_count-1;
    fprintf(stdout,"Decrease reference count on user %p (%d -> %d) for %s\n",user,user->reference_count+1,user->reference_count,why);
    if(user->reference_count==0){
        free((*user).login_handle);
        pthread_mutex_unlock(&(user->lock));
        pthread_mutex_destroy(&(user->lock));
        free(user);
        return;
    }
    pthread_mutex_unlock(&(user->lock));
}

USER *user_create(char *handle){

    USER *person=(USER*)malloc(sizeof(USER));
    char * userhandle=malloc(strlen(handle)+1);
    strcpy(userhandle,handle);
    person->login_handle=userhandle;
    person->reference_count=0;
    person->next=NULL;
    pthread_mutex_init(&(person->lock), NULL);
    user_ref(person,"Creating a new user");

    return person;
}

char *user_get_handle(USER *user){
    char * handle=user->login_handle;
    return handle;
}