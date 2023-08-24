#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"
#include "csapp.h"

int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload){
    //fd-- file discriptor of socket
    //ptr to hdr
    //ptr to payload
   int length=(*hdr).payload_length;
   length=htonl(length);
   int type=(*hdr).type;
   if(length>0 && type!=0 && payload!=NULL){
       //payload exists need to do two writes
        rio_writen(fd,hdr,sizeof(*hdr));
        rio_writen(fd,payload,length);

   }
   else if(length==0 && type!=0){
       //payload doesnt exist need to do only one write
       rio_writen(fd,hdr,sizeof(*hdr));
   }
   else{
       return -1;
   }

    return 0;
}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload){
    int read=rio_readn(fd,hdr,sizeof(CHLA_PACKET_HEADER));
    if(read != sizeof(CHLA_PACKET_HEADER)){
        return -1;
    }
    uint32_t length;
    length=ntohl(hdr->payload_length);
    uint8_t type=0;
    type=(hdr->type);
    //fprintf(stdout,"length in recv %d\n",length);
    if(payload!=NULL && type!=0 && length>0){ /// payload

       char *pptr=calloc(1,length);
       int r=rio_readn(fd,pptr,length);
       if(r!=length){
           return -1;
       }
       *payload=pptr;
        return 0;
       //fprintf(stdout,"payload is: %s\n",(char*)pptr);
    }
    else if(length==0 && type !=0){
        return 0;
    }
    else{
        return -1;
    }
    return 0;

 }