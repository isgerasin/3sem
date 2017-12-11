
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
#include <limits.h>

#include <sys/time.h>
#include <sys/resource.h>


#include "../def/check.h"


#define SIGACTION(sig, handler, set, flg) \
		act.sa_handler = handler; \
		act.sa_mask = set; \
		act.sa_flags = flg; \
		_(sigaction(sig, &act, &oact))

int counter = 0;
char bit = 0;


void hndl(int signo)
{
	exit(139);
}




int main(int argc, char const *argv[])
{
	pid_t pid = fork();
	if ( pid == 0)
	{
	//	*(int*)NULL = 7;
		fprintf(stderr, "END\n");
		exit(0);
	}
	while(1);
	
//	exit(3);
//	
	sigset_t set = {};
	struct sigaction act = {};
	struct sigaction oact = {};

	//(sigemptyset(&set));

	//SIGACTION(SIGSEGV,hndl,set, 0);

	while(1);
	//int i = atoi(argv[1]);
	//exit(i);
	return 0;
}


