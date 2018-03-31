#ifndef KCP_WRAP_H
#define KCP_WRAP_H
/*
 * 来源：https://github.com/cpp2go/kcpnet
 * 基于原始代码做了简单的修改
 */

#include <stdio.h>
#include "ikcp.h"
#include "time_func.h"
#include "udp_socket.h"

/*
 * task：用户处理数据的地方
 * 发送数据：调用send函数
 * 接收数据：需要实现on_recv虚函数
 * 封装了kcp的底层操作
 * 另外，udpserver、udpclient的发送数据的时候实际上也是经过task的send函数
 */
template <typename T> class UdpServer;
template <typename T> class ClientThread;
template <typename T> class UdpClient;
class Task
{
public:
    template <typename T> friend class UdpServer;
    template <typename T> friend class ClientThread;
    template <typename T> friend class UdpClient;
    Task()
    {
        conv = 0;
        kcp = NULL;
        current = 0;
        nexttime = 0;
        alivetime = nexttime + 10000;
        memset(buffer, 0, sizeof(buffer));
    }

    ~Task()
    {
        if (kcp != NULL)
        {
            ikcp_flush(kcp);
            ikcp_release(kcp);
        }
        printf("关闭连接 %d\n", conv);
    }

    int send(const char  * buf, int len)
    {
        // 调用kcp进行数据发送
        int nret = ikcp_send(kcp, buf, len);
        if (nret == 0)
        {
            nexttime = iclock();
            alivetime = nexttime + 10000;
        }
        //printf("发送数据 %d,%d,%d\n", conv, len, nret);
        return nret;
    }

    virtual bool isalive()
    {
        return alivetime > current;
    }

    virtual int on_recv(const char *buf, int len) = 0;

private:
    bool init(IUINT32 conv, udpsocket *udpsock ,int mode = 2)
    {
        if (udpsock == NULL)
        {
            return false;
        }
        this->conv = conv;
        this->nexttime = 0;

        kcp = ikcp_create(conv, (void*)udpsock);

        kcp->output = &Task::udp_output;
        //kcp->logmask = 0xffff;
        //kcp->writelog = &Task::writelog;

        ikcp_wndsize(kcp, 128, 128);

        switch (mode)
        {
        case 0:
            // 默认模式
            ikcp_nodelay(kcp, 0, 10, 0, 0);
            break;
        case 1:
            // 普通模式，关闭流控等
            ikcp_nodelay(kcp, 0, 10, 0, 1);
            break;
        case 2:
            // 启动快速模式
            // 第二个参数 nodelay-启用以后若干常规加速将启动
            // 第三个参数 interval为内部处理时钟，默认设置为 10ms
            // 第四个参数 resend为快速重传指标，设置为2
            // 第五个参数 为是否禁用常规流控，这里禁止
            //ikcp_nodelay(kcp, 0, 10, 0, 0);
            //ikcp_nodelay(kcp, 0, 10, 0, 1);
            //ikcp_nodelay(kcp, 1, 10, 2, 1);
            ikcp_nodelay(kcp, 1, 5, 1, 1); // 设置成1次ACK跨越直接重传, 这样反应速度会更快. 内部时钟5毫秒.

            kcp->rx_minrto = 10;
            kcp->fastresend = 1;
            break;
        default:
            printf("%d,%d 模式错误!\n", conv,mode);
        }

        printf("新建连接 %d\n", conv);
        return true;
    }

    // recv函数，主要是被server和client调用
    // udp套接字接收到数据之后，调用这个函数，该函数把数据放进kcp中进行处理
    // kcp处理完成之后会调用on_recv虚函数
    int recv(const char  * buf, int len)
    {
        // 把数据放进kcp中进行处理
        int nret = ikcp_input(kcp, buf, len);
        if (nret == 0)
        {
            nexttime = iclock();
            alivetime = nexttime + 10000;
        }
        return nret;
    }

    // 循环检测放在kcp中的数据是否已经处理完成，如果处理完成，那么调用on_recv虚函数
    void timerloop()
    {
        current = iclock();

        if (nexttime > current)
        {
            return;
        }

        nexttime = ikcp_check(kcp, current);
        if (nexttime != current)
        {
            return;
        }

        ikcp_update(kcp, current);
        //ikcp_flush(kcp);

        while (true) {
            int nrecv = ikcp_recv(kcp, buffer, sizeof(buffer));
            if (nrecv < 0)
            {
                if (nrecv == -3)
                {
                    printf("buffer太小 %d,%d\n", conv, sizeof(buffer));
                }
                break;
            }
            on_recv(buffer,nrecv);
        }
    }

    static int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
    {
        return ((udpsocket*)user)->sendto(buf, len);
    }

    static void writelog(const char *log, struct IKCPCB *kcp, void *user)
    {
        printf("%s\n", log);
    }

private:
    IUINT32 conv;
    ikcpcb *kcp;
    IUINT32 nexttime;
    IUINT32 current;
    IUINT32 alivetime;
    char buffer[65536];
};

#endif // TASK_H
