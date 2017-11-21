#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdlib.h>

//#include "../def/check.h"

#define _( var ) \
			fprintf(stderr, "%d| (%d) %s\n", __LINE__, (var), #var );

int main(int argc, char const *argv[])
{
	
	char buf[] = "vvvvvvvvvv";
	/*
	//_(creat("hello", 01));
	//_(open("test", O_RDONLY | O_CREAT))
	int fd = 0;
	mkfifo("fifo", 666);
	//fprintf(stderr, "%d| (%o) %s\n", __LINE__, umask(0), "#var" );
	_(fd = open("fifo", O_RDWR));
	_(write(fd, "aaa", 3));
	_(close(fd));
	_(fd = open("fifo", O_RDONLY | O_NONBLOCK));
	_(write(1, buf, read(fd, buf, 3)));
	*/
	_(ftok(0, 0));

	int fd = 0;
	mkfifo("fifo", 666);
	
	fd = open("fifo", O_RDWR);
	write(fd, "aaa", 3);
	
	fd = open("fifo", O_RDONLY | O_NONBLOCK);
	_(write(1, buf, 0));
	return 0;
}