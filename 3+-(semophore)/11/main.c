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

#define Z(sb, s, flg) do{\
			sb.sem_num = (s);\
			sb.sem_op = 0;\
			sb.sem_flg = (flg);\
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

int StartClient()
{
	struct sembuf locsb[SEMN] = {};
	int semid = 0;
	char* shmbuf = NULL;
	int shmid = 0;
	_(shmid = InitConnection(&semid, &shmbuf));

	Z(locsb[0], clientN, IPC_NOWAIT);
	A(locsb[1], clientN, 1, SEM_UNDO);
	errno = 0;
	if (semop(semid, locsb, 2) < 0)
	{
		if ( errno == EIDRM | errno == EINVAL | errno == EAGAIN )
			goto end;
		else 
			return -1;
	}

	//*(size_t*)shmbuf = 0;
	//_((size_t) memset(shmbuf, 0, SHMBUFSIZE));

	//_(semctl(semid, full, SETVAL, 1));
/*
	A(locsb[0], full, 1, 0);
	 _(semop(semid, locsb, 1));

	D(locsb[0], mutex, 1, 0);
	A(locsb[1], mutex, 1, 0);
	_(semop(semid, locsb, 2));

	//D(locsb[0], init, 1, 0);
	//_(semop(semid, locsb, 1));
*/
	A(locsb[0], mutex, 1, SEM_UNDO);
	 _(semop(semid, locsb, 1));

	size_t nread = 0;
	do
	{
		D(locsb[0], full, 1, 0);
		D(locsb[1], mutex, 1, SEM_UNDO);
		D(locsb[2], serverN, 1, IPC_NOWAIT);
		A(locsb[3], serverN, 1, 0);
		errno = 0;
		if (semop(semid, locsb, 4) < 0)
		{
			if (errno == EAGAIN)
				goto end1;
			else
				return -1;
		}

		nread = *(size_t*)shmbuf;
		_(write(1, shmbuf + sizeof(size_t), nread));

		A(locsb[0], mutex, 1, SEM_UNDO);
		_(semop(semid, locsb, 1));
	}while(nread);

	end1:
	semctl(semid, IPC_RMID, 0);
	shmctl(shmid, IPC_RMID, NULL);
	end:
	shmdt(shmbuf);
	return 0;
}

int StartServer(const char* name)
{
	struct sembuf locsb[SEMN] = {};
	int semid = 0;
	char* shmbuf = NULL;
	int shmid = 0;
	_(shmid = InitConnection(&semid, &shmbuf));

	int fd = 0;
	_(fd = open(name, O_RDONLY));

	Z(locsb[0], serverN, IPC_NOWAIT);
	A(locsb[1], serverN, 1, SEM_UNDO);
	errno = 0;
	if (semop(semid, locsb, 2) < 0)
	{
		if ( errno == EIDRM | errno == EINVAL | errno == EAGAIN)
			goto end;
		else 
			return -1;
	}
/*
	A(locsb[0], mutex, 1, 0);
	D(locsb[1], full, 1, 0);
	_(semop(semid, locsb, 2));
*/

	size_t readn = 0;
	do{
		Z(locsb[0], full, 0);
		D(locsb[1], mutex, 1, SEM_UNDO);
		D(locsb[2], clientN, 1, IPC_NOWAIT);
		A(locsb[3], clientN, 1, 0);
		errno = 0;
		if (semop(semid, locsb, 4) < 0)
		{
			if (errno == EAGAIN)
			{
				semctl(semid, IPC_RMID, 0);
				shmctl(shmid, IPC_RMID, NULL);
				goto end;
			}
			else
				return -1;
		}
		

		_(readn  = read(fd, shmbuf + sizeof(size_t), SHMBUFSIZE - sizeof(size_t)));
		*(size_t*)shmbuf = readn;

		A(locsb[0], mutex, 1, SEM_UNDO);
		A(locsb[1], full, 1, 0);
		_(semop(semid, locsb, 2));
	}while(readn);

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
