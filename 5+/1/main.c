#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "../../def/check.h"

#define LOCSIZE ((size_t) 128*1024)

typedef struct channel_t
{
	int pipefd[2];
	char  locBuf[LOCSIZE];
	char* buf;
	size_t bufSize;
	long long int n;
} channel_t;

int ChannelOk(channel_t* this);

int ChannelDtor(channel_t* this);

channel_t* ChannelCtor(size_t bufSize, long long int n)
{

	channel_t* this = calloc(1, sizeof(*this));
	if (this == NULL)
		return (channel_t*)-1;

	this->bufSize = bufSize;
	this->n = n;

	if (pipe(this->pipefd) < 0)
		return (channel_t*)-1;

	this->buf = (char*) calloc(bufSize, sizeof(*this->buf));

	if (this->buf == NULL)
	{
		ChannelDtor(this);
		return (channel_t*)-1;
	}

	if (!ChannelOk(this))
		this = (channel_t*)-1;
	
	return this;
}

int ChannelDtor(channel_t* this)
{
	if (this == NULL)
		return 0;
	free(this->buf);
	this->buf = NULL;

	close(this->pipefd[0]);
	(this->pipefd)[0] = -1;

	close(this->pipefd[1]);
	(this->pipefd)[1] = -1;

	this->bufSize = 0;
	free(this);
	return 0;
}

int ChannelOk(channel_t* this)
{
	return  this &&
			(this->buf != 0) &&
			(this->bufSize != 0) &&
			(this->pipefd[0] != -1) &&
			(this->pipefd[1] != -1);
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
			"\t\tpipefd[0] = %i\n"
			"\t\tpipefd[1] = %i\n"
			"\tbuf = [%p]\n{\n",
			this, (ChannelOk(this) ? "ok" : "ERROR!!!"), this->bufSize, this->n, this->pipefd[0], this->pipefd[1], this->buf);

	write(STDERR_FILENO, this->buf, this->bufSize);
	fprintf(stderr, "\n}\n\tlocBuf[%lu]{\n", LOCSIZE);
	write(STDERR_FILENO, this->locBuf, LOCSIZE);

	fprintf(stderr, "\n}\n======================================================\n");
	return 0;
}

int StartChild(channel_t* chnl)
{
	size_t nread = 0;
	_(nread = read(chnl->pipefd[0], chnl->locBuf, LOCSIZE));
	while(nread)
	{
		ChannelDump(chnl);
		_(write(chnl->pipefd[1], chnl->locBuf, LOCSIZE));
		_(nread = read(chnl->pipefd[0], chnl->locBuf, LOCSIZE));
	}	
	exit(0);
}

int InitServer(long long int N, int inputFd, channel_t** chnls)
{
	for (long long int i = N; i > 0; i--)
	{
		_((size_t) (chnls[i] = ChannelCtor( ((size_t) pow(3, i)) *512, i)));

		pid_t pid = 0;
		_(pid = fork());

		if (pid == 0)
		{
			if (i == N)
			{
				close(chnls[N]->pipefd[0]);
				chnls[N]->pipefd[0] = inputFd;
			}
			else if(i == 1)
			{
				close(chnls[1]->pipefd[1]);
				chnls[1]->pipefd[1] = STDOUT_FILENO;
			}
			_(StartChild(chnls[i]));
			exit(0);
		}
	}
	return 0;
}

int StartServer(long long int N, int inputFd)
{
	errno = 0;
	channel_t** chnls = (channel_t**) calloc(N+1, sizeof(*chnls));

	if (chnls == NULL)
		return -1;

	_(InitServer(N, inputFd, chnls));

	size_t nread = 0;
	for (long long i = N; i > 1; i--)
	{
		_(nread = read(chnls[i]->pipefd[0], chnls[i]->buf, chnls[i]->bufSize));
		_(write(chnls[i-1]->pipefd[1], chnls[i]->buf, nread));
		
	}

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