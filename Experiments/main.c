#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdlib.h>


//#include "../def/check.h"

#define SEMN 4
#define SHMBUFSIZE 4096

#define A(sb, s, n, flg) do{\
			(sb).sem_num = (s);\
			(sb).sem_op = (n);\
			(sb).sem_flg = (flg);\
		}while(0)

#define D(sb, s, n, flg) do{\
			(sb).sem_num = (s);\
			(sb).sem_op = -(n);\
			(sb).sem_flg = (flg);\
		}while(0)

#define Z(sb, s, flg) do{\
			sb.sem_num = (s);\
			sb.sem_op = 0;\
			sb.sem_flg = (flg);\
		}while(0)

#define _( var ) \
			fprintf(stderr, "%d| (%d) %s\n", __LINE__, (var), #var );

const char filename[] = "main.c";

union semun
{
	int              val;    /* Value for SETVAL */
	struct semid_ds* buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short*  array;  /* Array for GETALL, SETALL */
	struct seminfo*  __buf;  /* Buffer for IPC_INFO (Linux-specific) */
};

int InitConnection(int* semid, char** shmbuf)
{
	key_t key = 0;
	_(key =  ftok(filename, 0));
	int shmid = 0;
	_(shmid = shmget(key, SHMBUFSIZE, IPC_CREAT | 0666));
	_((size_t)(*shmbuf = (char*) shmat(shmid, NULL, 0)));
	struct sembuf locsb[SEMN] = {};
	_(*semid = semget(key, SEMN, IPC_CREAT | 0666));
	return 0;

}

int DumpSem(int semid)
{
	union semun args = {};
	unsigned short array[SEMN] = {};
	args.array = array;
	_(semctl(semid, 0, GETALL, args));
	fprintf(stderr, "------------------------\n");
	for (int i = 0; i < SEMN; i++)
		fprintf(stderr, "\t%d\n", array[i]);
	fprintf(stderr, "------------------------\n");

	return 0;
}

int main(int argc, char const *argv[])
{
	
	
	/*
	char buf[] = "vvvvvvvvvv";
	//_(creat("hello", 01));
	//_(open("test", O_RDONLY | O_CREAT))
	int fd = 0;
	mkfifo("fifo", 666);
	//fprintf(stderr, "%d| (%o) %s\n", __LINE__, umask(0), "#var" );
	_(fd = open("fifo", O_RDWR));
	_(write(fd, "aaa", 3));
	_(close(fd));
	_(fd = open("fifo", O_RDONLY | O_NONBLOCK));
	_(write(1, buf, read(fd, buf, 3)));
	*/
	/*
	_(ftok(0, 0));

	int fd = 0;
	mkfifo("fifo", 666);
	
	fd = open("fifo", O_RDWR);
	write(fd, "aaa", 3);
	
	fd = open("fifo", O_RDONLY | O_NONBLOCK);
	_(write(1, buf, 0));
	*/

	struct sembuf locsb[SEMN + 1] = {};

	int semid = 0;
	char* shmbuf = NULL;
	int shmid = 0;
	_(shmid = InitConnection(&semid, &shmbuf));

	DumpSem(semid);


	
	D(locsb[0], 0, 2, 0);
	_(semop(semid, locsb, 1));
	A(locsb[0], 0, 2, SEM_UNDO);
	_(semop(semid, locsb, 1));

	return 0;
}