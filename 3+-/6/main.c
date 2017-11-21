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

#define SEMN 7
#define SEMINIT {3, 3, 2, 0, 0, 1, 1}
#define SHMBUFSIZE 2048
#define MAXRETR 3

const char filename[] = "main.c";

union semun
{
               int              val;    /* Value for SETVAL */
               struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
               unsigned short  *array;  /* Array for GETALL, SETALL */
               struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
};

enum semp
{
	empty = 0,
	mutex = 1,
	full = 2,
	initS = 3,
	initC= 4,
	otherS = 5,
	otherC = 6

};

int DumpSem(int semid)
{
	union semun args = {};
	unsigned short array[SEMN] = {};
	args.array = array;
	_(semctl(semid, 0, GETALL, args));
	for (int i = 0; i < SEMN; i++)
		fprintf(stderr, "\t%d\n", array[i]);
	return 0;
}
/*
int InitSem(key_t key)
{
	
	union semun arg = {};
	unsigned short array[] = SEMINIT;
	struct semid_ds sids = {};
	int semid = 0;
	errno = 0;
	if ((semid = semget(key, SEMN, IPC_EXCL | IPC_CREAT | 0666)) >= 0)
	{
		arg.array = array;
		sleep(2);
		_(semctl(semid, 0, SETALL, arg));
	}
	else if (errno == EEXIST )
	{
		_(semid = semget(key, SEMN, 0666));
		for (int i = 0; i < MAXRETR; i++)
		{
			arg.buf = &sids;
			_(semctl(semid, 0, IPC_STAT, arg));
			if ( arg.buf->sem_ctime != 0)
			{
				return semid;
			}
			//sleep(1);
		}
		fprintf(stderr, "Try again.\n" );
		_(semctl(semid, IPC_RMID, 0));
		return -1;
	}
	else if (semid < 0)
	{

		PL;
		return -1;
	}
	return semid;
}
*/
int StartClient()
{

	key_t key = 0;
	//ftok(0, 0);
	_(key =  ftok(filename, 0));
	int shmid = 0;
	_(shmid = shmget(key, SHMBUFSIZE, IPC_CREAT | 0666));
	char* shmbuf = NULL;
	_((size_t) (shmbuf = (char*) shmat(shmid, NULL, 0)));
	int semid = 0;
	struct sembuf locsb[SEMN] = {};
	//_(semid = InitSem(key));
	_(semid = semget(key, SEMN, IPC_CREAT | 0666));

	A(locsb[0], initS, 1, SEM_UNDO);
	_(semop(semid, locsb, 1));

	if ( semctl(semid, otherC, GETVAL) != 0 )
		goto end;

	D(locsb[0], initC, 1, SEM_UNDO);
	D(locsb[1], full, 1, SEM_UNDO);
	A(locsb[2], otherC, 1, SEM_UNDO);
	errno = 0;
	if (semop(semid, locsb, 3) < 0)
	{
		if (errno == EIDRM || errno == EINVAL)
			goto end;
		else 
			return -1;
	}


	/*
	PL;
	A(locsb[0], otherS, 1);
	D(locsb[1], otherC, 1);
	_(semop(semid, locsb, 2));
	PL;
*/

	//DumpSem(semid);
	char locbuf[SHMBUFSIZE] = {};
	int nwrite = 0;
	
	do{

		D(locsb[0], full, 1, SEM_UNDO);
		//PL;
		D(locsb[1], mutex, 1, SEM_UNDO);
		_(semop(semid, locsb, 2));

		_((size_t)memcpy(locbuf, shmbuf, SHMBUFSIZE));
		//PL;
		A(locsb[0], mutex, 1, SEM_UNDO);
		A(locsb[1], empty, 1, SEM_UNDO);
		_(semop(semid, locsb, 2));

		if (*(size_t*)locbuf)
			_(write(1, locbuf+sizeof(size_t),*(size_t*)locbuf));
	}while(*(size_t*)locbuf == SHMBUFSIZE - sizeof(size_t));

	end:
	//semctl(semid, IPC_RMID, 0);
	shmctl(shmid, IPC_RMID, NULL);
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
	//_(semid = InitSem(key));
	//DumpSem(semid);

	_(semid = semget(key, SEMN, IPC_CREAT | 0666));
	//semctl(semid, IPC_RMID, 0);
	//DumpSem(semid);
	/*
	PL;
	A(locsb[0], otherC, 1);
	_(semop(semid, locsb, 1));
	PL;
	D(locsb[0], otherS, 1);
	_(semop(semid, locsb, 1));
	PL;

	A(locsb[0], mutex, 1);
	A(locsb[1], empty, 1);
	locsb[0].sem_flg = 0;
	locsb[1].sem_flg = 0;
	_(semop(semid, locsb, 2));
	*/
	//PL;
	
	if ( semctl(semid, otherS, GETVAL) != 0 )
		goto end;


	A(locsb[0], mutex, 1, 0);
	A(locsb[1], empty, 1, 0);
	A(locsb[2], initC, 1, SEM_UNDO);
	D(locsb[3], initS, 1, SEM_UNDO);
	A(locsb[4], full, 1, 0);
	A(locsb[5], otherS, 1, SEM_UNDO);
	errno = 0;
	if(semop(semid, locsb, 6) < 0)
	{
		if (errno == EIDRM || errno == EINVAL)
			goto end;
		else
			return -1;
	}
	//PL;

	size_t nread = 0;
	char locbuf[SHMBUFSIZE] = {};
	*(size_t*)locbuf = 2;
	do{
		_(nread = read(fd, locbuf+sizeof(size_t), SHMBUFSIZE-sizeof(size_t)));

		D(locsb[0], empty, 1, SEM_UNDO);
		D(locsb[1], mutex, 1, SEM_UNDO);
		if (semop(semid, locsb, 2) < 0)
		{
			if (errno == EIDRM || errno == EINVAL)
				goto end;
			else 
				return -1;
		}

		*(size_t*)locbuf = nread;
		_((size_t)memcpy(shmbuf, locbuf, nread + sizeof(size_t)));

		A(locsb[0], mutex, 1, SEM_UNDO);
		A(locsb[1], full, 1, SEM_UNDO);
		_(semop(semid, locsb, 2));


	}while (nread);
	
	D(locsb[0], mutex, 1, 0);
	D(locsb[1], full, 1, 0);
	_(semop(semid, locsb, 2));

	
	semctl(semid, IPC_RMID, 0);
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
