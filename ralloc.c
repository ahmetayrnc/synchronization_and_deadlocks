#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ralloc.h"

pthread_cond_t *conds;
pthread_mutex_t lock;

int d_handling_method;

int *resources_avail = NULL;
int **processes_max = NULL;
int **processes_alloced = NULL;
int **process_requests = NULL;
int *process_waitings = NULL;

int *temp_available;
int **temp_allocation;

int resource_count;
int process_count;

int bankers(int *avail, int **alloc)
{
    int working[resource_count];
    int finished[process_count]; //keep track of finished processes
    int flag = 1;                //for the while loop
    int dead_count = 0;

    //allocate need
    int **need = malloc(process_count * sizeof(int *));
    for (int i = 0; i < process_count; i++)
    {
        need[i] = malloc(resource_count * sizeof(int));
    }

    //set working
    for (int i = 0; i < resource_count; i++)
    {
        working[i] = avail[i];
    }

    //mark all the processes as unfinished
    for (int i = 0; i < process_count; i++)
    {
        finished[i] = 0;
    }

    //find the need
    for (int i = 0; i < process_count; i++)
    {
        for (int j = 0; j < resource_count; j++)
        {
            need[i][j] = processes_max[i][j] - alloc[i][j];
        }
    }

    //while flag is true
    while (flag)
    {
        //set flag to false
        flag = 0;

        //for each process
        for (int i = 0; i < process_count; i++)
        {
            //if the process is finished continue
            if (finished[i] == 1)
            {
                continue;
            }

            int passed_resources = 0;

            //for each resource
            for (int j = 0; j < resource_count; j++)
            {
                //if need > available continue
                if (need[i][j] <= working[j])
                {
                    passed_resources++;
                }
            }

            //if all the resources is good enough
            if (passed_resources == resource_count)
            {
                //add the resources back to working
                //and mark the process as finished
                //start from the begining of the process list again
                for (int j = 0; j < resource_count; j++)
                {
                    working[j] += alloc[i][j];
                    finished[i] = 1;

                    //set the flag to 1
                    flag = 1;
                }
                break;
            }
        }
    }

    dead_count = 0;

    //count unfinished processes
    for (int i = 0; i < process_count; i++)
    {
        if (finished[i] == 0)
        {
            dead_count++;
        }
    }

    for (int i = 0; i < process_count; i++)
    {
        free(need[i]);
    }
    free(need);

    //if there is atleast 1 deadlocked process
    if (dead_count > 0)
    {
        printf("unsafe\n");
        return -1;
    }

    printf("safe\n");
    return 0;
}

int ralloc_init(int p_count, int r_count, int r_exist[], int d_handling)
{
    if (p_count > MAX_PROCESSES)
    {
        printf("P > MAX P");
        return -1;
    }

    if (r_count > MAX_RESOURCE_TYPES)
    {
        printf("R > MAX R");
        return -1;
    }

    //allocate conditional variables and init them
    conds = malloc(p_count * sizeof(pthread_cond_t));
    for (int i = 0; i < p_count; i++)
    {
        if (pthread_cond_init(&conds[i], NULL) != 0)
        {
            printf("Error creating condition variables");
            return -1;
        }
    }

    //save the parameters and allocate arrays
    d_handling_method = d_handling;

    process_count = p_count;
    resource_count = r_count;

    resources_avail = malloc(r_count * sizeof(int));
    process_waitings = malloc(p_count * sizeof(int));
    processes_max = malloc(p_count * sizeof(int *));
    processes_alloced = malloc(p_count * sizeof(int *));
    process_requests = malloc(p_count * sizeof(int *));
    temp_available = malloc(r_count * sizeof(int));
    temp_allocation = malloc(p_count * sizeof(int *));
    for (int i = 0; i < p_count; i++)
    {
        processes_max[i] = malloc(r_count * sizeof(int));
        processes_alloced[i] = malloc(r_count * sizeof(int));
        process_requests[i] = malloc(r_count * sizeof(int));
        temp_allocation[i] = malloc(r_count * sizeof(int));
    }

    for (int i = 0; i < r_count; i++)
    {
        resources_avail[i] = r_exist[i];
        temp_available[i] = 0;
    }

    for (int i = 0; i < p_count; i++)
    {
        process_waitings[i] = 0;
        for (int j = 0; j < r_count; j++)
        {
            processes_max[i][j] = 0;
            processes_alloced[i][j] = 0;
            process_requests[i][j] = 0;
            temp_allocation[i][j] = 0;
        }
    }

    return 0;
}

int ralloc_maxdemand(int pid, int r_max[])
{
    //save the max demand of the process
    for (int i = 0; i < resource_count; i++)
    {
        processes_max[pid][i] = r_max[i];
    }
    return 0;
}

int ralloc_request(int pid, int demand[])
{
    pthread_mutex_lock(&lock);

    //if the process demanded more than its spesificed max, return -1
    for (int i = 0; i < resource_count; i++)
    {
        if (processes_max[pid][i] - processes_alloced[pid][i] < demand[i])
        {
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    //check if the resources are enough for the request
    int shortage = 0;
    for (int i = 0; i < resource_count; i++)
    {
        if (resources_avail[i] < demand[i])
        {
            shortage = 1;
            break;
        }
    }

    //if the resources are not available, block the thread
    if (shortage == 1)
    {
        for (int i = 0; i < resource_count; i++)
        {
            process_requests[pid][i] = demand[i];
        }
        process_waitings[pid] = 1;
        printf("P%d, waits on cond 1\n", pid);
        pthread_cond_wait(&conds[pid], &lock);
    }

    //if the method is avoidance
    if (d_handling_method == DEADLOCK_AVOIDANCE)
    {
        //setup temp allocation array
        for (int i = 0; i < process_count; i++)
        {
            for (int j = 0; j < resource_count; j++)
            {
                temp_allocation[i][j] = processes_alloced[i][j];
            }
        }

        //setup temp available array
        for (int i = 0; i < resource_count; i++)
        {
            temp_available[i] = resources_avail[i];
        }

        //give the resources using temps
        for (int i = 0; i < resource_count; i++)
        {
            temp_available[i] -= demand[i];
            temp_allocation[pid][i] += demand[i];
        }

        //run the algorithm, if the request will cause a deadlock
        //block the thread
        if (bankers(temp_available, temp_allocation) != 0)
        {
            for (int i = 0; i < resource_count; i++)
            {
                process_requests[pid][i] = demand[i];
            }
            process_waitings[pid] = 2;
            printf("P%d, waits on cond 2\n", pid);
            pthread_cond_wait(&conds[pid], &lock);
            //after signalling

            //-----------------------
            //give resource
            // for (int i = 0; i < resource_count; i++)
            // {
            //     // resources_avail[i] -= demand[i];
            //     processes_alloced[pid][i] += demand[i];
            // }

            // //set their waiting flags and clear saved requests
            // for (int i = 0; i < resource_count; i++)
            // {
            //     process_requests[pid][i] = 0;
            // }
            // process_waitings[pid] = 0;
            // printf("P%d, got its request\n", pid);
            //-----------------------

            pthread_mutex_unlock(&lock);
            return 0;
        }
    }

    //if the requests passes the test
    //give the resources
    for (int i = 0; i < resource_count; i++)
    {
        resources_avail[i] -= demand[i];
        processes_alloced[pid][i] += demand[i];
    }

    //set their waiting flags and clear saved requests
    for (int i = 0; i < resource_count; i++)
    {
        process_requests[pid][i] = 0;
    }
    process_waitings[pid] = 0;
    printf("P%d, got its request\n", pid);

    pthread_mutex_unlock(&lock);
    return 0;
}

int ralloc_release(int pid, int demand[])
{
    pthread_mutex_lock(&lock);

    //check if its trying to release more than allocced
    for (int i = 0; i < resource_count; i++)
    {
        if (demand[i] > processes_alloced[pid][i])
        {
            printf("Can't release more than allocated");
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    //release the resources
    for (int i = 0; i < resource_count; i++)
    {
        processes_alloced[pid][i] -= demand[i];
        resources_avail[i] += demand[i];
    }

    //if no avoidance, check if we can give resources, signal if we can
    if (d_handling_method != DEADLOCK_AVOIDANCE)
    {
        for (int i = 0; i < process_count; i++)
        {
            //zero means not waiting
            if (process_waitings[i] == 0)
            {
                continue;
            }

            int allow = 0;
            for (int j = 0; j < resource_count; j++)
            {
                if (process_requests[i][j] > resources_avail[j])
                {
                    allow = -1;
                }
            }

            if (allow == 0)
            {
                pthread_cond_signal(&conds[i]);
                pthread_mutex_unlock(&lock);
                return 0;
            }
        }
        pthread_mutex_unlock(&lock);
        return 0;
    }

    //if deadlock avoidance

    //check the first waitings
    for (int i = 0; i < process_count; i++)
    {
        //looking for first waiting processes
        if (process_waitings[i] == 0)
        {
            continue;
        }

        if (process_waitings[i] == 2)
        {
            continue;
        }

        //give resources if you can
        int allow = 0;
        for (int j = 0; j < resource_count; j++)
        {
            if (process_requests[i][j] > resources_avail[j])
            {
                allow = -1;
            }
        }

        if (allow == 0)
        {
            printf("Released P%d from 1\n", i);
            pthread_cond_signal(&conds[i]);
            pthread_mutex_unlock(&lock);
            return 0;
        }
    }

    //check the second waitings
    for (int p = 0; p < process_count; p++)
    {
        //looking for second waiting processes
        if (process_waitings[p] == 0)
        {
            continue;
        }
        if (process_waitings[p] == 1)
        {
            continue;
        }

        //prepare for bankers
        //setup temps
        for (int i = 0; i < process_count; i++)
        {
            for (int j = 0; j < resource_count; j++)
            {
                temp_allocation[i][j] = processes_alloced[i][j];
            }
        }

        for (int i = 0; i < resource_count; i++)
        {
            temp_available[i] = resources_avail[i];
        }

        for (int i = 0; i < resource_count; i++)
        {
            temp_available[i] -= process_requests[p][i];
            temp_allocation[p][i] += process_requests[p][i];
        }

        //if the bankers algorithm passes, signal the thread
        if (bankers(temp_available, temp_allocation) == 0)
        {
            printf("Released P%d from 2\n", p);

            //--------------------------------------
            //give resource
            for (int i = 0; i < resource_count; i++)
            {
                resources_avail[i] -= process_requests[p][i];
                processes_alloced[p][i] += process_requests[p][i];
            }

            //set their waiting flags and clear saved requests
            for (int i = 0; i < resource_count; i++)
            {
                process_requests[p][i] = 0;
            }
            process_waitings[p] = 0;
            printf("P%d, got its request\n", p);

            //--------------------------------------

            pthread_cond_signal(&conds[p]);
            pthread_mutex_unlock(&lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

int detection(int *avail, int **alloc, int *dead)
{
    int working[resource_count];
    int finished[process_count]; //keep track of finished processes
    int flag = 1;                //for the while loop
    int dead_count = 0;

    //allocate need
    int **need = malloc(process_count * sizeof(int *));
    for (int i = 0; i < process_count; i++)
    {
        need[i] = malloc(resource_count * sizeof(int));
    }

    //set working
    for (int i = 0; i < resource_count; i++)
    {
        working[i] = avail[i];
    }

    //mark all the processes as unfinished
    for (int i = 0; i < process_count; i++)
    {
        finished[i] = 0;
    }

    //find the need
    for (int i = 0; i < process_count; i++)
    {
        for (int j = 0; j < resource_count; j++)
        {
            need[i][j] = processes_max[i][j] - alloc[i][j];
        }
    }

    //while flag is true
    while (flag)
    {
        //set flag to false
        flag = 0;

        //for each process
        for (int i = 0; i < process_count; i++)
        {
            //if the process is finished continue
            if (finished[i] == 1)
            {
                continue;
            }

            int passed_resources = 0;

            //for each resource
            for (int j = 0; j < resource_count; j++)
            {
                //if request > available continue
                if (need[i][j] <= working[j])
                {
                    passed_resources++;
                }
            }

            //if all the resources is good enough
            if (passed_resources == resource_count)
            {
                //add the resources back to available
                //and mark the process as finished
                //start from the begining of the process list again
                for (int j = 0; j < resource_count; j++)
                {
                    working[j] += alloc[i][j];
                    finished[i] = 1;

                    //set the flag to 1
                    flag = 1;
                }
                break;
            }
        }
    }

    dead_count = 0;

    //mark unfinished processes as deadlocked
    for (int i = 0; i < process_count; i++)
    {
        dead[i] = -1;
        if (finished[i] == 0)
        {
            dead[i] = 1;
            dead_count++;
        }
    }

    //if there is atleast 1 deadlocked process
    // if (dead_count > 0)
    // {
    //     printf("\n\n There is a Deadlock and the Deadlock process are\n");
    //     for (int i = 0; i < process_count; i++)
    //     {
    //         if (dead[i] == 1)
    //         {
    //             printf("P%d: %d\t", i, dead[i]);
    //         }
    //     }
    // }

    for (int i = 0; i < process_count; i++)
    {
        free(need[i]);
    }
    free(need);

    return dead_count;
}

int ralloc_detection(int procarray[])
{
    // printf("ralloc_detection called\n");
    return detection(resources_avail, processes_alloced, procarray);
}

int ralloc_end()
{
    // printf("ralloc_end called\n");

    free(conds);

    for (int i = 0; i < process_count; i++)
    {
        free(processes_max[i]);
        free(processes_alloced[i]);
        free(process_requests[i]);
        free(temp_allocation[i]);
    }
    free(temp_available);
    free(temp_allocation);
    free(process_waitings);
    free(resources_avail);
    free(processes_max);
    free(processes_alloced);
    free(process_requests);

    return 0;
}