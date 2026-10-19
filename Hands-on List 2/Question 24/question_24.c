/*
 * Question 24: System Resource Limits using getrlimit()
 *
 * Prints the soft and hard limits for available process resources.
 * RLIM_INFINITY is displayed as "unlimited".
 *
 * Test:
 *   gcc question_24.c -o question_24
 *   ./question_24
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

struct resource_info {
    int resource;
    const char *name;
    const char *unit;
};

static void print_limit_value(rlim_t value, const char *unit)
{
    if (value == RLIM_INFINITY) {
        printf("unlimited");
    } else {
        printf("%llu %s", (unsigned long long)value, unit);
    }
}

static void print_resource_limit(const struct resource_info *info)
{
    struct rlimit limit;

    if (getrlimit(info->resource, &limit) == -1) {
        perror(info->name);
        return;
    }

    printf("%-18s Soft: ", info->name);
    print_limit_value(limit.rlim_cur, info->unit);

    printf(" | Hard: ");
    print_limit_value(limit.rlim_max, info->unit);
    printf("\n");
}

int main(void)
{
    const struct resource_info resources[] = {
        {RLIMIT_CPU,    "RLIMIT_CPU",    "seconds"},
        {RLIMIT_FSIZE,  "RLIMIT_FSIZE",  "bytes"},
        {RLIMIT_DATA,   "RLIMIT_DATA",   "bytes"},
        {RLIMIT_STACK,  "RLIMIT_STACK",  "bytes"},
        {RLIMIT_CORE,   "RLIMIT_CORE",   "bytes"},
        {RLIMIT_RSS,    "RLIMIT_RSS",    "bytes"},
        {RLIMIT_NOFILE, "RLIMIT_NOFILE", "descriptors"},
        {RLIMIT_AS,     "RLIMIT_AS",     "bytes"},
#ifdef RLIMIT_NPROC
        {RLIMIT_NPROC,  "RLIMIT_NPROC",  "processes/threads"},
#endif
#ifdef RLIMIT_MEMLOCK
        {RLIMIT_MEMLOCK, "RLIMIT_MEMLOCK", "bytes"},
#endif
#ifdef RLIMIT_LOCKS
        {RLIMIT_LOCKS,  "RLIMIT_LOCKS",  "locks"},
#endif
#ifdef RLIMIT_SIGPENDING
        {RLIMIT_SIGPENDING, "RLIMIT_SIGPENDING", "signals"},
#endif
#ifdef RLIMIT_MSGQUEUE
        {RLIMIT_MSGQUEUE, "RLIMIT_MSGQUEUE", "bytes"},
#endif
#ifdef RLIMIT_NICE
        {RLIMIT_NICE,   "RLIMIT_NICE",   "priority value"},
#endif
#ifdef RLIMIT_RTPRIO
        {RLIMIT_RTPRIO, "RLIMIT_RTPRIO", "priority"},
#endif
    };

    size_t count = sizeof(resources) / sizeof(resources[0]);

    printf("System Resource Limits\n");
    printf("======================\n");

    for (size_t i = 0; i < count; i++) {
        print_resource_limit(&resources[i]);
    }

    return EXIT_SUCCESS;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_24.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_24.c -o question_24
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_24
System Resource Limits
======================
RLIMIT_CPU         Soft: unlimited | Hard: unlimited
RLIMIT_FSIZE       Soft: unlimited | Hard: unlimited
RLIMIT_DATA        Soft: unlimited | Hard: unlimited
RLIMIT_STACK       Soft: 8388608 bytes | Hard: unlimited
RLIMIT_CORE        Soft: 0 bytes | Hard: unlimited
RLIMIT_RSS         Soft: unlimited | Hard: unlimited
RLIMIT_NOFILE      Soft: 1024 descriptors | Hard: 524288 descriptors
RLIMIT_AS          Soft: unlimited | Hard: unlimited
RLIMIT_NPROC       Soft: 21260 processes/threads | Hard: 21260 processes/threads
RLIMIT_MEMLOCK     Soft: 8388608 bytes | Hard: 8388608 bytes
RLIMIT_LOCKS       Soft: unlimited | Hard: unlimited
RLIMIT_SIGPENDING  Soft: 21260 signals | Hard: 21260 signals
RLIMIT_MSGQUEUE    Soft: 819200 bytes | Hard: 819200 bytes
RLIMIT_NICE        Soft: 0 priority value | Hard: 0 priority value
RLIMIT_RTPRIO      Soft: 0 priority | Hard: 0 priority
*/