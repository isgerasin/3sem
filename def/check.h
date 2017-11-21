#include <errno.h>

#define PL fprintf(stderr, " %s %d \n", __PRETTY_FUNCTION__,  __LINE__)

#define _( var ) do{\
		errno = 0;\
		if ( (var) == -1 ){\
			PL; perror(#var ); return -1;} \
		}while(0)


