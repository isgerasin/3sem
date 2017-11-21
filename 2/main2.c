#include <stdio.h>
#include <unistd.h>

extern char** environ;

int main(int argc, char* argv[])
{
	char* arg[] = {"ls", "-l", "/", NULL};
	pid_t pid = fork();
	if ( pid == 0)
	{
		execve(argv[1], arg , environ);
		printf("Hello\n");
	}
	return 0;
}
