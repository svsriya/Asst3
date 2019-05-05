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
	//int is_occ;
	int cfd;
}th_container;


typedef struct sig_waiter_args{
	sigset_t *set;
	int sockfd;
}sig_waiter_args;

