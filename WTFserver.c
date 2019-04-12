#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/sockets.h>
#include <netdb.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
 * Setting up the server:
 *  1. getaddrinfo on itself
 *  2. make call to the socket
 *  3. make call to bind
 *  4. make call to listen
 *  5. call to accept (loop to accept multiple connections
*/

int main( int argc, char** argv )
{
	
	return 0; 
}
