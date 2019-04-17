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

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int main(int argc, char ** argv){

	//goal: tar testdir
	//ex argv "tar cfz Asst0.tgz Asst0"
	char * cmd  = "tar";
	//char * args = "tar cvf testdir.tgz testdir";
	char * args[5];
	args[0] = "tar";
	args[1] = "cvf";
	args[2] = "test.txt.tgz";
	args[3] = "test.txt";
	args[4] = NULL;

	//fork
	pid_t child_id = fork();
	if(child_id == -1 ){
		printf("Fork failed.\n");
	}else if(child_id == 0 ){
		printf("I am the child.\n");
		
		if( execvp(cmd, args) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		}

	}else{
		printf("I am the parent.\n");
		int status;

		waitpid(child_id, &status, 0);
	//	return EXIT_SUCCESS;
	}

	//goal: untar
/*	char * args1[4];
	args1[0] = "tar";
	args1[1] = "-xvf";
	args1[2] = "testdir1.tgz";
	args1[3] = "NULL";
	pid_t child2_id = fork();
	if(child2_id == -1 ){
		printf("Fork2 failed.\n");
	}else if(child2_id == 0 ){
		printf("I am the child2.\n");
		if( execvp(cmd, args1) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		}
	}else{
		printf("I am the parent.\n");
		int status;
		waitpid(child2_id, &status, 0);
		// return EXIT_SUCCESS;
	}
*/
	return 0;	
}
