#ifndef GET_TIME_H
#define GET_TIME_H


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

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

#include <stdint.h>

/* get system time */
void itimeofday(long *sec, long *usec);
/* get clock in millisecond 64 */
int64_t iclock64(void);

uint32_t iclock();
/* sleep in millisecond */
void isleep(unsigned long millisecond);

#endif // GET_TIME_H
