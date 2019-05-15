#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include "ralloc.h"
#include <sys/time.h>

int handling_method; // deadlock handling method
pthread_mutex_t lock;
int finished_processes;

void *aprocess(void *p)
{
    int req[10]; //requests of the thread
    int pid;
    pid = *((int *)p);

    printf("Thread: %d\n", pid);

    for (int i = 0; i < 10; i++)
    {
        req[i] = 4;
    }
    ralloc_maxdemand(pid, req);

    for (int i = 0; i < 10; i++)
    {
        req[i] = 2;
    }

    for (int k = 0; k < 100; k++)
    {
        ralloc_request(pid, req); //request resources
        sleep(1);
        ralloc_request(pid, req);
        sleep(1);
        ralloc_release(pid, req); //request resources
        ralloc_release(pid, req); //request resources
    }

    pthread_mutex_lock(&lock);
    finished_processes++;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

//test p_count, r_count, deadlock_mode, resource count, max_resource_demand, request amount
int main(int argc, char **argv)
{
    int dn;
    int *deadlocked;
    int N;
    int M;
    int h;
    int r;
    int *exist;
    int *pids;
    pthread_t *tids;
    int *threads;

    N = atoi(argv[1]);
    M = atoi(argv[2]);
    h = atoi(argv[3]);
    r = atoi(argv[4]);

    printf("Process count: %d\n", N);
    printf("Resource type count: %d\n", M);
    printf("Resource count per type: %d\n", r);

    if (h == 0)
    {
        handling_method = DEADLOCK_NOTHING;
        printf("Deadlock Mode: NOTHING\n");
    }
    else if (h == 1)
    {
        handling_method = DEADLOCK_AVOIDANCE;
        printf("Deadlock Mode: AVOIDANCE\n");
    }
    else if (h == 2)
    {
        handling_method = DEADLOCK_DETECTION;
        printf("Deadlock Mode: DETECTION\n");
    }
    printf("----\n\n");

    exist = malloc(M * sizeof(int));
    for (int i = 0; i < M; i++)
    {
        exist[i] = r;
    }

    deadlocked = malloc(N * sizeof(int));
    pids = malloc(N * sizeof(int));
    tids = malloc(N * sizeof(pthread_t));
    threads = malloc(N * sizeof(int));

    if (ralloc_init(N, M, exist, handling_method) < 0)
    {
        printf("Error creating ralloc");
        exit(-1);
    }

    for (int i = 0; i < N; ++i)
    {
        pids[i] = i;
        threads[i] = pthread_create(&(tids[i]), NULL, (void *)&aprocess, (void *)&(pids[i]));
    }

    while (1)
    {
        sleep(10); // detection period
        if (handling_method == DEADLOCK_DETECTION)
        {
            //-- measurement start --
            struct timeval tv1, tv2;
            gettimeofday(&tv1, NULL);

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

            gettimeofday(&tv2, NULL);
            //-- measurement end --

            printf("-- DONE! --\n");
            printf("Total time = %f seconds\n",
                   (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
                       (double)(tv2.tv_sec - tv1.tv_sec));
        }
        if (finished_processes == N)
        {
            for (int i = 0; i < N; ++i)
            {
                pthread_join(tids[i], NULL);
            }
            ralloc_end();

            free(exist);
            free(pids);
            free(tids);
            free(threads);
            free(deadlocked);

            exit(0);
        }
    }

    // for (int i = 0; i < N; ++i)
    // {
    //     pthread_join(tids[i], NULL);
    // }
    // gettimeofday(&tv2, NULL);
    // //-- measurement end --

    // printf("-- DONE! --\n");
    // printf("Total time = %f seconds\n",
    //        (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
    //            (double)(tv2.tv_sec - tv1.tv_sec));

    // ralloc_end();

    // free(exist);
    // free(pids);
    // free(tids);
    // free(threads);
}