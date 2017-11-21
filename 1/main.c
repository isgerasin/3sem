#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
/*
int IsStrNum(char* str)
{
	while(*str != 0 )
	{
		if ( !isdigit(*str) )
			return 0;
		str++;
	}
	return 1;
}
*/
int main(int argc, char* argv[])
{

	if (argc != 2)
	{
		fprintf(stderr, "Input isn't correct!\n");
		return 1;
	}
	long long n = 0;
	char* endptr = NULL; 
	errno = 0;
	n = strtoll( argv[1], &endptr , 10 );
	if ((errno == ERANGE && (n == LONG_MAX || n == LONG_MIN)) || (errno != 0 && n ==0))
	{
		fprintf(stderr, "Number is so big!!\n");
		return 1;
	}
	if (endptr == argv[1] || *endptr != 0)
	{
		fprintf(stderr, "Input isn't correct!\n");
		return 1;
	}

	fprintf( stderr, "%lli \n", n);
	for (long long i = 0; i <=n; i++)
		printf("%lli ", i);
	return 0;
}
