/* 
 * File : dcomm.h 
 */ 

#ifndef _RECEIVER_H_ 
#define _RECEIVER_H_ 

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define MIN_UPPERLIMIT 5
#define MAX_LOWERLIMIT 2

#define bzero(p, size) (void)memset((p), 0 , (size))
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500
/* Define receive buffer size */
#define RXQSIZE 8


/* FUNCTIONS AND PROCEDURES */
static Byte *rcvchar(int sockfd, QTYPE *queue);

static Byte *q_get(QTYPE *, Byte *);

void *childRProcess(void * threadid);

void error(const char* message);

#endif