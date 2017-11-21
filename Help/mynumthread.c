#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <pthread.h>

typedef struct buffer
{
	int i; // записывается номер нити
	int sem; // заисывается semid
	int mybufsemA;
	int mybufsemD; 
} buffer_t;
buffer_t buf = {};

void *mythread(void *dummy){
	buffer_t * dummy_buffer = (buffer_t *) dummy;
/*
	if (semop(dummy_buffer->sem, (dummy_buffer->mybufsemA), 1) < 0){ 
	//if (semop(*dummy_buffer.sem, &(*dummy_buffer.mybufsemA), 1) < 0){ // ПЕРЕДЕЛАТЬ АРГУМЕНТЫ semop
	    printf("Can\'t wait for Acondition and i = %d\n", dummy_buffer->i);
		exit(-1);
  	}
  	if (semop(dummy_buffer->sem, (dummy_buffer->mybufsemD), 1) < 0){ 
	    printf("Can\'t wait for Dcondition and i = %d\n", dummy_buffer->i);
		exit(-1);
  	}*/
	printf("thread_number %d\n", (dummy_buffer->i));
	return NULL;
	//printf("thread_number", *dummy_buffer.i); // но не i а через dummy выйти на i
}

int main(int argc, char *argv[], char *envp[]){
	int i = 0;
	key_t key = 0;
	int semid = 0;
	int result = 0;
	char pathname[] = "mynumthread.c";
	if(argc != 2) // < 2
	{
		printf("Usage: %s [number to print]\n", argv[0]);
		exit(0);
	}
	if ((key = ftok(pathname, 0)) < 0){
		printf("Can\'t generate key\n");
		exit(-1);
	} 
	if ((semid = semget (key, 1, 0666 | IPC_CREAT)) < 0){
		printf("Can\'t get semid\n");
		exit(-1);
	}

//struct sembuf mybuf[*argv[1] + 1 ]; // использую семафор типа A столько сколько и нитей, т.е. 7 и еще один сем-р типа D
	//ЧТО ЗА *argv[1] ???? Это что за тип такой ??? как ты с эти работетшь? 
	//А ведь компилятор тебе варнинги на это все писал, а ты игнорила!
	//А варнингам не приятно, когда ты их игноригшь!!!!
	int n = atoi(argv[1]);
	struct sembuf* mybuf = (struct sembuf*) calloc(n+1, sizeof(*mybuf));

	pthread_t * child_thrid = (pthread_t *)calloc(n+1, sizeof(pthread_t));
	//for ( i = 1; i <= argv[1]; i++){
	//ТОЖЕ САМОЕ!!!!!!!!!
	for (i = 0; i <= n; i++){
	//	НАХРЕН НЕ НУЖНО тк смотри ниже
/*
		mybuf[i - 1].sem_op = -(i+1); // тип A зависит от номера нити
		mybuf[i - 1].sem_num = 0;
		mybuf[i - 1].sem_flg = 0;
		mybuf[*argv[1] + 1].sem_op = -1; // тип D ни от чего не зависит
		mybuf[*argv[1] + 1].sem_num = 0;
		mybuf[*argv[1] + 1].sem_flg = 0;
*/
		buf.i = i;
		buf.sem = semid;
		buf.mybufsemA = mybuf[i - 1].sem_op;
		buf.mybufsemD = mybuf[*argv[1] + 1].sem_op;

		pthread_create( &child_thrid[i], NULL, mythread, &buf);
		pthread_join(child_thrid[i], NULL);
	//	Последняя Функция ждет выполнения потока. Нахрен семафоры ! она ждет, пока напечатается
	//	Или мне нужно пояснить задание, можно ли использовать эту функцию. Она у тебя была написана
	}
	return 0;

}