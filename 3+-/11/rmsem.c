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

#include "../../def/check.h"

#define A(sb, s, n, flg) do{\
			sb.sem_num = (s);\
			sb.sem_op = (n);\
			sb.sem_flg = (flg);\
		}while(0)

#define D(sb, s, n, flg) do{\
			sb.sem_num = (s);\
			sb.sem_op = -(n);\
			sb.sem_flg = (flg);\
		}while(0)

#define Z(sb, s) do{\
			sb.sem_num = (s);\
			sb.sem_op = 0;\
			sb.sem_flg = SEM_UNDO;\
		}while(0)

#define SEMN 4
#define SHMBUFSIZE 4096

const char filename[] = "main.c";

union semun
{
	int              val;    /* Value for SETVAL */
	struct semid_ds* buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short*  array;  /* Array for GETALL, SETALL */
	struct seminfo*  __buf;  /* Buffer for IPC_INFO (Linux-specific) */
};

enum semp
{
	mutex = 0,
	full = 1,
	serverN = 2,
	clientN = 3
};

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
	key_t key =  ftok(filename, 0);
	int shmid =  shmget(key, SHMBUFSIZE, IPC_CREAT | 0666);
	char* shmbuf = NULL;
	_((size_t) (shmbuf = (char*) shmat(shmid, NULL, 0)));
	int semid = 0;
	struct sembuf locsb[SEMN] = {};
	_(semid = semget(key, SEMN, 0666));
	DumpSem(semid);
	errno = 0;
	semctl(semid, IPC_RMID, 0);
	perror("rmsem");
	return 0;
}