/*
 * Question 25: Setting System Resource Limits
 *
 * Uses getrlimit() to read RLIMIT_NOFILE, setrlimit() to set
 * the soft limit, and getrlimit() again to verify the change.
 *
 * Test:
 *   gcc question_25.c -o question_25
 *   ./question_25
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

static void print_limit(const char *label, rlim_t value)
{
    printf("%s: ", label);

    if (value == RLIM_INFINITY)
        printf("unlimited\n");
    else
        printf("%llu\n", (unsigned long long)value);
}

int main(void)
{
    struct rlimit current_limit;
    struct rlimit new_limit;
    struct rlimit verified_limit;
    rlim_t target_soft_limit = 512;

    printf("Resource: RLIMIT_NOFILE\n\n");

    /* Step 1: Retrieve the current limits. */
    if (getrlimit(RLIMIT_NOFILE, &current_limit) == -1) {
        perror("getrlimit");
        return EXIT_FAILURE;
    }

    printf("Before setrlimit():\n");
    print_limit("Soft limit", current_limit.rlim_cur);
    print_limit("Hard limit", current_limit.rlim_max);

    /*
     * Step 2: Choose a soft limit of 512, unless the
     * existing hard limit is lower.
     */
    if (current_limit.rlim_max != RLIM_INFINITY &&
        current_limit.rlim_max < target_soft_limit) {
        target_soft_limit = current_limit.rlim_max;
    }

    new_limit.rlim_cur = target_soft_limit;

    /* Preserve the existing hard limit. */
    new_limit.rlim_max = current_limit.rlim_max;

    /* Step 3: Apply the new limits. */
    if (setrlimit(RLIMIT_NOFILE, &new_limit) == -1) {
        perror("setrlimit");
        return EXIT_FAILURE;
    }

    printf("\nsetrlimit() completed successfully.\n");

    /* Step 4: Retrieve the limits again for verification. */
    if (getrlimit(RLIMIT_NOFILE, &verified_limit) == -1) {
        perror("getrlimit");
        return EXIT_FAILURE;
    }

    printf("\nAfter setrlimit():\n");
    print_limit("Soft limit", verified_limit.rlim_cur);
    print_limit("Hard limit", verified_limit.rlim_max);

    return EXIT_SUCCESS;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_25.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_25.c -o question_25
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_25
Resource: RLIMIT_NOFILE

Before setrlimit():
Soft limit: 1024
Hard limit: 524288

setrlimit() completed successfully.

After setrlimit():
Soft limit: 512
Hard limit: 524288
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ulimit -Sn
ulimit -Hn
1024
524288
*/