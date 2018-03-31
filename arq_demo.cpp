#include "arq_demo.h"
#include <assert.h>
#include "ikcp.h"
#include <pthread.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "ikcp.h"
#include "time_func.h"
#include "server.h"
#include "client.h"

static const char* ip = "127.0.0.1";
static const int port = 9898;

//1400
#define PACKET_SIZE 1024
#define MTU_SIZE 1400

#define KCP_ID 123

// https://github.com/cpp2go/kcpnet
class server_processor:public Task{
public:
    int on_recv(const char *buf, int len){
        printf("%s\n", buf);
        return 0;
    }
};

class client_processor:public Task{
public:
    int on_recv(const char *buf, int len){
        return 0;
    }
};

void* server_thread(void *){
    UdpServer<server_processor> server;
    server.bind(ip,port);
    server.run();
}


void* client_thread(void *){
    UdpClient<client_processor> client;
    client.connect(ip,port,KCP_ID);

    char buf[PACKET_SIZE] = {0};

    int index = 0;
    while (true) {
        if(index >= 1000){
            break;
        }
        memset(buf,0,PACKET_SIZE);
        sprintf(buf,"%d",index++);

        client.send(buf,PACKET_SIZE);
        usleep(20 * 1000);
    }

    client.shutdown();
}
