#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

/*
 * 同步队列
 */
#include <deque>
#include <pthread.h>

using namespace std;

template<typename T>
class SyncQueue{
private:
    pthread_mutex_t lock;
    pthread_cond_t cond;
    deque<T> inner_queue;

    void Lock(){
        pthread_mutex_lock(&lock);
    }

    void Unlock(){
        pthread_mutex_unlock(&lock);
    }

    void Wait(){
        while(inner_queue.empty()){
            pthread_cond_wait(&cond,&lock);
        }
    }

    void Signal(){
        pthread_cond_signal(&cond);
    }

public:
    SyncQueue(){
        pthread_mutex_init(&lock,NULL);
        pthread_cond_init(&cond,NULL);
    }

    ~SyncQueue(){
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&lock);
    }

    void Push(T info){
        Lock();
        bool empty = inner_queue.empty();
        inner_queue.push_back(info);
        if(empty){
            Signal();
        }
        Unlock();
    }

    T Pop(){
        Lock();
        Wait();
        T info = inner_queue[0];
        inner_queue.pop_front();
        Unlock();
        return info;
    }

    size_t Size(){
        Lock();
        size_t size = inner_queue.size();
        Unlock();
        return size;
    }

    bool Empty(){
        Lock();
        bool empty = inner_queue.empty();
        Unlock();
        return empty;
    }
};

#endif // SYNC_QUEUE_H
