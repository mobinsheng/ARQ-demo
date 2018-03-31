#ifndef CLIENT_H
#define CLIENT_H

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
#include "task.h"

template<typename T>
class UdpClient
{
public:
    UdpClient() :task(NULL){}
    ~UdpClient()
    {
        if (task != NULL)
        {
            delete task;
        }
    }
    //  连接到服务器
    bool connect(const char *addr, unsigned short int port, IUINT32 conv)
    {
        if (!udpsock.connect(addr, port))
        {
            return false;
        }

        task = new T;
        if (!task->init(conv, &udpsock))
        {
            return false;
        }

        isstop = false;

        // udp接收线程接收到数据之后，放到kcp中，kcp线程检测内部缓冲区，把用户级别的数据
        // 提取出来

        // 启动udp数据接收线程
        _thread = std::thread(std::bind(&UdpClient::udp_recv_loop, this));
        // 启动kcp数据组装线程
        _threadtm = std::thread(std::bind(&UdpClient::kcp_process_loop, this));
        return true;
    }

    int send(const char *buf, int size)
    {
        _mutex.lock();
        int ret = task->send(buf, size);
        _mutex.unlock();
        return ret;
    }

    void shutdown()
    {
        isstop = true;
        udpsock.shutdown();
        _thread.join();
        _threadtm.join();
    }


    bool isalive()
    {
        _mutex.lock();
        bool alive = task->isalive();
        _mutex.unlock();
        return alive;
    }

    virtual int parsemsg(const char *buf, int len)
    {
        printf("收到数据 %s,%d\n", buf, len);
        return 0;
    }
private:
    void kcp_process_loop()
    {
        for (; !isstop;)
        {
            _mutex.lock();
            task->timerloop();
            _mutex.unlock();
            std::chrono::milliseconds dura(1);
            std::this_thread::sleep_for(dura);
        }
    }

    void udp_recv_loop()
    {
        char buff[65536] = { 0 };
        struct sockaddr_in seraddr;
        for (; !isstop;)
        {
            socklen_t len = sizeof(struct sockaddr_in);
            int size = udpsock.recvfrom(buff, sizeof(buff), (struct sockaddr*)&seraddr, &len);
            if (size == 0)
            {
                continue;
            }
            if (size < 0)
            {
                printf("接收失败 %d,%d \n", udpsock.getsocket(), size);
                continue;
            }
            _mutex.lock();
            task->recv(buff, size);
            _mutex.unlock();
        }
    }
private:
    udpsocket udpsock;
    Task* task;
    std::thread _thread;
    std::thread _threadtm;
    std::mutex _mutex;
    volatile bool isstop;
};

#endif // CLIENT_H
