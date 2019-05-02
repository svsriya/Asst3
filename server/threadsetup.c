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
	int is_done;	
	int is_occ;
}th_container;
