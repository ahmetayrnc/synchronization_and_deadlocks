#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include "ralloc.h"

int handling_method; // deadlock handling method
pthread_mutex_t lock;
int finished_processes;

#define M 3               // number of resource types
int exist[3] = {5, 5, 5}; // resources existing in the system

#define N 5        // number of processes - threads
pthread_t tids[N]; // ids of created threads

void *aprocess(void *p)
{
    int req[3]; //requests of the thread
    int k;
    int pid;

    pid = *((int *)p);

    printf("this is thread %d\n", pid);
    fflush(stdout);

    req[0] = 4;
    req[1] = 4;
    req[2] = 4;
    ralloc_maxdemand(pid, req);

    for (k = 0; k < 2; ++k)
    {
        req[0] = 2;
        req[1] = 2;
        req[2] = 2;
        ralloc_request(pid, req); //request resources
        sleep(1);                 //simulate usage
        ralloc_request(pid, req); //request resources
        sleep(1);                 //simulate usage
        ralloc_release(pid, req); //release resources
        ralloc_release(pid, req); //release resources
    }

    pthread_mutex_lock(&lock);
    finished_processes++;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int dn;            // number of deadlocked processes
    int deadlocked[N]; // array indicating deadlocked processes
    int k;
    int i;
    int pids[N]; //process ids array

    for (k = 0; k < N; ++k)
    {
        deadlocked[k] = -1; // initialize
    }

    handling_method = atoi(argv[1]);

    ralloc_init(N, M, exist, handling_method);

    printf("library initialized\n");
    fflush(stdout);

    //create N threads representing processes
    for (i = 0; i < N; ++i)
    {
        pids[i] = i;
        pthread_create(&(tids[i]), NULL, (void *)&aprocess,
                       (void *)&(pids[i]));
    }

    printf("threads created = %d\n", N);
    fflush(stdout);

    while (1)
    {
        sleep(10); // detection period
        if (handling_method == DEADLOCK_DETECTION)
        {
            dn = ralloc_detection(deadlocked);
            if (dn > 0)
            {
                printf("Deadlocked: ");
                for (int i = 0; i < N; i++)
                {
                    if (deadlocked[i] == 1)
                    {
                        printf("P%d ", i);
                    }
                }
                printf("\n");
            }
            else
            {
                printf("No Deadlock.\n");
            }
        }
        if (finished_processes == N)
        {
            for (int i = 0; i < N; ++i)
            {
                pthread_join(tids[i], NULL);
            }
            ralloc_end();
            exit(0);
        }
    }
}