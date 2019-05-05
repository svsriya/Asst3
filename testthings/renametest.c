#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int main( int argc, char** argv )
{
	char* oldpath = argv[1];
	char* newpath = argv[2];
	if( rename( oldpath, newpath ) == -1 ){
		printf( "Errno: %d Msg: %s Line#: %d\n", errno, strerror(errno), __LINE__ );
	}	
	else printf( "success!\n" );
	return 0;
}
