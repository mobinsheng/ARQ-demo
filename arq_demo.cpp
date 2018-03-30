#include "arq_demo.h"
#include "sync_queue.h"
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

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#include <windows.h>
#elif !defined(__unix)
#define __unix
#endif

#ifdef __unix
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#endif

/* get system time */
static inline void itimeofday(long *sec, long *usec)
{
    #if defined(__unix)
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
    #else
    static long mode = 0, addsec = 0;
    BOOL retval;
    static IINT64 freq = 1;
    IINT64 qpc;
    if (mode == 0) {
        retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        freq = (freq == 0)? 1 : freq;
        retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
        addsec = (long)time(NULL);
        addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
        mode = 1;
    }
    retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
    retval = retval * 2;
    if (sec) *sec = (long)(qpc / freq) + addsec;
    if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
    #endif
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
    long s, u;
    IINT64 value;
    itimeofday(&s, &u);
    value = ((IINT64)s) * 1000 + (u / 1000);
    return value;
}

static inline IUINT32 iclock()
{
    return (IUINT32)(iclock64() & 0xfffffffful);
}

/* sleep in millisecond */
static inline void isleep(unsigned long millisecond)
{
    #ifdef __unix 	/* usleep( time * 1000 ); */
    struct timespec ts;
    ts.tv_sec = (time_t)(millisecond / 1000);
    ts.tv_nsec = (long)((millisecond % 1000) * 1000000);
    /*nanosleep(&ts, NULL);*/
    usleep((millisecond << 10) - (millisecond << 4) - (millisecond << 3));
    #elif defined(_WIN32)
    Sleep(millisecond);
    #endif
}

int create_client_socket (const char* ip,int port,struct sockaddr_in* server_addr){
    int addrLen;
    int sclient_fd;
    sclient_fd = socket(AF_INET,SOCK_DGRAM,0);
    if (sclient_fd == -1){
        perror("socket fail");
        return EXIT_FAILURE;
    }
    addrLen=sizeof(struct sockaddr_in);
    bzero(server_addr,addrLen);

    server_addr->sin_family=AF_INET;
    server_addr->sin_port=htons(port);
    if (inet_pton(AF_INET,ip,&server_addr->sin_addr)==0){
        printf("Invalid IP adress\n");
        return EXIT_FAILURE;
    }
    return sclient_fd;
}

int create_server_socket (const char* ip,int port){
    socklen_t l;
    int sfd;

    struct sockaddr_in sock_serv;

    sfd = socket(AF_INET,SOCK_DGRAM,0);
    if (sfd == -1){
        perror("socket fail");
        return EXIT_FAILURE;
    }

    l=sizeof(struct sockaddr_in);
    bzero(&sock_serv,l);

    if(ip == NULL || strlen(ip) == 0){
        sock_serv.sin_family=AF_INET;
        sock_serv.sin_port=htons(port);
        sock_serv.sin_addr.s_addr=htonl(INADDR_ANY);
    }
    else{
        sock_serv.sin_family=AF_INET;
        sock_serv.sin_port=htons(port);
        if (inet_pton(AF_INET,ip,&sock_serv.sin_addr)==0){
            printf("Invalid IP adress\n");
            return EXIT_FAILURE;
        }
    }

    if(::bind(sfd,(struct sockaddr*)&sock_serv,l)==-1){
        perror("bind fail");
        return EXIT_FAILURE;
    }

    return sfd;
}

static const char* ip = "127.0.0.1";
static const int port = 9898;
int client_fd;
int server_fd;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
ikcpcb* server_kcp;
ikcpcb* client_kcp;
//1400
#define SIZE 1024

#define KCP_ID 123

int client_output(const char *buf, int len, ikcpcb *kcp, void *user){
    socklen_t addrLen = sizeof(sockaddr_in);
    return sendto(client_fd,buf,len,0,(sockaddr*)&server_addr,addrLen);
}

int server_output(const char *buf, int len, ikcpcb *kcp, void *user){
    socklen_t addrLen = sizeof(sockaddr_in);
    return sendto(server_fd,buf,len,0,(sockaddr*)&client_addr,addrLen);
}

void* server_thread(void *){
    server_kcp = ikcp_create(KCP_ID,0);
    server_kcp->output = server_output;
    ikcp_wndsize(server_kcp, 128, 128);
    ikcp_nodelay(server_kcp, 0, 10, 0, 0);

    server_fd = create_server_socket(ip,port);
    assert(server_fd >= 0);

    char buf[SIZE] = {0};
    socklen_t addrLen = sizeof(sockaddr_in);

    int x = 0;
    while (true) {
        ikcp_update(server_kcp, iclock());
        memset(buf,0,SIZE);
        int ret = recvfrom(server_fd,buf,SIZE,0,(sockaddr*)&client_addr,&addrLen);
        if(ret <= 0){
            break;
        }
        if(strcmp(buf,"bye") == 0){
            break;
        }

        x = ikcp_input(server_kcp, buf, ret);

    }

    while (true) {
        int ret = ikcp_recv(server_kcp, buf, 64);
        // 没有收到包就退出
        if (ret < 0)
            break;
        else{
            printf(buf);
            printf("\n");
        }
    }

    memset(buf,0,SIZE);
    sprintf(buf,"bye");
    sendto(server_fd,buf,SIZE,0,(sockaddr*)&client_addr,addrLen);
}

void* client_recv_thread(void*){
    char buf[SIZE] = {0};
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(sockaddr_in);
    while (true) {
        memset(buf,0,SIZE);
        socklen_t len = sizeof(sockaddr_in);
        int ret = recvfrom(client_fd,buf,SIZE,0,(sockaddr*)&addr,&addrLen);
        if(ret <= 0){
            break;
        }
        if(strcmp(buf,"bye") == 0){
            break;
        }
        ikcp_input(client_kcp, buf, ret);
        while (true) {
            ret = ikcp_recv(client_kcp, buf, 10);
            // 没有收到包就退出
            if (ret < 0)
                break;
        }

    }
}

void* client_thread(void *){
    client_kcp = ikcp_create(KCP_ID,0);
    client_kcp->output = client_output;
    ikcp_wndsize(client_kcp, 128, 128);
    ikcp_nodelay(client_kcp, 0, 10, 0, 0);

    client_fd = create_client_socket(ip,port,&server_addr);
    assert(client_fd >= 0);

    pthread_t t1;
    pthread_create(&t1,NULL,client_recv_thread,NULL);

    int index = 0;
    char buf[SIZE] = {0};
    socklen_t addrLen = sizeof(sockaddr_in);
    while (true) {
        ikcp_update(client_kcp, iclock());
        if(index >= 1000){
            break;
        }
        memset(buf,0,SIZE);
        sprintf(buf,"%d",index);

        if(index % 100 != 0){
            ikcp_send(client_kcp,buf,SIZE);
        }

        ikcp_flush(client_kcp);

        usleep(1000);

        ++index;
    }



    memset(buf,0,SIZE);
    sprintf(buf,"bye");
    sendto(client_fd,buf,SIZE,0,(sockaddr*)&server_addr,addrLen);

    pthread_join(t1,NULL);
}
