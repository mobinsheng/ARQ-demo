#ifndef SERVER_H
#define SERVER_H
/*
 * 来源：https://github.com/cpp2go/kcpnet
 * 基于原始代码做了简单的修改
 */

#ifdef _WIN32
#include <winsock2.h>
#endif
#include <stdio.h>
#include <thread>
#include <mutex>
#include <map>
#include<vector>
#include "task.h"

#define SIMULATE_LOST_PACKET 0

template<typename T>
class ClientThread
{
public:
    void init()
    {
        isstop = false;
        _thread = std::thread(std::bind(&ClientThread::loop, this));
    }
    void shutdown()
    {
        isstop = true;
        _thread.join();
        _mutex.lock();
        for (std::map<IUINT32, Task*>::iterator iter = clients.begin();
            iter != clients.end(); ++iter)
        {
            delete iter->second;
        }
        _mutex.unlock();
    }
    // 循环检查客户是否还存活
    void loop()
    {
        for (; !isstop;)
        {
            _mutex.lock();
            for (std::map<IUINT32, Task*>::iterator iter = clients.begin();
                iter != clients.end();)
            {
                if (!iter->second->isalive())
                {
                    delete iter->second;
                    iter = clients.erase(iter);
                }
                else
                {
                    iter->second->timerloop();
                    ++iter;
                }
            }
            _mutex.unlock();
            std::chrono::microseconds dura(10);
            std::this_thread::sleep_for(dura);
        }
    }

    // 处理接收到的数据，实际调用task的recv进行处理（内部会进行kcp处理，然后调用on_recv虚函数）
    void recv(SOCKET sock, struct sockaddr_in *paddr, IUINT32 conv, const char *buff, int size)
    {
        Task *pclient = NULL;
        _mutex.lock();
        std::map<IUINT32, Task*>::iterator iter = clients.find(conv);
        if (iter == clients.end())
        {
            udpsocket* s = new udpsocket(sock,paddr);
            pclient = new T();
            pclient->init(conv,s);
            clients.insert(std::map<IUINT32, Task*>::value_type(conv, pclient));
        }
        else
        {
            pclient = iter->second;
        }
        pclient->recv(buff, size);
        _mutex.unlock();
    }
public:
    volatile bool isstop;
    std::mutex _mutex;
    std::thread _thread;
    std::map<IUINT32, Task*> clients;
};

template<typename T>
class UdpServer
{
public:

    bool bind(const char *addr, unsigned int short port)
    {
        if (!udpsock.bind(addr, port))
        {
            return false;
        }

        isstop = false;
        _thread = std::thread(std::bind(&UdpServer::run, this));

        maxtdnum = 10;
        for (unsigned int i = 0; i < maxtdnum; i++)
        {
            ClientThread<T>* hthread = new ClientThread<T>;
            hthread->init();
            timerthreads.push_back(hthread);
        }

        return true;
    }

    void shutdown()
    {
        isstop = true;
        udpsock.shutdown();
        _thread.join();
        for (typename  std::vector<ClientThread<T>*>::iterator iter = timerthreads.begin();
            iter != timerthreads.end(); ++iter)
        {
            (*iter)->shutdown();
            delete *iter;
        }
    }

    void run()
    {
        char buff[65536] = { 0 };
        struct sockaddr_in cliaddr;
        for (; !isstop;)
        {
            // 使用udp接收数据
            socklen_t len = sizeof(struct sockaddr_in);
            int size = udpsock.recvfrom(buff, sizeof(buff), (struct sockaddr*)&cliaddr, &len);
            if (size == 0)
            {
                continue;
            }
            if (size < 0)
            {
                printf("接收失败 %d,%d \n", udpsock.getsocket(), size);
                continue;
            }
#if SIMULATE_LOST_PACKET
            // 模拟丢包
            static int index = 0;
            ++index;
            if(index % 4 == 0){
                continue;
            }
#endif

            // 接收到的数据放到线程中进行处理，不要阻塞接收线程
            IUINT32 conv = ikcp_getconv(buff);
            // 注意这里，一个线程可能处理多个kcp（客户由kcp的id进行唯一标识）
            timerthreads[conv % maxtdnum]->recv(udpsock.getsocket(), &cliaddr, conv, buff, size);
        }
    }

private:
    udpsocket udpsock;
    std::thread _thread;
    volatile bool isstop;
    unsigned int maxtdnum;
    std::vector<ClientThread<T>*> timerthreads;
};


#endif // SERVER_H
