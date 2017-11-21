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



#define A(sb, s, n) do{\
			sb[s].sem_num = s;\
			sb[s].sem_op = n;\
			sb[s].sem_flg = SEM_UNDO;\
		}while(0)

#define D(sb, s, n) do{\
			sb[s].sem_num = s;\
			sb[s].sem_op = -n;\
			sb[s].sem_flg = SEM_UNDO;\
		}while(0)

#define Z(sb, s) do{\
			sb[s].sem_num = s;\
			sb[s].sem_op = 0;\
			sb[s].sem_flg = SEM_UNDO;\
		}while(0)

#define SEMN 5
#define SEMINIT {1, 1, 0, 0,0}
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
	mutex = 0,
	empty = 1,
	full = 2,
	inita = 3,
	initb= 4
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
		_(semctl(semid, 0, SETALL, arg));
	}
	else if (errno == EEXIST )
	{
		_(semid = semget(key, SEMN, 0666));
		for (int i = 0; i < MAXRETR; i++)
		{
			arg.buf = &sids;
			_(semctl(semid, 0, IPC_STAT, arg));
			if ( arg.buf->sem_ctime != 0) // ctime otime !~
			{
				return semid;
			}
			sleep(1);
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

int StartClient()
{

	key_t key = 0;
	_(key =  ftok(filename, 0));

	int shmid = 0;
	_(shmid = shmget(key, SHMBUFSIZE, IPC_CREAT | 0666));

	char* shmbuf = NULL;
	_((size_t) (shmbuf = (char*) shmat(shmid, NULL, 0)));

	int semid = 0;
	struct sembuf sb[SEMN] = {};
	_(semid = InitSem(key));
	//semdump(semid);

	A(sb, inita, 1);
	_(semop(semid, sb + inita, 1));
	
	D(sb, initb, 1);
	errno = 0;
	if (semop(semid, sb + initb, 1) < 0)
	{
		if ( errno == EIDRM || errno == EINVAL)
			return 0;
		else
			return -1;
	}

	//DumpSem(semid);
	//exit(0);

	char locbuf[SHMBUFSIZE] = {};
	int nwrite = 0;
	do{
		D(sb, full, 1);
		errno = 0;
		if (semop(semid, sb + full, 1) < 0)
		{
			if (errno == EIDRM || errno == EINVAL)
				goto end;
			else
				return -1;
		}
		D(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));

		_((size_t)memcpy(locbuf, shmbuf, SHMBUFSIZE));
		_((size_t)memset(shmbuf, 0, SHMBUFSIZE));

		A(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));
		A(sb, empty, 1);
		_(semop(semid, sb + empty, 1));

		if (!locbuf[0])
			_(write(1, locbuf+1, SHMBUFSIZE-1));
	}while(!locbuf[0]);

	end:
	semctl(semid, IPC_RMID, 0);
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
	struct sembuf sb[SEMN] = {};
	_(semid = InitSem(key));

	D(sb, inita, 1);
	errno = 0;
	if (semop(semid, sb + inita, 1) < 0)
	{
		if ( errno == EIDRM || errno == EINVAL)
			return 0;
		else
			return -1;
	}
	A(sb, initb, 1);
	_(semop(semid, sb + initb, 1));
	

	size_t nread = 0;
	char locbuf[SHMBUFSIZE] = {};
	locbuf[0] = 2;
	do{
		_(nread = read(fd, locbuf+1, SHMBUFSIZE-1));
		D(sb, empty, 1);
		errno = 0;
		if (semop(semid, sb + empty, 1) < 0)
		{
			if (errno == EIDRM || errno == EINVAL)
				goto end;
			else
				return -1;
		}
		D(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));

		locbuf[0] = (char) !nread;
		_((size_t)memcpy(shmbuf, locbuf, nread + 1));
			

		A(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));
		A(sb, full, 1);
		_(semop(semid, sb + full, 1));


	}while (nread);
	
	end:
	semctl(semid, IPC_RMID, 0);
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