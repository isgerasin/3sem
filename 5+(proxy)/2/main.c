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
	char locBuf[LOCSIZE];

	char* buf;
	size_t beginBuf;
	size_t endBuf;
	status_t status;
} channel_t;

int ChannelOk(channel_t* this);

int ChannelDtor(channel_t* this);

channel_t* ChannelCtor(size_t bufSize, long long int n)
{
	errno = 0;
	channel_t* this = (channel_t*) calloc(1, sizeof(channel_t));
	if (this == NULL)
		return NULL;

	this->bufSize = bufSize;
	this->n = n;
	this->beginBuf = 0;
	this->endBuf = 0;
	this->status = EMPTY;

	if (pipe(this->pipeIn) < 0)
		return NULL;

	if (pipe(this->pipeOut) < 0)
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

	close(this->pipeOut[1]);
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
	close(chnl)
	ssize_t nread = 1;
	while(nread = read(chnl->pipeIn[0], chnl->locBuf, MSGSIZE))
	{
		_(nraed);
		_(write(chnl->pipeOut[1], chnl->locBuf, nread));
	}

	
	exit(0);
}


int StartServer(long long int N, int inputFd)
{

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