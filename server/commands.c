#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <openssl/sha.h>
#include <fcntl.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int commit( char*, int );

int commit( char* projname, int csd )
{
/*	need to read in the proj name
 *	check that the project exists, write error or success to client
 * 	currentversion will also be read in with projname
 * 	Error can also be sent from the client
 *  	make a call to currver to send the manifest
 *  	wait for client to send either .Commit file or error
*/
	char buflen[10];
        if( read(cfd, buflen, sizeof(buflen)) == -1){
                printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }

        int len = charToInt((char*)buflen);
        printf("Number of bytes_projname being recieved: %d\n", len);
	
	char* bufread2 = (char*)malloc( len + 1 );
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){
                rdres = read( cfd, bufread2+i, len-i );
                printf("rdres: %d\n", rdres);
                if( rdres == -1 ){
                        printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        return -1;
                }
                i += rdres;
        }
        bufread2[len] = '\0';
        printf( "projname received from client: %s\n", bufread2);

	if( searchProj(bufread2) == -5){
		char len[10];

                char * err = (char *)malloc(sizeof(char)* 27);
                strcat(err,  "Error: project not found.\n");
                sprintf(len, "%010d", strlen(err));
                int i = 0;
                int written;

                writeToSocket(&err, cfd);

                printf(ANSI_COLOR_YELLOW "err sent to client\n" ANSI_COLOR_RESET, len);
                free(bufread2);
                free(err);
                return -1;
        }
	else{	//write succes to client
		char* suc = "projfound";
		writeToSocket( &err, cfd );
		printf( ANSI_COLOR_YELLOW "Projfound sent to the client\n" ANSI_COLOR_RESET );
		free(bufread2);
	}
	// read in current version
	
}
