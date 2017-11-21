#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define CHECK_ERR(str) \
		{perror(str); exit(-1);}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Invalid args\n");
		
	}
	errno = 0;
	int pfd[2] = {};
	if (pipe(pfd) < 0)
		CHECK_ERR("pipe")
	errno = 0;
	pid_t pid = fork();
	if (pid < 0)
		CHECK_ERR("fork")
	else if (pid == 0)
	{
		int fd = 0;
		errno = 0;
		if ((fd = open(argv[1], O_RDONLY)) < 0)
			CHECK_ERR("open")
		struct stat st = {};
		errno = 0;
		if (fstat(fd, &st ) < 0)
			CHECK_ERR("fstat")
		char* buf = (char*) calloc(st.st_size, 1);
		if (buf == NULL)
			CHECK_ERR("calloc")
		if (read(fd, buf, st.st_size) < 0)
			CHECK_ERR("read")
		//printf("%s", buf);
		if (write(pfd[1], buf, st.st_size) < 0)
			CHECK_ERR("pipe write")
		free(buf);
	}
	else 
	{
		char* buf = (char*) calloc(100, 1);
		read(pfd[0], buf, 100);
		printf("%s", buf);
	}
}
