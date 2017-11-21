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

#define SEMN 5
#define SHMBUFSIZE 4096

const char filename[] = "main.c";


int main(int argc, char const *argv[])
{
	key_t key =  ftok(filename, 0);
	int shmid =  shmget(key, SHMBUFSIZE, IPC_CREAT | 0666);
	char* shmbuf = NULL;
	_((size_t) (shmbuf = (char*) shmat(shmid, NULL, 0)));
	int semid = 0;
	struct sembuf locsb[SEMN] = {};
	_(semid = semget(key, SEMN, 0666));
	errno = 0;
	semctl(semid, IPC_RMID, 0);
	perror("rmsem");
	return 0;
}