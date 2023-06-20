#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#define QUEUE_SIZE 30
struct Queue 
{
    int insert;
    int remove;
    int storage[QUEUE_SIZE];
};
struct Queue * queuePtr;
int numItems = 100;
int consumerPid;
sem_t *prodsem;
sem_t *consem;
void enqueue(int item)
{
    queuePtr->storage[queuePtr->insert++] = item;
    if (queuePtr->insert >= QUEUE_SIZE) queuePtr->insert = 0;
    //kill(consumerPid, SIGUSR1);
}
void producer()
{
    //sleep(1);  // make sure consumer is running
    for (int i=0; i<=numItems; ++i)
    {
    //    printf("%d ", i);
    //    fflush(stdout);
        sem_wait(prodsem);
        enqueue(i);
        sem_post(consem);
    }
    enqueue(-1);
    printf("producer finished producing\n");
    waitpid(consumerPid, 0, 0);
}
int rxCount = 0;
bool done = false;
int dequeue()
{
    int result = queuePtr->storage[queuePtr->remove++];
    if (queuePtr->remove >= QUEUE_SIZE) queuePtr->remove = 0;
    return result;
}
void consumer()
{
    consem=sem_open("ConSem",O_CREAT,S_IRUSR|S_IWUSR,0);
    while ( ! done){
        sem_wait(consem);
        int value=dequeue();
        if(value!=-1){
            printf("%d ",value);
        }
        sem_post(prodsem);
        if(value==-1){
            done=true;
        }
    }
}
int main(int argc, char **argv)
{
    prodsem=sem_open("ProdSem",O_CREAT,S_IRUSR|S_IWUSR,30);
    consem=sem_open("ConSem",O_CREAT,S_IRUSR|S_IWUSR,0);
    if (argc > 2)
    {
        printf("Usage:  %s [items]\n", argv[0]);
	return 1;
    }
    if (argc == 2)
    {
        numItems = atoi(argv[1]);
    }
    queuePtr = mmap(NULL, 
                    sizeof(struct Queue), 
                    PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (queuePtr == MAP_FAILED)
    {
        perror("mmap failed");
        return 4;
    }
    int pid = fork();
    if (pid < 0)
    {
        perror("error in fork");
    }
    if (pid == 0) 
    {
        // child
        printf("consumer starting\n");
        consumer();
    } else {
        // parent
        printf("producer starting\n");
        consumerPid = pid;
        producer();
    }
}
