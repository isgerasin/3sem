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
#define FIFONMS 100

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

int KillMe()
{
	char buf[100] = {};
	sprintf(buf, "kill -9 %d", getpid());
	system(buf);
	return 0;
}
int GetMsg(int fd)
{
	struct msg_t msg = {};
	pid_t sender = 0;
	int readn = 0;
	CHECK(readn = read(fd, &msg, sizeof(msg)));
	//fprintf(stderr, "!%d!\n", readn);
	while(readn != 0 )
	{
		printf("%s", msg.txtMsg);
		CHECK(readn = read(fd, &msg, sizeof(msg)));
	}
	return 0;
}

int StartClient()
{
	int fd = 0;
	int fdN = 0;
	int nwrite = 0;
	pid_t pid = getpid();
	char newFifoName[FIFONMS] = {};
	snprintf(newFifoName, FIFONMS,"%d.fifo", pid);

	CHECK(InitFifo(newFifoName));
	CHECK(fdN = open(newFifoName, O_RDONLY | O_NONBLOCK));

	CHECK(InitFifo(fifoName));
	CHECK(fd = open(fifoName, O_WRONLY));
	CHECK(nwrite = write(fd, &pid, sizeof(pid)));
	//critical section
	if (nwrite == 0)
		goto end1;
	sleep(2);
	CHECK(fcntl(fdN, F_SETFL, O_RDONLY));
	CHECK(GetMsg(fdN));
	//---------
	close(fdN);
	end1:
	close(fd);
	unlink(fifoName);
	unlink(newFifoName);
	return 0;
	
}

int SendMsg(int fd, const char* name)
{
	int fdInput = 0;
	CHECK(fdInput = open(name, O_RDONLY));
	int i = 0;
	int nread = 0;
	struct msg_t msgs = {};
	CHECK(nread = read(fdInput, &msgs.txtMsg, TXTSIZE));
	msgs.sender = getpid();	
	while(nread != 0)
	{
		msgs.info = i; 	
		CHECK(write(fd, &msgs, sizeof(msgs)));
		memset(&msgs, 0, sizeof(msgs));
		CHECK(nread = read(fdInput, &msgs.txtMsg, TXTSIZE));
		i++;
	}
	close(fdInput);
	return 0;
}

int StartServer(const char* input)
{
	size_t numMsg = 0;
	int fd = 0;
	int fdN = 0;
	int nread = 0;
	pid_t pid = getpid();
	CHECK(InitFifo(fifoName));
	CHECK(fd = open(fifoName, O_RDONLY));
	CHECK(nread = read(fd, &pid, sizeof(pid)));
	//critical section
	if (nread == 0)
		goto end1;
	char newFifoName[FIFONMS] = {};
	snprintf(newFifoName,FIFONMS, "%d.fifo", pid);
	CHECK(InitFifo(newFifoName));
	fdN = open(newFifoName, O_WRONLY | O_NONBLOCK);
	//-----------
	if (fdN < 0 && errno == ENXIO)
		goto end1;
	else if (fdN < 0)
		return -1;
	CHECK(fcntl(fdN, F_SETFL, O_WRONLY));
	CHECK(SendMsg(fdN, input));
	close(fdN);
	end1:
	close(fd);
	unlink(fifoName);
	unlink(newFifoName);
	return 0;
	
}

int fdcmp(int fda, int fdb)
{
	struct stat sta = {};
	struct stat stb = {};
	CHECK(fstat(fda, &sta));
	CHECK(fstat(fdb, &stb));
	printf( "%d %d\n"
			"%d %d\n", sta.st_ino, stb.st_ino, sta.st_dev, stb.st_dev);
	return (sta.st_ino == stb.st_ino) &&
		   (sta.st_dev == stb.st_dev);
}

int main(int argc, char* argv[])
{
	int fd = open("txt", O_RDONLY);
	int fd1 = open("txt", O_RDONLY);

	printf("%d %d\n",fd, fd1 );
	printf("%d\n", fdcmp(fd, fd1));

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
