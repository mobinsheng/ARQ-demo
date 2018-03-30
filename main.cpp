#include <iostream>
#include "arq_demo.h"
using namespace std;



int main()
{
    pthread_t t1,t2;
    pthread_create(&t1,NULL,server_thread,NULL);
    pthread_create(&t2,NULL,client_thread,NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    return 0;
}
