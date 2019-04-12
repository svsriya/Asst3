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

/* To set up client:
 *  1. getaddrinfo on server
 *  2. make call to socket
 *  3. make call to connect
*/

int main( int argc, char** argv )
{
	char* command;
	char* ip;
	char* port;
	struct addrinfo hints;
	struct addrinfo * result;
	int res;
	int sd; 
	// check the args
	command = argv[1];
	
	if( strcmp(command, "configure") )
	{
		// setting ip and port environment variables 
		ip = argv[2];
		port = argv[3];
		if( setenv( "IP_ADDRS", ip, 1 ) == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			return 0;
		}
		if( setenv( "PORT_NUM", port, 1 ) == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_C0LOR_RESET, errno, strerror(errno), __LINE__);
			return 0; 
		}
		
	}
	else // temporarily just testing that connecting to server works
	{
		// call getaddrinfo
		hints.ai_flags = 0;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		if( (res = getaddrinfo( getenv("IP_ADDRS"), getenv("PORT_NUM"), &hints, &result )) != 0 )
		{
			printf( ANSI_COLOR_CYAN stderr, "getaddrinfo: %s Line#: %d\n", gai_strerror(res) ANSI_COLOR_RESET, __LINE__ );
			exit(2);
		}   
		
		if( ( sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}
	
		if( connect( sd, result->ai_addr, result->ai_addrlen ) == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			close(sd);
			exit(2);
		}
		// CONNECTED
	}
	return 0;
}
