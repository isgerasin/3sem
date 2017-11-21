#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	char* endptr = NULL;
	long long n = strtoll(argv[1], &endptr, 10);
	long long i = 0;
	for (i = 0; i < n; i++)
	{
		pid_t pid = fork();
		if (pid < 0)
		{
			fprintf(stderr, "Error at fork\n");
			exit(1);
		}
		if (pid == 0)
			fprintf(stderr, "%lld\n", i);
		else 
			break;
	}
	return 0;
}
