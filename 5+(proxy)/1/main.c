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
#define MSGSIZE (512*3)
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define IS_NULL(ptr)             \
			do{                  \
				if (ptr == NULL) \
					return NULL; \
			}while(0)

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

ssize_t StartChild(channel_t* chnl)
{
	_(close(chnl->pipeIn[1]));
	_(close(chnl->pipeOut[0]));

	ssize_t nread = 1;
	while(nread)
	{
		_(nread = read(chnl->pipeIn[0], chnl->locBuf, MSGSIZE));
		// if (chnl->n == 2)
			// sleep(1);
		_(write(chnl->pipeOut[1], chnl->locBuf, nread) );
	}

	close(chnl->pipeIn[0]);
	close(chnl->pipeOut[1]);
	exit(0);
}

int ServerDtorN(server_t* this, long long n)
{

	this->maxOutFd = 0;
	this->maxInFd = 0;
	FD_ZERO(&this->outFds);
	FD_ZERO(&this->inFds);

	for (long long i = this->N; i > n; i--)
		ChannelDtor(this->chnls[i]);

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

	for (size_t i = this->N; i >= 1;i--)
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
	IS_NULL(this);


	this->chnls = (channel_t**) calloc(N+1, sizeof(*(this->chnls)));
	IS_NULL(this->chnls);


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
		//errno = 0;
		chnls[i] = ChannelCtor( ((size_t) pow(3, i)) *512, i);
		IS_NULL(chnls[i]);
		pid_t pid = 0;
		pid = fork();
		if (pid < 0)
			return NULL;
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
			ServerDtorN(this, i);
			//ServerDump(this);
			if (StartChild(chnl) < 0)
				return NULL;
			exit(0);
		}
		if (i == 1)
			this->lastChild = pid;
	}

	for (long long int i = N; i >= 1; i--)
	{
		close(chnls[i]->pipeIn[0]);
		close(chnls[i]->pipeOut[1]);
		fcntl(chnls[i]->pipeIn[1], F_SETFL, O_WRONLY | O_NONBLOCK);
		FD_SET(chnls[i]->pipeIn[1], &this->inFds);
		this->maxInFd = max(this->maxInFd, chnls[i]->pipeIn[1]);

		fcntl(chnls[i]->pipeOut[0], F_SETFL, O_RDONLY | O_NONBLOCK);
		FD_SET(chnls[i]->pipeOut[0], &this->outFds);
		this->maxOutFd = max(this->maxOutFd, chnls[i]->pipeOut[0]);
	}
	FD_CLR(this->chnls[N]->pipeIn[1], &this->inFds);
	close(chnls[N]->pipeIn[1]);

	FD_CLR(chnls[1]->pipeOut[0], &this->outFds);
	close(chnls[1]->pipeOut[0]);

	return this;
}

ssize_t ServerWrite(server_t* this, long long i)
{
	ssize_t nwrite = 0;

	channel_t* chnl = this->chnls[i+1];

	if (chnl->status == EMPTY)
		return 0;

	if (chnl->endBuf >= chnl->beginBuf)
	{
		nwrite = write(this->chnls[i]->pipeIn[1], chnl->buf + chnl->beginBuf, min(MSGSIZE, chnl->endBuf - chnl->beginBuf));
		if (nwrite == -1) return -1;
		chnl->beginBuf = (chnl->beginBuf + nwrite) % chnl->sizeBuf;
	}
	else
	{
		nwrite = write(this->chnls[i]->pipeIn[1], chnl->buf + chnl->beginBuf, min(MSGSIZE, chnl->sizeBuf - chnl->beginBuf));
		if (nwrite == -1) return -1;
		chnl->beginBuf = (chnl->beginBuf + nwrite) % chnl->sizeBuf;

		if (chnl->beginBuf == 0)
			nwrite += chnl->beginBuf = write(this->chnls[i]->pipeIn[1], chnl->buf + chnl->beginBuf, MSGSIZE - nwrite);
	}

	if (chnl->beginBuf == chnl->endBuf)
		chnl->status = EMPTY;
	else
		chnl->status = PARTLY;

	return nwrite;
}

ssize_t ServerRead(server_t* this, long long i)
{
	ssize_t nread = 0;
	channel_t* chnl = this->chnls[i];

	if (chnl->status == FULL)
		return -1;

	if (chnl->beginBuf - chnl->endBuf > 0)
	{
		_(nread = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, min(chnl->beginBuf - chnl->endBuf, MSGSIZE)));
		chnl->endBuf = (chnl->endBuf + nread) % chnl->sizeBuf;
	}
	else
	{
		_(nread = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, min(chnl->sizeBuf - chnl->endBuf, MSGSIZE)));
		chnl->endBuf = (chnl->endBuf + nread) % chnl->sizeBuf;

		if (chnl->endBuf == 0)
			_(nread += chnl->endBuf = read(chnl->pipeOut[0], chnl->buf + chnl->endBuf, MSGSIZE - nread));
	}

	if (chnl->beginBuf == chnl->endBuf && !nread)
		chnl->status = EMPTY;
	else if (chnl->beginBuf == chnl->endBuf)
		chnl->status = FULL;
	else
		chnl->status = PARTLY;
	
	return nread;
}

int StartServer(long long int N, int inputFd)
{
	server_t* server = NULL;

	server = ServerCtor(N, inputFd);
	if (server == NULL)
		return -1;

	fd_set readfds = {};
	fd_set writefds = {};
	fd_set reservWrite = {};
	int nfds = max(server->maxInFd, server->maxOutFd);
	int res = 0;
	ssize_t nread = 0;
	ssize_t nwrite = 0;
	int isend = 0;
	//server->inFds = {};
	while(!isend)
	{
		PL;
		readfds = server->outFds;
		_(res = select(server->maxOutFd, &readfds, /*&writefds*/ NULL, NULL, NULL));

		for (long long i = N; i >= 1; i--)
		{
			
			 // fprintf(stderr, "%lli\n", i);
			if(FD_ISSET(server->chnls[i]->pipeOut[0], &readfds) && server->chnls[i]->status != FULL)
			{
				// PL;
				_(nread = ServerRead(server, i));

				if (nread == 0)
				{
					FD_CLR(server->chnls[i]->pipeOut[0], &server->outFds);
					close(server->chnls[i]->pipeOut[0]);
				}

				FD_SET(server->chnls[i-1]->pipeIn[1], &reservWrite);
			}
		}
		/*
				if (FD_ISSET(server->chnls[i-1]->pipeIn[1], &writefds) && server->chnls[i]->status != EMPTY)
				{
				// PL;
					_(nwrite = ServerWrite(server, i-1));
				
				}*/
			
/*
			if (FD_ISSET(server->chnls[i]->pipeIn[1], &writefds) && server->chnls[i+1]->status != EMPTY)
			{
				// PL;
				nwrite = 1;
				while(nwrite > 0)
					nwrite = ServerWrite(server, i);

				FD_CLR(server->chnls[i]->pipeIn[1], &writefds);
			}
			
			// PL;
			if (FD_ISSET(server->chnls[i]->pipeIn[1], &writefds) &&
				server->chnls[i+1]->status == EMPTY && 
				!FD_ISSET(server->chnls[i+1]->pipeOut[0], &server->outFds))
			{
				// PL;
				FD_CLR(server->chnls[i]->pipeIn[1], &server->inFds);
				close(server->chnls[i]->pipeIn[1]);
				if (i == 1) 
					isend = 1;
				// PL;
			}*/

//!!!!!!!!!!!!!!!!!
		writefds = reservWrite;
		PL;
		_(res = select(server->maxInFd, NULL, &writefds, NULL, NULL));

		for (long long i = N; i >= 1; i--)
		{
			if (FD_ISSET(server->chnls[i]->pipeIn[1], &writefds) && server->chnls[i+1]->status != EMPTY)
			{
				// PL;
				nwrite = 1;
				while(nwrite > 0)
					nwrite = ServerWrite(server, i);

				if (server->chnls[i+1]->status == EMPTY && 
					!FD_ISSET(server->chnls[i+1]->pipeOut[0], &server->outFds))
				{

					FD_CLR(server->chnls[i]->pipeIn[1], &server->inFds);
					close(server->chnls[i]->pipeIn[1]);
					if (i == 1) 
						isend = 1;
					// PL;
				}
				FD_CLR(server->chnls[i]->pipeOut[0], &writefds);
				FD_CLR(server->chnls[i]->pipeOut[0], &reservWrite);
			}
			/*
			if (FD_ISSET(server->chnls[i]->pipeIn[1], &writefds) &&
				server->chnls[i+1]->status == EMPTY && 
				!FD_ISSET(server->chnls[i+1]->pipeOut[0], &server->outFds))
			{
				// PL;
				FD_CLR(server->chnls[i]->pipeIn[1], &server->inFds);
				close(server->chnls[i]->pipeIn[1]);
				if (i == 1) 
					isend = 1;
				// PL;
			}*/
//!!!!!!!!!!!!!!!!

		}
	}

	//_(waitpid(server->lastChild, NULL, 0));

	ServerDtorN(server, 0);
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