/*
 * Question 28: Multithreading Exploration
 * Creates three threads using pthread_create() and prints their IDs.
 *
 * Test:
 *   gcc question_28.c -o question_28 -pthread
 *   ./question_28
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NUM_THREADS 3

void *thread_function(void *arg)
{
    int thread_number = *(int *)arg;
    pthread_t tid = pthread_self();

    printf("Worker %d: Thread ID = %lu\n",
           thread_number, (unsigned long)tid);

    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int thread_numbers[NUM_THREADS];

    printf("Creating %d threads...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_numbers[i] = i + 1;

        int rc = pthread_create(
            &threads[i],
            NULL,
            thread_function,
            &thread_numbers[i]
        );

        if (rc != 0) {
            fprintf(stderr, "pthread_create failed: %s\n",
                    strerror(rc));

            for (int j = 0; j < i; j++)
                pthread_join(threads[j], NULL);

            return EXIT_FAILURE;
        }

        printf("Main: Created thread %d, ID = %lu\n",
               i + 1, (unsigned long)threads[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        int rc = pthread_join(threads[i], NULL);

        if (rc != 0) {
            fprintf(stderr, "pthread_join failed: %s\n",
                    strerror(rc));
            return EXIT_FAILURE;
        }
    }

    printf("All threads have completed.\n");

    return EXIT_SUCCESS;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_28.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_28.c -o question_28 -pthread
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_28
Creating 3 threads...
Main: Created thread 1, ID = 130570994579136
Worker 1: Thread ID = 130570994579136
Main: Created thread 2, ID = 130570986186432
Worker 2: Thread ID = 130570986186432
Worker 3: Thread ID = 130570977793728
Main: Created thread 3, ID = 130570977793728
All threads have completed.
*/