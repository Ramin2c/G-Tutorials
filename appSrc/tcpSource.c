//
//  tcpSource.c
//  GStreamerPractice
//
//  Created by Ramin on 2017-08-28.
//  Copyright © 2017 Ramin. All rights reserved.
//

#include "tcpSource.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

static struct serverInfo{
    struct sockaddr *serverAddress;
    int listen_fd;
    int port;
}info;

static bool stopListening = false;
static pthread_t threadID;

static void *startRelaying(void *serverInfo);

static size_t bufferMaxSize = 160000;

//fix return value
void setupTCPSource(int serverPort){
    info.port = serverPort;
    
    //File descriptor
    int listen_fd =  socket(AF_INET, SOCK_STREAM, 0);
    
    //struct to hold IP Address & Port Numbers
    struct sockaddr_in socketAddress;
    memset(&socketAddress, 0, sizeof (struct sockaddr_in));
    
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = htons(INADDR_ANY);
    socketAddress.sin_port = htons(info.port);
    
    int res = bind(listen_fd, (struct sockaddr*)&socketAddress, sizeof(socketAddress));
    if (res < 0){
        printf("bind failed\n");
        exit(EXIT_FAILURE);
    }
    
    res = listen(listen_fd, 1);
    if (res < 0){
        printf("listen failed\n");
        exit(EXIT_FAILURE);
    }
    
    info.listen_fd = listen_fd;
    info.serverAddress = &socketAddress;
    
    pthread_create(&threadID, NULL, startRelaying, NULL);
    printf("TCP Source started, listening on port %d\n", info.port);

}

void stopTCPSource(){
    stopListening = true;
}

static void *startRelaying(){
    int comm_fd = accept(info.listen_fd, (struct sockaddr*)NULL , NULL);
    
    if (comm_fd < 0)
        printf("Accept failed\n");
    
    //As long as it should go, relay whatever data is received
    while(!stopListening)
    {
        char buffer[bufferMaxSize];
        memset(buffer, 0, sizeof(bufferMaxSize));
        ssize_t size = read(comm_fd,buffer, bufferMaxSize);
        if (size > 0){
            memcpy(b_color, buffer, size - 1);
            printf("data %s received of size %d \n", buffer, (int)size);
        }
    }
    
    printf("TCP Source stopped\n");
    return NULL;
}
