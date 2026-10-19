#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void)
{
    const char *path = "myfifo_d";
    struct stat info;

    /* S_IFIFO selects a FIFO; 0666 specifies requested permissions. */
    if (mknod(path, S_IFIFO | 0666, 0) == -1) {
        perror("mknod");
        return EXIT_FAILURE;
    }

    /* Confirm that the created filesystem entry is a FIFO. */
    if (stat(path, &info) == -1) {
        perror("stat");
        return EXIT_FAILURE;
    }

    if (!S_ISFIFO(info.st_mode)) {
        fprintf(stderr, "Created entry is not a FIFO\n");
        return EXIT_FAILURE;
    }

    printf("FIFO created successfully: %s\n", path);
    return EXIT_SUCCESS;
}

/*
 * Test:
 * gcc question_6_d.c -o question_6_d
 * ./question_6_d
 *
 * Expected output:
 * FIFO created successfully: myfifo_d
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_6_a.sh
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_6_b.sh
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_6_c.sh
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_6_d.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_6_e.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ chmod +x question_6_a.sh question_6_b.sh

./question_6_a.sh
./question_6_b.sh
FIFO created successfully: myfifo_a
FIFO created successfully: myfifo_b
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ls -l myfifo_a myfifo_b
file myfifo_a myfifo_b
prw-rw-r-- 1 shubham-singh shubham-singh 0 Oct 19 20:08 myfifo_a
prw-rw-r-- 1 shubham-singh shubham-singh 0 Oct 19 20:08 myfifo_b
myfifo_a: fifo (named pipe)
myfifo_b: fifo (named pipe)
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ chmod +x question_6_c.sh
./question_6_c.sh
mknod exit status: 0
mkfifo exit status: 0

mknod trace:
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 50.24    0.000210          21        10         1 openat
 19.62    0.000082          20         4           statx
 13.40    0.000056          56         1           mknodat
 11.24    0.000047          23         2         2 statfs
  5.50    0.000023          11         2         2 access
  0.00    0.000000           0         1           execve
------ ----------- ----------- --------- --------- ----------------
100.00    0.000418          20        20         5 total

mkfifo trace:
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 44.97    0.000179          17        10         1 openat
 20.35    0.000081          20         4           statx
 13.57    0.000054          54         1           mknodat
  9.80    0.000039          19         2         2 statfs
  7.29    0.000029          29         1           chmod
  4.02    0.000016           8         2         2 access
  0.00    0.000000           0         1           execve
------ ----------- ----------- --------- --------- ----------------
100.00    0.000398          18        21         5 total
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_6_d.c -o question_6_d
gcc question_6_e.c -o question_6_e

./question_6_d
./question_6_e
FIFO created successfully: myfifo_d
FIFO created successfully: myfifo_e
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ls -l myfifo_a myfifo_b myfifo_d myfifo_e
prw-rw-r-- 1 shubham-singh shubham-singh 0 Oct 19 20:08 myfifo_a
prw-rw-r-- 1 shubham-singh shubham-singh 0 Oct 19 20:08 myfifo_b
prw-rw-r-- 1 shubham-singh shubham-singh 0 Oct 19 20:09 myfifo_d
prw-rw-r-- 1 shubham-singh shubham-singh 0 Oct 19 20:09 myfifo_e
*/