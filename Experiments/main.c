/*
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
{/*
	pid_t pid = fork();
	if ( pid == 0)
	{
		*(int*)NULL = 7;
		fprintf(stderr, "END\n");
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
*/



#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/select.h>
#include <sys/wait.h>



#include "../def/check.h"

#define LOCSIZE ((size_t) 128*1024)
//PIPE_BUF
#define MSGSIZE (512*3)//PIPE_BUF//(512*3)
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

typedef enum status_t
{
	EMPTY,
	PARTLY,
	FULL
}status_t;

typedef struct channel_t
{
	size_t sizeBuf;
	long long int n;

	int pipeIn[2];
	int pipeOut[2];
	char  locBuf[LOCSIZE];

	char* buf;
	size_t beginBuf;
	size_t endBuf;
	status_t status;
} channel_t;

typedef struct server_t
{
	long long int N;
	channel_t** chnls;

	int inputFd;
	pid_t lastChild;

	fd_set inFds;
	int maxInFd;

	fd_set outFds;
	int maxOutFd;

} server_t;

int ChannelOk(channel_t* this);

int ChannelDtor(channel_t* this);



channel_t* ChannelCtor(size_t sizeBuf, long long int n)
{
	errno = 0;
	channel_t* this = calloc(1, sizeof(channel_t));
	if (this == NULL)
		return NULL;

	this->sizeBuf = sizeBuf;
	this->n = n;
	this->beginBuf = 0;
	this->endBuf = 0;
	this->status = EMPTY;

	if (pipe(&(this->pipeIn[0])) < 0)
		return NULL;

	if (pipe(&(this->pipeOut[0])) < 0)
		return NULL;

	this->buf = (char*) calloc(sizeBuf, sizeof(*this->buf));

	if (this->buf == NULL)
	{
		ChannelDtor(this);
		return NULL;
	}

	if (!ChannelOk(this))
		this = NULL;
	
	return this;
}

int ChannelDtor(channel_t* this)
{
	if (this == NULL)
		return 0;
	free(this->buf);
	this->buf = NULL;

	close(this->pipeIn[0]);
	close(this->pipeIn[1]);
	close(this->pipeOut[0]);
	close(this->pipeOut[1]);

	this->pipeIn[0] = -1;
	this->pipeIn[1] = -1;
	this->pipeOut[0] = -1;
	this->pipeOut[1] = -1;

	this->sizeBuf = 0;
	free(this);
	return 0;
}

int ChannelOk(channel_t* this)
{
	return  this &&
			(this->buf != 0) &&
			(this->sizeBuf != 0) &&
			(this->pipeIn[0] != -1) &&
			(this->pipeIn[1] != -1) &&
			(this->pipeOut[0] != -1) &&
			(this->pipeOut[1] != -1);
}

int ChannelDump(channel_t* this)
{
	fprintf(stderr, "======================================================\n");
	if (this == NULL)
	{
		fprintf(stderr, "NULL\n======================================================\n");
		return 0;
	}

	fprintf(stderr,
			"Channel_t [%p] %s\n"
			"\tsizeBuf = %lu\n"
			"\tn = %lli\n"
			"\t\tpipeIn[0] = %i\n"
			"\t\tpipeIn[1] = %i\n"
			"\t\tpipeOut[0] = %i\n"
			"\t\tpipeOut[1] = %i\n"
			"\tbuf = [%p]\n"
			"\t\tstatus = %d\n"
			"\t\tbeginBuf = %li\n"
			"\t\tendBuf = %li\n"
			"\t\tsizeBuf = %li\n{\n",
			this, (ChannelOk(this) ? "ok" : "ERROR!!!"), this->sizeBuf, this->n, this->pipeIn[0], this->pipeIn[1],this->pipeOut[0], this->pipeOut[1], this->buf, this->status, this->beginBuf, this->endBuf, this->sizeBuf);

	write(STDERR_FILENO, this->buf, this->sizeBuf);
	fprintf(stderr, "\n}\n\tlocBuf[%lu]\n{\n", LOCSIZE);
	write(STDERR_FILENO, this->locBuf, LOCSIZE);

	fprintf(stderr, "\n}\n======================================================\n");
	return 0;
}

int StartChild(channel_t* chnl)
{
	_(close(chnl->pipeIn[1]));
	_(close(chnl->pipeOut[0]));

	ssize_t nread = 1;
	//fprintf(stderr, "%d\n", chnl->pipeIn[0]);
	while(nread)
	{
		// sleep(chnl->n);
		_(nread = read(chnl->pipeIn[0], chnl->locBuf, MSGSIZE));
		//fprintf(stderr, "%c%c  %li\n_________", chnl->locBuf[0], chnl->locBuf[1], nread);
		//*(ssize_t)chnl->locBuf = nraed;
		// ChannelDump(chnl);
		//ChannelDump(chnl);
		//fprintf(stderr, "%lli\n", nread);

		_(write(chnl->pipeOut[1], chnl->locBuf, nread));
	}
	//*(ssize_t)chnl->locBuf = -1;
	//write(chnl->pipeOut[1], chnl->locBuf, sizeof(nread))

/*
	struct stat sbIn = {}; 
	struct stat sbOut = {}; 
	fstat(chnl->pipeIn[0], &sbIn);
	fstat(chnl->pipeOut[1], &sbOut);
	fprintf(stderr, "in = %d out = %d\n", sbIn.st_nlink, sbOut.st_nlink);*/

	//ChannelDump(chnl);
	//sleep(chnl->n);

	close(chnl->pipeIn[0]);
	close(chnl->pipeOut[1]);
	//PL;
	exit(0);
}

int ServerDtorN(server_t* this, long long n)
{

	//this->N = 0;
	this->maxOutFd = 0;
	this->maxInFd = 0;
	FD_ZERO(&this->outFds);
	FD_ZERO(&this->inFds);

	for (long long i = this->N; i > n; i--)
	{
		// PL;
		// fprintf(stderr, "%lli\n", i);
		ChannelDtor(this->chnls[i]);
	}

	free(this->chnls);
	free(this);
	return 0;
}

int ServerDump(server_t* this)
{
	fprintf(stderr, "--------------------------------------------\n");
	if (this == NULL)
	{
		fprintf(stderr, "NULL\n--------------------------------------------\n");
		return 0;
	}
	fprintf(stderr, "server_t [%p]\n"
					"\tN = %lli\n", this, this->N);

	for (size_t i = this->N; i >= 0;i--)
	{
		ChannelDump(this->chnls[i]);
	}

	fprintf(stderr, "--------------------------------------------\n");
	return 0;
}

server_t* ServerCtor(long long int N, int inputFd)
{
	errno = 0;

	server_t* this = calloc(1, sizeof(*this));
	if (this == NULL)
		return NULL;


	this->chnls = (channel_t**) calloc(N+1, sizeof(*(this->chnls)));
	if (this->chnls == NULL)
		return NULL;


	FD_ZERO(&this->inFds);
	FD_ZERO(&this->outFds);
	this->maxInFd = 0;
	this->maxOutFd = 0;
	this->N = N;
	this->inputFd = inputFd;
	this->lastChild = 0;

	channel_t** chnls = this->chnls; 

	for (long long int i = N; i >= 1; i--)
	{
		_((size_t) (chnls[i] = ChannelCtor( ((size_t) pow(3, i)) *512, i)));
/*
		struct stat sb1 = {};
		fstat(chnls[i]->pipeIn[0], &sb1);
		struct stat sb2 = {};
		fstat(chnls[i]->pipeIn[1], &sb2);
		struct stat sb3 = {};
		fstat(chnls[i]->pipeOut[0], &sb3);
		struct stat sb4 = {};
		fstat(chnls[i]->pipeOut[1], &sb4);
		ChannelDump(chnls[i]);
		fprintf(stderr, "%d %i\n",chnls[i]->pipeIn[0], sb1.st_ino);
		fprintf(stderr, "%d %i\n", chnls[i]->pipeIn[1], sb2.st_ino);
		fprintf(stderr, "%d %i\n", chnls[i]->pipeOut[0], sb3.st_ino);
		fprintf(stderr, "%d %i\n",chnls[i]->pipeOut[1], sb4.st_ino);
*/

		pid_t pid = 0;
		_(pid = fork());
		_(pid);
		if (pid == 0)
		{
			channel_t* chnl = chnls[i];

			if (i == N)
			{
				close(chnl->pipeIn[0]);
				chnl->pipeIn[0] = inputFd;
			}
			else if(i == 1)
			{
				close(chnl->pipeOut[1]);
				chnl->pipeOut[1] = STDOUT_FILENO;
			}
			//PL;
			//fprintf(stderr, "%lli\n", i);

			ServerDtorN(this, i);
			//ServerDump(this);
			_(StartChild(chnl));
			exit(0);
		}
		//sleep(1);
		if (i == 1)
			this->lastChild = pid;
		
		// _(close(chnls[i]->pipeIn[0]));
		// _(close(chnls[i]->pipeOut[1]));
		
		//close(chnls[i]->pipeIn[1]);
		//close(chnls[i]->pipeOut[0]);
		/*
		_(fcntl(chnls[i]->pipeIn[1], F_SETFL, O_WRONLY | O_NONBLOCK));
		FD_SET(chnls[i]->pipeIn[1], &this->inFds);
		this->maxInFd = max(this->maxInFd, chnls[i]->pipeIn[1]);
		
		_(fcntl(chnls[i]->pipeOut[0], F_SETFL, O_RDONLY | O_NONBLOCK));
		FD_SET(chnls[i]->pipeOut[0], &this->outFds);
		this->maxOutFd = max(this->maxOutFd, chnls[i]->pipeOut[0]);*/
	}

	for (long long int i = N; i >= 1; i--)
	{
		/*
		struct stat sbIn = {};
		struct stat sbOut = {};
		fstat(chnls[i]->pipeIn[1], &sbIn);
		fstat(chnls[i]->pipeOut[0], &sbOut);
		fprintf(stderr, "in = %d out = %d\n", sbIn.st_nlink, sbOut.st_nlink);
		*/
		_(close(chnls[i]->pipeIn[0]));
		_(close(chnls[i]->pipeOut[1]));
		//close(chnls[i]->pipeIn[1]);
		//close(chnls[i]->pipeOut[0]);

		_(fcntl(chnls[i]->pipeIn[1], F_SETFL, O_WRONLY | O_NONBLOCK));
		FD_SET(chnls[i]->pipeIn[1], &this->inFds);
		this->maxInFd = max(this->maxInFd, chnls[i]->pipeIn[1]);
		
		_(fcntl(chnls[i]->pipeOut[0], F_SETFL, O_RDONLY | O_NONBLOCK));
		FD_SET(chnls[i]->pipeOut[0], &this->outFds);
		this->maxOutFd = max(this->maxOutFd, chnls[i]->pipeOut[0]);

		;
		
	}

	FD_CLR(this->chnls[N]->pipeIn[1], &this->inFds);
	close(chnls[N]->pipeIn[1]);

	FD_CLR(chnls[1]->pipeOut[0], &this->outFds);
	close(chnls[1]->pipeOut[0]);


	//while(1);
	return this;
}

ssize_t ServerWrite(channel_t* this, long long i)
{
	/*
	size_t nwrite = 0;
	if (this->chnls[i+1]->status != EMPTY)
	{
		_(nwrite = write(this->chnls[i]->pipeIn[1], this->chnls[i+1]->buf + this->chnls[i+1]->beginBuf, MSGSIZE));
		this->chnls[i+1]->beginBuf = (this->chnls[i+1]->beginBuf + nwrite) % this->chnls[i+1]->sizeBuf;

		if (this->chnls[i+1]->beginBuf == this->chnls[i+1]->endBuf)
			this->chnls[i+1]->status = EMPTY;
		else
			this->chnls[i+1]->status = PARTLY;
	}
	*/

	ssize_t nwrite = 0;

	channel_t* chnl = this; ///////////////////////////////////////// Achtung!!!!!!11

	if (chnl->status == EMPTY)
		return -1;

	if (chnl->endBuf > chnl->beginBuf)
	{
		_(nwrite = write(chnl->pipeIn[1], chnl->buf + chnl->beginBuf, min(MSGSIZE, chnl->endBuf - chnl->beginBuf)));
		chnl->beginBuf += nwrite;
	}
	else
	{
		_(nwrite = write(chnl->pipeIn[1], chnl->buf + chnl->beginBuf, min(MSGSIZE, chnl->sizeBuf - chnl->beginBuf)));
		chnl->beginBuf = (chnl->beginBuf + nwrite) % chnl->sizeBuf;

		if (chnl->beginBuf == 0)
			_(nwrite += chnl->beginBuf = write(chnls->pipeIn[1], chnl->buf + chnl->beginBuf, MSGSIZE - nwrite));
	}

	if (chnl->beginBuf == chnl->endBuf)
		chnl->status = EMPTY;
	else
		chnl->status = PARTLY;

	return nwrite;
}

ssize_t ServerRead(channel_t* this, long long i)
{
	/*
	size_t nread = 0;
	if (this->chnls[i]->status != FULL)
	{
		_(nread = read(this->chnls[i]->pipeOut[0], this->chnls[i]->buf + this->chnls[i]->endBuf, MSGSIZE));
		this->chnls[i]->endBuf = (this->chnls[i]->beginBuf + MSGSIZE) % this->chnls[i]->sizeBuf;

		if (this->chnls[i]->beginBuf + MSGSIZE > this->chnls[i]->endBuf)
			this->chnls[i]->status = FULL;
		else
			this->chnls[i]->status = PARTLY;
	}
	else 
		nread = 1;
	*/
	
	ssize_t nread = 0;
	PL;
	channel_t* chnl = this; ////////////////////////////////
	PL;

	if (chnl->status == FULL)
		return -1;
	/*
	if (chnl->beginBuf - chnl->endBuf >= MSGSIZE)
	{
		_(nread = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, MSGSIZE));
		chnl->endBuf += nread;
	}
	else if (chnl->sizeBuf - chnl->endBuf >= MSGSIZE)
	{
		_(nread = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, MSGSIZE));
		chnl->endBuf = (chnl->endBuf + nread) % chnl->sizeBuf;
	}
	else
	{
		_(nread = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, min(chnl->sizeBuf - chnl->endBuf, MSGSIZE)));
		chnl->endBuf = (chnl->endBuf + nread) % chnl->sizeBuf;

		if (chnl->endBuf == 0)
			_(nraed += chnl->endBuf = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, MSGSIZE - nread));
	}
	*/

	if (chnl->beginBuf - chnl->endBuf > 0)
	{
		_(nread = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, min(chnl->beginBuf - chnl->endBuf, MSGSIZE)));
		chnl->endBuf += nread;
	}
	else
	{
		_(nread = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, min(chnl->sizeBuf - chnl->endBuf, MSGSIZE)));
		chnl->endBuf = (chnl->endBuf + nread) % chnl->sizeBuf;

		if (chnl->endBuf == 0)
			_(nread = chnl->endBuf += read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, MSGSIZE - nread));
	}

	if (chnl->beginBuf == chnl->endBuf)
		chnl->status = FULL;
	else
		chnl->status = PARTLY;
	
	return nread;
}

int StartServer(long long int N, int inputFd)
{
	//PL;
	server_t* server = NULL;
	_(server = ServerCtor(N, inputFd));

	fd_set readfds = {};
	fd_set writefds = {};
	int nfds = max(server->maxInFd, server->maxOutFd);
	int res = 0;

	size_t nread = 1;
	while(nread)
	{
		//sleep(10);
		readfds = server->outFds;
		writefds = server->inFds;
		_(res = select(nfds, &readfds, &writefds, NULL, NULL));

		for (long long i = N; i >= 1; i--)
		{
			if (FD_ISSET(server->chnls[i]->pipeOut[0], &readfds) && server->chnls[i]->status != FULL)
			{

				_(nread = ServerRead(server, i));
				//_(nread = read(server->chnls[i]->pipeOut[0], server->chnls[i]->buf, MSGSIZE));

				fprintf(stderr, "CAn read   %lli %lu\n", i, nread);

				if ( nread == 0)
				{
					FD_CLR(server->chnls[i]->pipeOut[0], &server->outFds);
					close(server->chnls[i]->pipeOut[0]);

					FD_CLR(server->chnls[i]->pipeIn[1], &server->inFds);
					close(server->chnls[i]->pipeIn[1]);

					//if ( i == 1 ) nread = 1;
				}
			}
			if (FD_ISSET(server->chnls[i]->pipeIn[1], &writefds) && server->chnls[i+1]->status != EMPTY)
			{

				 _(nread = ServerWrite(server, i));
				fprintf(stderr, "\t\t\tcan Write   %lli %lu\n", i, nread);
				if (!FD_ISSET(server->chnls[i+1]->pipeOut[0], &server->outFds))
				{
					FD_CLR(server->chnls[i]->pipeIn[1], &server->inFds);
					close(server->chnls[i]->pipeIn[1]);

					if ( i == 1 ) nread = 0;
				}
			}
			ChannelDump(server->chnls[i]);
			fprintf(stderr, "++++%lli\n", i);
		}
		//fprintf(stderr, "_______________\n");
	}
	pid_t lastChild = server->lastChild;
	//ServerDtorN(server, 0);
	//PL;
	_(waitpid(lastChild, NULL, 0));
	//while(1);
	return 0;
}

int main(int argc, char const *argv[])
{
	/*
	if (argc != 3)
	{
		fprintf(stderr, "Use: %s <N> <filename>\n", argv[0]);
		return -1;
	}

	long long int N = 0;
	_(N = strtoll(argv[1], NULL, 10));

	if ( N <= 1 )
	{
		fprintf(stderr, "Invalid N\n");
		return -1;
	}

	int inputFd = 0;
	_(inputFd = open(argv[2], O_RDONLY));

	_(StartServer(N, inputFd));
	*/

	channel_t* this = ChannelCtor(20, 1);
	close(this->pipeOut[0]);
	close(this->pipeOut[1]);
	close(this->pipeIn[1]);
	close(this->pipeIn[0]);
	this->pipeOut[0] = STDIN_FILENO;
	this->pipeIn[1] = STDOUT_FILENO;
	while (1)
	{
		PL;
		ServerRead(&this, 0);
		ChannelDump(this);
		fprintf(stderr, "++++++++++++++++++++++++++++++++++++++++++++\n");
		ServerWrite(&this, 0);
		ChannelDump(this);

	}


	return 0;
}