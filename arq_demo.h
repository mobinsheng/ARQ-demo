#ifndef _ARQ_DEMO_H_
#define _ARQ_DEMO_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

int create_server_socket (const char* ip,int port);

void* server_thread(void*);

int create_client_socket (const char* ip,int port,struct sockaddr_in* server_addr);

void* client_thread(void*);

#endif // COMMON_H
