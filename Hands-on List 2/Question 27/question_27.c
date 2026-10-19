/*
 * Question 27: System Limitation Exploration
 *
 * Queries exec argument/environment limits, process limits,
 * clock ticks, open-file limits, page size, total physical pages,
 * and currently available physical pages.
 *
 * Test:
 *   gcc question_27.c -o question_27
 *   ./question_27
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#include <limits.h>

static void print_sysconf_value(const char *description, int name)
{
    errno = 0;
    long value = sysconf(name);

    if (value == -1) {
        if (errno == 0) {
            printf("%s: indeterminate or unsupported\n", description);
        } else {
            perror(description);
        }
        return;
    }

    printf("%s: %ld\n", description, value);
}

static void print_rlimit_value(const char *description, rlim_t value)
{
    if (value == RLIM_INFINITY) {
        printf("%s: unlimited\n", description);
    } else {
        printf("%s: %llu\n",
               description, (unsigned long long)value);
    }
}

static void print_open_file_limit(void)
{
    struct rlimit limit;

    printf("\n(d) Maximum number of open files\n");

    print_sysconf_value("    sysconf(_SC_OPEN_MAX)", _SC_OPEN_MAX);

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        perror("    getrlimit(RLIMIT_NOFILE)");
        return;
    }

    print_rlimit_value("    Current soft RLIMIT_NOFILE", limit.rlim_cur);
    print_rlimit_value("    Current hard RLIMIT_NOFILE", limit.rlim_max);
}

static void print_process_limit(void)
{
    printf("\n(b) Maximum simultaneous processes per real user ID\n");

    print_sysconf_value("    sysconf(_SC_CHILD_MAX)", _SC_CHILD_MAX);

#ifdef RLIMIT_NPROC
    struct rlimit limit;

    if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
        perror("    getrlimit(RLIMIT_NPROC)");
    } else {
        print_rlimit_value("    Current soft RLIMIT_NPROC", limit.rlim_cur);
        print_rlimit_value("    Current hard RLIMIT_NPROC", limit.rlim_max);
    }
#endif
}

int main(void)
{
    long page_size;
    long total_pages;
    long available_pages;

    printf("System Limitation Exploration\n");
    printf("==============================\n");

    /* (a) Maximum combined argument and environment size for exec. */
    printf("\n(a) Maximum exec argument/environment size\n");
    print_sysconf_value("    sysconf(_SC_ARG_MAX)", _SC_ARG_MAX);

    /* (b) Maximum simultaneous processes per real user ID. */
    print_process_limit();

    /* (c) Clock ticks per second for times()-related interfaces. */
    printf("\n(c) Clock ticks per second\n");
    print_sysconf_value("    sysconf(_SC_CLK_TCK)", _SC_CLK_TCK);
    printf("    Note: this is not necessarily the kernel jiffy frequency.\n");

    /* (d) Maximum number of open files. */
    print_open_file_limit();

    /* (e) Memory page size. */
    printf("\n(e) Size of a memory page\n");
    errno = 0;
    page_size = sysconf(_SC_PAGESIZE);

    if (page_size == -1) {
        if (errno == 0)
            printf("    Page size is indeterminate or unsupported.\n");
        else
            perror("    sysconf(_SC_PAGESIZE)");
    } else {
        printf("    Page size: %ld bytes\n", page_size);
    }

    /* (f) Total physical memory pages on Linux. */
    printf("\n(f) Total number of physical memory pages\n");
#ifdef _SC_PHYS_PAGES
    errno = 0;
    total_pages = sysconf(_SC_PHYS_PAGES);

    if (total_pages == -1) {
        if (errno == 0)
            printf("    Total physical pages: indeterminate or unsupported\n");
        else
            perror("    sysconf(_SC_PHYS_PAGES)");
    } else {
        printf("    Total physical pages: %ld\n", total_pages);

        if (page_size > 0)
            printf("    Approximate physical memory: %.2f MiB\n",
                   ((double)total_pages * page_size) / (1024.0 * 1024.0));
    }
#else
    printf("    _SC_PHYS_PAGES is not available on this system.\n");
#endif

    /* (g) Currently available physical memory pages on Linux. */
    printf("\n(g) Currently available physical memory pages\n");
#ifdef _SC_AVPHYS_PAGES
    errno = 0;
    available_pages = sysconf(_SC_AVPHYS_PAGES);

    if (available_pages == -1) {
        if (errno == 0) {
            printf("    Available physical pages: indeterminate or unsupported\n");
        } else {
            perror("    sysconf(_SC_AVPHYS_PAGES)");
        }
    } else {
        printf("    Available physical pages: %ld\n", available_pages);

        if (page_size > 0)
            printf("    Approximate available memory: %.2f MiB\n",
                   ((double)available_pages * page_size) / (1024.0 * 1024.0));
    }
#else
    printf("    _SC_AVPHYS_PAGES is not available on this system.\n");
#endif

    return EXIT_SUCCESS;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_27.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_27.c -o question_27
./question_27
System Limitation Exploration
==============================

(a) Maximum exec argument/environment size
    sysconf(_SC_ARG_MAX): 2097152

(b) Maximum simultaneous processes per real user ID
    sysconf(_SC_CHILD_MAX): 21260
    Current soft RLIMIT_NPROC: 21260
    Current hard RLIMIT_NPROC: 21260

(c) Clock ticks per second
    sysconf(_SC_CLK_TCK): 100
    Note: this is not necessarily the kernel jiffy frequency.

(d) Maximum number of open files
    sysconf(_SC_OPEN_MAX): 1024
    Current soft RLIMIT_NOFILE: 1024
    Current hard RLIMIT_NOFILE: 524288

(e) Size of a memory page
    Page size: 4096 bytes

(f) Total number of physical memory pages
    Total physical pages: 1372304
    Approximate physical memory: 5360.56 MiB

(g) Currently available physical memory pages
    Available physical pages: 524227
    Approximate available memory: 2047.76 MiB
*/