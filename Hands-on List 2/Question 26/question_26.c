/*
 * Question 26: Measure 100 getppid() calls using the x86 TSC.
 * Uses RDTSC with LFENCE ordering to obtain timestamp readings.
 *
 * Test:
 *   gcc question_26.c -o question_26
 *   ./question_26
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

#if !defined(__i386__) && !defined(__x86_64__)
#error "This program requires an x86 or x86-64 processor."
#endif

#define ITERATIONS 100

static inline uint64_t read_tsc(void)
{
    unsigned int low, high;

    __asm__ volatile (
        "lfence\n\t"
        "rdtsc\n\t"
        "lfence\n\t"
        : "=a"(low), "=d"(high)
        :
        : "memory"
    );

    return ((uint64_t)high << 32) | low;
}

int main(void)
{
    uint64_t start, end, total_ticks;
    volatile pid_t parent_pid = 0;

    /*
     * Start measurement.
     */
    start = read_tsc();

    /*
     * Execute getppid() exactly 100 times.
     */
    for (int i = 0; i < ITERATIONS; i++) {
        parent_pid = getppid();
    }

    /*
     * End measurement.
     */
    end = read_tsc();

    total_ticks = end - start;

    printf("Number of getppid() calls: %d\n", ITERATIONS);
    printf("Parent PID (last call): %ld\n", (long)parent_pid);
    printf("Total TSC ticks: %llu\n",
           (unsigned long long)total_ticks);
    printf("Average TSC ticks per call: %.2f\n",
           (double)total_ticks / ITERATIONS);

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_26.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_26.c -o question_26
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_26
Number of getppid() calls: 100
Parent PID (last call): 8281
Total TSC ticks: 145468
Average TSC ticks per call: 1454.68
*/