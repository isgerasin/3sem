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



#include "../../def/check.h"

#define LOCSIZE ((size_t) 128*1024)
//PIPE_BUF
#define MSGSIZE (512*3)//PIPE_BUF//(512*3)
#define max(a, b) ((a) > (b) ? a: b)

typedef enum status_t
{
	EMPTY,
	PARTLY,
	FULL
}status_t;

typedef struct channel_t
{
	size_t bufSize;
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



channel_t* ChannelCtor(size_t bufSize, long long int n)
{
	errno = 0;
	channel_t* this = calloc(1, sizeof(channel_t));
	if (this == NULL)
		return NULL;

	this->bufSize = bufSize;
	this->n = n;
	this->beginBuf = 0;
	this->endBuf = 0;
	this->status = EMPTY;

	if (pipe(&(this->pipeIn[0])) < 0)
		return NULL;

	if (pipe(&(this->pipeOut[0])) < 0)
		return NULL;

	this->buf = (char*) calloc(bufSize, sizeof(*this->buf));

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
	(this->pipeIn)[0] = -1;

	close(this->pipeIn[1]);
	(this->pipeIn)[1] = -1;

	close(this->pipeOut[0]);
	(this->pipeOut)[0] = -1;

	close(this->pipeIn[1]);
	(this->pipeOut)[1] = -1;

	this->bufSize = 0;
	free(this);
	return 0;
}

int ChannelOk(channel_t* this)
{
	return  this &&
			(this->buf != 0) &&
			(this->bufSize != 0) &&
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
			"\tbufSize = %lu\n"
			"\tn = %lli\n"
			"\t\tpipeIn[0] = %i\n"
			"\t\tpipeIn[1] = %i\n"
			"\t\tpipeOut[0] = %i\n"
			"\t\tpipeOut[1] = %i\n"
			"\tbuf = [%p]\n{\n",
			this, (ChannelOk(this) ? "ok" : "ERROR!!!"), this->bufSize, this->n, this->pipeIn[0], this->pipeIn[1],this->pipeOut[0], this->pipeOut[1], this->buf);

	write(STDERR_FILENO, this->buf, this->bufSize);
	fprintf(stderr, "\n}\n\tlocBuf[%lu]\n{\n", LOCSIZE);
	write(STDERR_FILENO, this->locBuf, LOCSIZE);

	fprintf(stderr, "\n}\n======================================================\n");
	return 0;
}

int StartChild(channel_t* chnl)
{
	long long int nread = 1;
	//fprintf(stderr, "%d\n", chnl->pipeIn[0]);
	while(nread)
	{
		_(nread = read(chnl->pipeIn[0], chnl->locBuf, MSGSIZE));
		//ChannelDump(chnl);
		//fprintf(stderr, "%lli\n", nread);
		_(write(chnl->pipeOut[1], chnl->locBuf, nread));
	}

	close(chnl->pipeIn[0]);
	close(chnl->pipeOut[1]);
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
	//	PL;
	//	fprintf(stderr, "%lli\n", i);
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
					"\tN = %d\n", this, this->N);

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

	//N = 1;

	FD_ZERO(&this->inFds);
	FD_ZERO(&this->outFds);
	this->maxInFd = 0;
	this->maxOutFd = 0;
	this->N = N;
	this->inputFd = inputFd;
	this->lastChild = 0;

	channel_t** chnls = this->chnls; 

	for (long long int i = N; i > 0; i--)
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

			_(close(chnl->pipeIn[1]));
			_(close(chnl->pipeOut[0]));
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
			PL;
			fprintf(stderr, "%lli\n", i);

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

	for (long long int i = N; i > 0; i--)
	{
		_(close(chnls[i]->pipeIn[0]));
		_(close(chnls[i]->pipeOut[1]));

		_(fcntl(chnls[i]->pipeIn[1], F_SETFL, O_WRONLY | O_NONBLOCK));
		FD_SET(chnls[i]->pipeIn[1], &this->inFds);
		this->maxInFd = max(this->maxInFd, chnls[i]->pipeIn[1]);
		
		_(fcntl(chnls[i]->pipeOut[0], F_SETFL, O_RDONLY | O_NONBLOCK));
		FD_SET(chnls[i]->pipeOut[0], &this->outFds);
		this->maxOutFd = max(this->maxOutFd, chnls[i]->pipeOut[0]);
	}

	FD_CLR(this->chnls[N]->pipeIn[1], &this->inFds);
	close(chnls[N]->pipeIn[1]);

	FD_CLR(chnls[1]->pipeOut[0], &this->outFds);
	close(chnls[1]->pipeOut[0]);

	return this;
}

size_t ServerWrite(server_t* this, long long i)
{
	size_t nwrite = 0;
	if (this->chnls[i+1]->status != EMPTY)
	{
		_(nwrite = write(this->chnls[i]->pipeIn[1], this->chnls[i+1]->buf + this->chnls[i+1]->beginBuf, MSGSIZE));
		this->chnls[i+1]->beginBuf = (this->chnls[i+1]->beginBuf + nwrite) % this->chnls[i+1]->bufSize;

		if (this->chnls[i+1]->beginBuf == this->chnls[i+1]->endBuf)
			this->chnls[i+1]->status = EMPTY;
		else
			this->chnls[i+1]->status = PARTLY;
	}
	return nwrite;
}

size_t ServerRead(server_t* this, long long i)
{
	size_t nread = 0;
	if (this->chnls[i]->status != FULL)
	{
		_(nread = read(this->chnls[i]->pipeOut[0], this->chnls[i]->buf + this->chnls[i]->endBuf, MSGSIZE));
		this->chnls[i]->endBuf = (this->chnls[i]->beginBuf + nread) % this->chnls[i]->bufSize;
		if (this->chnls[i]->beginBuf == this->chnls[i]->endBuf)
			this->chnls[i]->status = FULL;
		else
			this->chnls[i]->status = PARTLY;
	}
	else 
		nread = 1;

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
	int nextClose = 0;
	while(nread)
	{
		//readfds = server->outFds;
		//writefds = server->inFds;
		_(res = select(nfds, &readfds, &writefds, NULL, NULL));
		//fprintf(stderr, "%d\n", res);

		for (long long i = N; i >= 1; i--)
		{
			if (FD_ISSET(server->chnls[i]->pipeOut[0], &readfds))
			{
				//_(nread = ServerRead(server, i));
				_(nread = read(server->chnls[i]->pipeOut[0], server->chnls[i]->buf, MSGSIZE));
				if (nread == 0)
				{
					FD_CLR(server->chnls[i]->pipeIn[1], &server->inFds);
					close(server->chnls[i]->pipeIn[1]);
					FD_CLR(server->chnls[i]->pipeOut[0], &server->outFds);
					close(server->chnls[i]->pipeOut[0]);
					continue;
				}

			}
			if (FD_ISSET(server->chnls[i]->pipeIn[1], &writefds))
			{
				//fprintf(stderr, "Write %d \n", i);
				_(write(server->chnls[i]->pipeIn[1], server->chnls[i+1]->buf, MSGSIZE));
				//(ServerWrite(server, i));
			}
			
			//ChannelDump(server->chnls[i]);
		}

	}
	pid_t lastChild = server->lastChild;
	//ServerDtorN(server, 0);
	PL;
	_(waitpid(lastChild, NULL, 0));
	//while(1);
	return 0;
}

int main(int argc, char const *argv[])
{
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

	return 0;
}