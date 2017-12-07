#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_F(val) \
				{ errno = 0; \
				if ((val) <  0) \
					{perror(#val); exit(-1);} }

#define CHECK_PTR(ptr, error) \
				if (ptr == NULL) \
					{fprintf(stderr,"%s:%s=NULL\n", error, #ptr);	 exit(-1);}

const char fifoName[] = "aaa.fifo";

char* MsgFromFile(const char* fileName)
{
	int fd = 0;
	struct stat st = {};
	CHECK_F(fd = open(fileName, O_RDONLY))
	CHECK_F(fstat(fd, &st))
	size_t fsize = st.st_size;
	char* msg = (char*) calloc(fsize, sizeof(*msg));
	CHECK_PTR(msg, "calloc")
	CHECK_F(read(fd, msg, fsize))
	return msg;
}

int SendFifoMsg(const char* msg, const char* fifoname)
{
	int fd = 0;
	int check = 0;
	CHECK_F(fd = open(fifoname, O_WRONLY))
	CHECK_F(check = write(fd, msg, strlen(msg)))
	close(fd);
	return 0;	
}

char* GetFifoMsg(const char* fifoName)
{
	int fd = 0;
	size_t size = 38641;
	char* msg = calloc(size, sizeof(*msg));
	CHECK_PTR(msg, "calloc")
	CHECK_F(fd = open(fifoName, O_RDONLY))
	CHECK_F(read(fd, msg, size))
	close(fd);
	return msg;
}

int InitFifo(const char* name)
{
	errno = 0;
	if (mkfifo(name, 0666) < 0)
	{
		if (errno != EEXIST)
		{
			perror("Can't create FIFO");
			return -1;
		}
	}
	return 0;
}

int DeletFifo(const char* name)
{
	CHECK_F(unlink(name))
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		if (InitFifo(fifoName) < 0)
			return -1;
		char* msg1 = GetFifoMsg(fifoName);
		CHECK_PTR( msg1, "GetFifoMsg" )
		printf("%s", msg1);
		free(msg1);
		CHECK_F(DeletFifo(fifoName))	
	}
	else if (argc == 2)
	{	
		char* msg2 = MsgFromFile(argv[1]);
		CHECK_PTR( msg2, "MsgFromFifo" )
		CHECK_F(SendFifoMsg(msg2, fifoName))
		printf( "Msg is sended\n" );
	}
	else 
		fprintf(stderr, "Invalid args\n");
	return 0;
}
