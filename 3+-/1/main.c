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

#define SEMN 5
#define SHMBUFSIZE 2048

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

const char filename[] = "main.c";

enum semp
{
	mutex = 0,
	empty = 1,
	full = 2,
	inita = 3,
	initb= 4
};

int hello()
{
	;
}

int StartClient()
{
	key_t key = 0;
	_(key =  ftok(filename, 0));

	int shmid = 0;
	_(shmid = shmget(key, SHMBUFSIZE, IPC_CREAT | 0666));

	char* shmbuf = NULL;
	_((size_t) (shmbuf = (char*) shmat(shmid, NULL, 0)));

	struct sembuf sb[SEMN] = {};
	int semid = 0;
	_(semid = semget(key, SEMN, IPC_CREAT | 0666));

	A(sb, mutex, 1);
	A(sb, empty, 1);
	_(semop(semid, sb, 2));


	A(sb, inita, 1);
	_(semop(semid, sb + inita, 1));
	
	D(sb, initb, 1);
	errno = 0;
	if (semop(semid, sb + initb, 1) < 0)
	{
		if ( errno == EIDRM )
			return 0;
		else
			return -1;
	}


	char locbuf[SHMBUFSIZE] = {};
	int nwrite = 0;
	do{
		D(sb, full, 1);
		_(semop(semid, sb + full, 1));
		D(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));

		//exit(0);
		_((size_t)memcpy(locbuf, shmbuf, SHMBUFSIZE));
		_((size_t)memset(shmbuf, 0, SHMBUFSIZE));

		A(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));
		A(sb, empty, 1);
		_(semop(semid, sb + empty, 1));

		if (!locbuf[0])
			_(write(1, locbuf+1, SHMBUFSIZE-1));
	}while(!locbuf[0]);

	_(semctl(semid, IPC_RMID, 0));
	_(shmctl(shmid, IPC_RMID, NULL));
	_(shmdt(shmbuf));
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

	struct sembuf sb[SEMN] = {};
	int semid = 0;
	_(semid = semget(key, SEMN, IPC_CREAT | 0666));
	//sleep(1);
	
	D(sb, inita, 1);
	errno = 0;
	if (semop(semid, sb + inita, 1) < 0)
	{
		if ( errno == EIDRM )
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
		_(semop(semid, sb + empty, 1));
		D(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));

		locbuf[0] = (char) !nread;
		_((size_t)memcpy(shmbuf, locbuf, nread + 1));
			

		A(sb, mutex, 1);
		_(semop(semid, sb + mutex, 1));
		A(sb, full, 1);
		_(semop(semid, sb + full, 1));


	}while (nread != 0);
	

	close(fd);

	_(shmdt(shmbuf));
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