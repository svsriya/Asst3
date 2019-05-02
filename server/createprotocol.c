#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "zpipe.c"
#include "createprotocol.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#include <assert.h>
#include "zlib.h"
//#include <bsd/unistd.h>





void createProtocol (char **, char **, int);
int openrequested (char **, char **);
void sendProtoco();
void destroyProtocolFile();
void traverseDir(char **);
void createGzip();

void createGzip(){

	printf("trying to create gzip\n");
	char * path = "./protocol.txt";
	FILE * fd_pt;

	if( (fd_pt = fopen("./protocol.txt", "r+")) == NULL){
		printf( ANSI_COLOR_RED "Errno: %d Message %s Line %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); exit(2);
	}

	FILE * fd_gzz;
	if( (fd_gzz = fopen("./protoc.gz", "w+")) == NULL){
		printf( ANSI_COLOR_RED "Errno: %d Message %s Line %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); exit(2);
	}

	int res = def(fd_pt, fd_gzz, -1);
	if (res != Z_OK)
            zerr(res);


//	fclose(fd_pt); fclose(fd_gzz);
//	printf("res of def: %d\n", res);
//	free(gzpath);
	return;

}


void traverseDir( char ** path){
	DIR * dirp = opendir(*path);
	int fd;
	struct dirent * dp;
	struct stat filestat;
	dp = readdir(dirp);
	dp= readdir(dirp);	
	int opd = 2;

	int pfd;
	char* protocpath = "./protocol.txt";
	if( (pfd = open(protocpath,  O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
	}

	while( (dp = readdir(dirp)) != NULL ){
		printf("dirp name: %s\n", dp->d_name);
		char * tmp = malloc(strlen(*path) + strlen(dp->d_name) +1 +1);
		tmp[0] = '\0';
		snprintf(tmp, strlen(*path) + strlen(dp->d_name) +1 +1, "%s/%s",*path, dp->d_name);
		if(stat(tmp, &filestat) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		}else{
			if(S_ISDIR(filestat.st_mode) ==1){ //directory encountered
				// append "-4:bytesname:name:" to protocol
				printf(ANSI_COLOR_YELLOW "directory detected\n" ANSI_COLOR_RESET);
				char bytesname[10];
				sprintf( bytesname, "%d", strlen(tmp) );
					
				//NOW WRITE TO PROTOCOLFILE!!!!
				char * append = malloc(3 + strlen(bytesname)+1 + strlen(tmp)+1 +1);
				append[0]= '\0';
				strcat( append, "-4:");
				strcat( append, bytesname); strcat (append, ":");
				strcat( append, tmp); strcat(append, ":");
				
		//		printf("append: %s\n", append);

				int res;
				res = write(pfd, append, strlen(append));
				if(res == -1){
					printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					free(append); free(tmp); exit(2);
				}
				free(append);
				traverseDir(&tmp);

			}else{ //file encountered
				//append "-3:bytesname:name:bytescontent:content:" to protocol
				//get filename bytes

				printf(ANSI_COLOR_YELLOW "file  detected\n" ANSI_COLOR_RESET);
				char bytesname[10];
				sprintf( bytesname, "%d", strlen(tmp) );
					
				//get file bytes data
				char bytesdata[10];
				sprintf( bytesdata, "%d", filestat.st_size );
					
				//open + read in file contents
				int ffd = open(tmp, O_RDONLY);
				if(ffd == -1){
					printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					exit(2);
				}
				
				char * buf = malloc(filestat.st_size+1);
				buf[0] = '\0';
				if( read(ffd, buf, filestat.st_size) == -1){
					printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					free(buf); exit(2);
				}

				//NOW WRITE TO PROTOCOLFILE!!!!
				char * append = malloc( 3 + strlen(bytesname)+1 + strlen(tmp)+1 + strlen(bytesdata)+1 + strlen(buf)+1+1);
				append[0]= '\0';
				strcat( append, "-3:");
				strcat( append, bytesname); strcat (append, ":");
				strcat( append, tmp); strcat(append, ":");
				strcat( append, bytesdata); strcat(append, ":");
				strcat( append, buf); strcat(append, ":");	
					
				printf("appendfile: %s\n", append);

				int res;
				res = write(pfd, append, strlen(append));
				if(res == -1){
					printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					free(append); exit(2);
				}else{
					free(append); free(buf);
					printf("file --> protocol successful \n");
				//	return;
				}
			}
			free(tmp);
		}
		opd++;
	}
	//if opd<=2, means directory is empty --> just return, protocol already written to before traverseDir called
	return;
}

void destroyProtocolFile(){
	char * cmd = "rm -rf protocol.txt";	
	if( (system(cmd)) == -1){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);
	}
}

int openrequested (char ** path, char ** cmd){
	
	//printf("requested_path: %s\n, path");	
	DIR * dirp;
	int fd;
	struct dirent * dp;
	struct stat filestat;

	printf("requested_path: %s\n",*path);	
	printf("entered openrequested!\n");
	//create protocol file to append to --> will be destroyed once delivered to client
	int pfd;
	char* protocpath = "./protocol.txt";
	if( (pfd = open(protocpath, O_CREAT | O_APPEND | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);
	}

	if( (dirp = opendir(*path)) == NULL){//tried to open directory failed
		if(errno = ENOTDIR){
			fd = open(*path, O_RDONLY);
			if(fd == -1){
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			}else{  //file successfully opened
				if(stat(*path, &filestat) == -1){
					( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
				}else{ // path sent in was a file. create the protocol
					//get filename bytes
					char bytesname[10];
					sprintf( bytesname, "%d", strlen(*path) );
					
					//get file bytes data
					char bytesdata[10];
					sprintf( bytesdata, "%d", filestat.st_size );
					
					//read in file contents

					char * buf = malloc(filestat.st_size+1);
					buf[0] = '\0';
					if( read(fd, buf, filestat.st_size) == -1){
						printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
						exit(2);
					}

					//NOW WRITE TO PROTOCOLFILE!!!!
					char * append = malloc(9 + 3 + strlen(bytesname)+1 + strlen(*path)+1 + strlen(bytesdata)+1 + strlen(buf)+1+1);
					append[0]= '\0';
					if(strcmp(*cmd, "currver") == 0){
						strcat(append, "sendsman:");
					}else if(strcmp(*cmd, "history")==0){
						strcat(append, "sendhist:");
					}else{
						strcat( append, "sendfile:");
					}
					strcat( append, "-3:");
					strcat( append, bytesname); strcat (append, ":");
					strcat( append, *path); strcat(append, ":");
					strcat( append, bytesdata); strcat(append, ":");
					strcat( append, buf); strcat(append, ":");	
					
//					printf("append: %s\n", append);

					int res;
					res = write(pfd, append, strlen(append));
					if(res == -1){
						printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
						exit(2);
					}else{
						free(append);
						return pfd;
					}
				}
			 
			}
		}else{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); exit(2);
		}
	
	}else{ //is directory
		/*traverseDir
			//make sure to include ./
			//senddir:-4:bytesname:name:
			//continue recursing, call traverseDir here
		*/

		printf("client requested a directory!\n");
		char bytesname[10];
		sprintf( bytesname, "%d", strlen(*path) );
					
		//NOW WRITE TO PROTOCOLFILE!!!!
		char * append = malloc(8 + 3 + strlen(bytesname)+1 + strlen(*path)+1 +1);
		append[0]= '\0';
		strcat( append, "senddir:");
		strcat( append, "-4:");
		strcat( append, bytesname); strcat (append, ":");
		strcat( append, *path); strcat(append, ":");
				
		printf("append_dir: %s\n", append);

		int res;
		res = write(pfd, append, strlen(append));
		if(res == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}else{
			free(append);
			printf("making call to traverseDir...\n");
			traverseDir(path);	
			return pfd;
		}		
	} 

	//close out dirp here :D
	closedir(dirp);
}


void createProtocol (char ** path, char ** cmd, int sockd){
	printf("pATHO: %s\n", *path);
	int openres = openrequested (path, cmd);
	printf("openres done\n");	
	struct stat filestat;

	int pfd;
	char* protocpath = "./protocol.txt";
	
	createGzip();
	//char * protocpath = "./protoc.gz";
        if( (pfd = open(protocpath, O_RDONLY))==-1){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
	}

	if(stat(protocpath, &filestat) == -1){
		(ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);
	}

	//printf("size of gz: %d\n", filestat.st_size);
	//printf("gz MODE: %o\n", filestat.st_mode);
	char * buf = malloc(filestat.st_size+1);
		buf[0] = '\0';
	
	if( read(pfd, buf, filestat.st_size) == -1){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);
	}

	//printf("protoc: %s\n", buf);
	//sendProtocol to client			
	char len[10];
	sprintf(len, "%010d", strlen(buf)); 
	int i = 0;
	int written;

	while( i != strlen(len)){
		if( (written = write(sockd, len+i, strlen(len)-i)) == -1){
               		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                	exit(2);
		}
		i+=written;
	}	
	
	printf(ANSI_COLOR_YELLOW "%s bytes to read send to client\n" ANSI_COLOR_RESET, len);

	i=0;
	while( i!= strlen(buf)){
		if( (written = write(sockd, buf+i, strlen(buf)-i)) == -1){
               		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                	exit(2);
		}
		i+=written;
	}

	printf(ANSI_COLOR_YELLOW "protocol sent to client\n" ANSI_COLOR_RESET, len);
	free(buf);	
	return;
}
