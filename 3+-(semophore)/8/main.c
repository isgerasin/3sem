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
			(sb).sem_num = (s);\
			(sb).sem_op = (n);\
			(sb).sem_flg = (flg);\
		}while(0)

#define D(sb, s, n, flg) do{\
			(sb).sem_num = (s);\
			(sb).sem_op = -(n);\
			(sb).sem_flg = (flg);\
		}while(0)

#define Z(sb, s) do{\
			sb.sem_num = (s);\
			sb.sem_op = 0;\
			sb.sem_flg = SEM_UNDO;\
		}while(0)

#define SEMN 5
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
	empty = 0,
	mutex = 1,
	full = 2,
	serverN = 3,
	clientN = 4
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

int StartClient()
{
	key_t key = 0;
	_(key =  ftok(filename, 0));
	int shmid = 0;
	_(shmid = shmget(key, SHMBUFSIZE, IPC_CREAT | 0666));
	char* shmbuf = NULL;
	_((size_t) (shmbuf = (char*) shmat(shmid, NULL, 0)));


	int semid = 0;
	struct sembuf locsb[SEMN] = {};
	_(semid = semget(key, SEMN, IPC_CREAT | 0666));

	A(locsb[0], clientN, 1, SEM_UNDO);
	_(semop(semid, locsb, 1));

	if (semctl(semid, clientN, GETVAL) != 1)
		goto end;
	
	char locbuf[SHMBUFSIZE] = {};
	size_t nread = 0;
	do
	{
		D(locsb[0], full, 1, 0);
		D(locsb[1], mutex, 1, SEM_UNDO);
		_(semop(semid, locsb, 2));

		_((size_t)memcpy(locbuf, shmbuf, SHMBUFSIZE));

		A(locsb[0], mutex, 1, SEM_UNDO);
		A(locsb[1], empty, 1, 0);
		_(semop(semid, locsb, 2));

		_(write(1, locbuf + sizeof(size_t), *(size_t*)locbuf));
	}while(*(size_t*)locbuf);

	semctl(semid, IPC_RMID, 0);
	shmctl(shmid, IPC_RMID, NULL);

	end:
	shmdt(shmbuf);
	return 0;
}

int StartServer(const char* name)
{
	key_t key = 0;
	_(key =  ftok(filename, 0));
	int shmid = 0;
	_(shmid = shmget(key, SHMBUFSIZE, IPC_CREAT | 0666));
	char* shmbuf = NULL;
	_((size_t)(shmbuf = (char*) shmat(shmid, NULL, 0)));
	int fd = 0;
	_(fd = open(name, O_RDONLY));
	int semid = 0;
	struct sembuf locsb[SEMN] = {};
	_(semid = semget(key, SEMN, IPC_CREAT | 0666));

	A(locsb[0], serverN, 1, SEM_UNDO);
	_(semop(semid, locsb, 1));

	if ( semctl(semid, serverN, GETVAL) != 1 )
		goto end;

	A(locsb[0], empty, 1, 0);
	A(locsb[1], mutex, 1, 0);
	_(semop(semid, locsb, 2));

	char locbuf[SHMBUFSIZE] = {};
	size_t readn = 0;
	do{
		_(*(size_t*)locbuf = read(fd, locbuf + sizeof(size_t), SHMBUFSIZE - sizeof(size_t)));

		D(locsb[0], empty, 1, 0);
		D(locsb[1], mutex, 1, SEM_UNDO);
		_(semop(semid, locsb, 2));

		_((size_t)memcpy(shmbuf, locbuf, SHMBUFSIZE));

		A(locsb[0], mutex, 1, SEM_UNDO);
		A(locsb[1], full, 1, 0);
		_(semop(semid, locsb, 2));
	}while(*(size_t*)locbuf);

	end:
	close(fd);
	shmdt(shmbuf);
	return 0;
}


int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		if (StartClient() < 0 )
		{
			perror("Err in CLient");
			return -1;
		}
	}
	else if (argc == 2)
	{
		if (StartServer(argv[1]) < 0)
		{
			perror("Error in Server");
			return -1;
		}
	}
	else
		printf("Error args\n");
	return 0;
}
