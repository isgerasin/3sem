#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CHECK( var ) 		\
	do{	errno = 0;			\
		if ((var) <	0 )		\
			return -1;		\
	}while(0)

#define PP printf("%d\n", __LINE__);

#define TXTSIZE (PIPE_BUF/2)

const char fifoName[] = "file.fifo";
const char fifoNameTow[] = "fileTow.fifo";

enum MsgInfo
{
	FIRST = 1,
	LAST = -1,
	MIDDLE = 0
};

struct msg_t
{
	pid_t sender;
	int info;
	char txtMsg[TXTSIZE+1];
};

int InitFifo(const char* name)
{
	errno = 0;
	if (mkfifo(name, 0666) < 0)
		if (errno != EEXIST)
			return -1;
	errno = 0;
	return 0;
}

int MsgDump(const struct msg_t msg)
{
	printf( "msg_t \n"
			"{"
			"\tsender = %d\n"
			"\tinfo = %d"
			"\ttxtMsg = \n----\n%s\n----\n}\n", msg.sender, msg.info, msg.txtMsg);
	return 0;
}

int GetMsgsFromFile(const char* name, struct msg_t** msgs, size_t* num)
{
	int fd = 0;
	CHECK(fd = open(name, O_RDONLY));
	struct stat fdSt = {};
	CHECK(fstat(fd, &fdSt));
	*num = fdSt.st_size / TXTSIZE + 2;
	*msgs = (struct msg_t*) calloc(*num, sizeof(struct msg_t));
	if (*msgs == NULL)
		return -1;
	for (size_t i = 0; i < *num; i++)
	{
		CHECK(read(fd, (*msgs)[i].txtMsg, TXTSIZE));
		(*msgs)[i].sender = getpid();	
		(*msgs)[i].info = i+1; 
	}
	//(*msgs)[0].info = FIRST;
	(*msgs)[*num-1].info = LAST;
	CHECK(close(fd));
	return 0;	
}

int GetMsg(int fd)
{
	struct msg_t msg = {};
	pid_t sender = 0;
	int readn = 0;
	do{
		CHECK(readn = read(fd, &msg, sizeof(struct msg_t)));
		printf("%s", msg.txtMsg );
	}while(  msg.info != LAST );
	return 0;
}

int StartClient()
{
	CHECK(InitFifo(fifoName));
	CHECK(InitFifo(fifoNameTow));
	int fd = 0;
	int nread = 0;
	int nwrite = 0;
	pid_t pid = 0;
	CHECK(fd = open(fifoNameTow, O_RDONLY));
	CHECK(nread = read(fd, &pid, sizeof(pid)));
	close(fd);
	if (nread == 0)
		goto end1;

	CHECK(fd = open(fifoName, O_WRONLY));
	pid = getpid();
	CHECK(nwrite = write(fd, &pid, sizeof(pid)));
	close(fd);
	char newFifoName[100] = {};
	sprintf(newFifoName, "%d.fifo", pid);
	CHECK(InitFifo(newFifoName));
	CHECK(fd = open(newFifoName, O_RDONLY));
	CHECK(GetMsg(fd));

	close(fd);
	end1:
	unlink(fifoNameTow);
	unlink(fifoName);
	unlink(newFifoName);
	return 0;
}

int SendMsg(int fd, struct msg_t* msgs)
{
	int i = -1;
	do{

		i++;
		CHECK(write(fd, msgs + i, sizeof(struct msg_t)));
	}while(msgs[i].info != LAST);
	return 0;
}

int StartServer(const char* input)
{
	CHECK(InitFifo(fifoName));
	CHECK(InitFifo(fifoNameTow));
	size_t numMsg = 0;
	struct msg_t* msgs = NULL;
	CHECK(GetMsgsFromFile(input, &msgs, &numMsg));
	//for (size_t i = 0; i < numMsg; i++)
	//	MsgDump(msgs[i]);
	int fd = 0;
	int nread = 0;
	pid_t pid = 0;
	
	CHECK(fd = open(fifoNameTow, O_WRONLY));
	CHECK(write(fd, &pid, sizeof(pid)));
	close(fd);

	CHECK(fd = open(fifoName, O_RDONLY));
	CHECK(nread = read(fd, &pid, sizeof(pid)));
	close(fd);
	if (nread == 0)
		goto end1;
		
	char newFifoName[100] = {};
	sprintf(newFifoName, "%d.fifo", pid);
	CHECK(InitFifo(newFifoName));
	CHECK(fd = open(newFifoName, O_WRONLY));
	CHECK(SendMsg(fd, msgs));
	
	close(fd);
	end1:
	free(msgs);
	unlink(fifoName);
	unlink(fifoNameTow);
	unlink(newFifoName);
	return 0;
}

int main(int argc, char* argv[])
{
	//printf("%ld\n", pathconf(fifoName, _PC_PIPE_BUF));
	//printf("%lu\n", PIPE_BUF);
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
