#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

void* printN(void* arg)
{
	printf("%ld\n", *(long*)arg);
	while(1);
	return NULL;
}

int main(int argc, char* argv[])
{
	creat("rtyuio", 0764);

	pthread_t tid;
	char* endptr = NULL;
	long n = strtol(argv[1], &endptr, 10);
	for (long i = 0; i < n; i++)
		if(	pthread_create(&tid, NULL, printN, &i) != 0)
			fprintf(stderr, "Error\n");
	while(1);
	return 0;
}
