
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "../../def/check.h"

#define SIGACTION(sig, handler, set, flg) \
		act.sa_handler = handler; \
		act.sa_mask = set; \
		act.sa_flags = flg; \
		_(sigaction(sig, &act, &oact))

int counter = 0;
char bit = 0;

void hndlSIGUSR(int signo)
{
	//fprintf(stderr, "IMSIG\n");
	switch(signo)
	{
		case SIGUSR1:
			bit = 0;
			break;
		case SIGUSR2:
			bit = 1;
			break;
		default:
			bit = -1;
	}
}

void hndl(int signo)
{
	//fprintf(stderr, "HI\n");
	return;
}

int StartClient(pid_t pid)
{
	sigset_t set = {};
	struct sigaction act = {};
	struct sigaction oact = {};
	char byte = 0;
	_(sigaddset(&set, SIGUSR1));
	_(sigaddset(&set, SIGUSR2));

	SIGACTION(SIGUSR1, hndlSIGUSR, set, 0);
	SIGACTION(SIGUSR2, hndlSIGUSR, set, 0);
	SIGACTION(SIGCHLD, hndlSIGUSR, set, 0);

	_(sigemptyset(&set));

	while(1)
	{
		for (int i = 0; i < 8; i++)
		{
			sigsuspend(&set);
			if (bit < 0)
				return 0;
			byte = (byte << 1) | bit;
			_(kill(pid, SIGUSR1));
		}
		_(write(STDOUT_FILENO, &byte, sizeof(byte)));
	}

	return 0;
}

int StartServer(const char* name)
{
	sigset_t set = {};
	struct sigaction act = {};
	struct sigaction oact = {};
	pid_t ppid = getppid();
	char byte = 0;
	size_t nread = 0;
	int fd = 0;

	_(fd = open(name, O_RDONLY));
	_(sigemptyset(&set));
	SIGACTION(SIGUSR1, hndl, set, 0);
	
	do
	{
		_(nread = read(fd, &byte, sizeof(byte)));
		for (int i = 0; i < 8 * sizeof(byte); i++)
		{
			switch(byte & (1 << 8))
			{
				case 0:
					_(kill(ppid, SIGUSR1));
					break;
				case (1 << 8):
					_(kill(ppid, SIGUSR2));
					break;
				default:
					return -1;
			}
			byte = byte << 1;
			_(sigemptyset(&set));
			alarm(5);
			sigsuspend(&set);
			alarm(0);

		}
	}while(nread);

	close(fd);
	return 0;
}


int main(int argc, char* argv[])
{
	
	if (argc != 2)
	{
		printf("Use: %s <filename>\n", argv[0]);
		return -1;
	}
	sigset_t set = {};
	_(sigemptyset(&set));
	_(sigaddset(&set, SIGUSR1));
	_(sigaddset(&set, SIGUSR2));
	_(sigprocmask(SIG_BLOCK, &set, NULL));
	errno = 0;
	pid_t pid = fork();
	if ( pid > 0 )
	{
		if (StartClient(pid) < 0 )
		{
			perror("Err in CLient");
			return -1;
		}
	}
	else if ( pid == 0 )
	{
		if (StartServer(argv[1]) < 0)
		{
			perror("Error in Server");
			return -1;
		}
	}
	else
	{
		perror("Error in fork()\n");
		return -1;
	}
	return 0;
}
