#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

//i#include "sendrcvfile.h"
//#include "sendrcvfile.c"

//#include "createprotocol.h"
//#include "createprotocol.c"


typedef struct th_container{
	pthread_t thread_id;
	int is_done; // -1=available, 0=running, 1=done/waiting to be canceled&joined on
	int cfd;
}th_container;


typedef struct sig_waiter_args{
	sigset_t *set;
	int sockfd;
}sig_waiter_args;


typedef struct PROJECT{
	pthread_mutex_t proj_lock;
	char * projname;
	pthread_mutex_t numthread_lock;
	int num_threads;
	int destroy;
	struct PROJECT* next;
}PROJECT; 



