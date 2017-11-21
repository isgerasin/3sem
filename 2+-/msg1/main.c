#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
//#include <sys/wait.h>

#include <errno.h>
#define _( var ) do{\
		errno = 0;\
		if ( (var) < 0 ){\
			perror( #var ); exit(-1);} \
		}while(0)

struct msg_t
{
	long type;
	struct 
	{
		long number;
	}content;
};

int Child(long i, int fd)
{
	struct msg_t myMsg = {};

	if ( i != 1)
		_(msgrcv(fd, &myMsg, sizeof(myMsg), i-1, 0));
	
	fprintf(stderr, "%ld ", i);
	myMsg.type = i;

	_(msgsnd(fd, &myMsg, sizeof(myMsg.content), 0));
	exit(0);
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Bad args\n");
		return -1;
	}
	else
	{
		int fd = 0;
		_(fd = msgget(IPC_PRIVATE, 0666));
		char* endptr = NULL;
		long n = strtol(argv[1], &endptr, 10);
		int i = 0;
		pid_t pid = 0;
		for (i = 1; i <= n; i++)
		{
			if ((pid = fork()) < 0)
				break;
			if (pid == 0)
				_(Child(i, fd));
			
		}
		int olderr = errno;
		struct msg_t myMsg = {};
		if (n > 0)
			_(msgrcv(fd, &myMsg, sizeof(myMsg), i-1, 0));
		if ( pid < 0 )
		{
			errno = olderr;
			perror("Error");
		}
		_(msgctl(fd, IPC_RMID, NULL));
	}
}
