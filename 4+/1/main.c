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
#include <signal.h>

#include "../../def/check.h"

int counter1 = 0;
int counter2 = 0;

void sa_handSIGUSR1(int signo)
{
	printf("Nooooooooooooo\n");
	//raise(signo);
	char buf[100] = {};
	scanf("%s", buf);
	printf(" USR1 %d\n", counter1++);
	return;
}

void sa_handSIGUSR2(int signo)
{
	printf("Nooooooooooooo\n");
	//raise(signo);
	char buf[100] = {};
	//scanf("%s", buf);
	printf(" USR2 %d\n", counter2++);
	return;
}

int StartClient()
{
	sigset_t set = {};
	sigset_t set1 = {};
	

	struct sigaction sigact = {};
	_(sigemptyset(&set));
	//set1 = set;
	//_(sigaddset(&set, SIGUSR2));
	sigact.sa_handler = sa_handSIGUSR1;
	sigact.sa_mask = set;

	_(sigaction(SIGUSR1, &sigact, NULL));
	sigact.sa_handler = sa_handSIGUSR2;
	sigact.sa_mask = set;
	_(sigaction(SIGUSR2, &sigact, NULL));
	while (1); 
	//_();
	return 0;
}

int StartServer(const char* name)
{
	int fd = 0;
	_(fd = open(name, O_RDONLY));

	close(fd);
	return 0;
}


int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		if (StartClient() < 0 )
		{
			perror("Err in CLient");
			return -1;
		}
	}
	else if (argc == 2)
	{
		if (StartServer(argv[1]) < 0)
		{
			perror("Error in Server");
			return -1;
		}
	}
	else
		printf("Error args\n");
	return 0;
}
