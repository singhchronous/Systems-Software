/*
 * Question 33: Creating an Orphan Process
 *
 * The child sends SIGKILL to its parent using kill().
 * The child then observes its changed parent PID and
 * continues executing as an orphan.
 *
 * Test:
 *   gcc question_33.c -o question_33
 *   ./question_33
 *
 * Expected:
 *   Parent terminates after receiving SIGKILL.
 *   Child continues with a new parent PID and exits later.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

int main(void)
{
    pid_t parent_pid = getpid();

    printf("Original parent PID: %ld\n", (long)parent_pid);
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        pid_t original_parent = getppid();

        printf("Child created. Child PID: %ld\n", (long)getpid());
        printf("Child's original parent PID: %ld\n",
               (long)original_parent);
        fflush(stdout);

        sleep(1);

        printf("Child sending SIGKILL to parent PID %ld\n",
               (long)original_parent);
        fflush(stdout);

        if (kill(original_parent, SIGKILL) == -1) {
            perror("kill");
            return 1;
        }

        while (getppid() == original_parent)
            sleep(1);

        printf("Parent has terminated. Child is now an orphan.\n");
        printf("Child's new parent PID: %ld\n", (long)getppid());
        fflush(stdout);

        sleep(5);
        printf("Orphan child exiting. PID: %ld\n", (long)getpid());
        return 0;
    }

    printf("Parent process waiting. PID: %ld, Child PID: %ld\n",
           (long)getpid(), (long)pid);
    fflush(stdout);

    while (1)
        pause();

    return 0;
}

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ vim question_33.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ gcc question_33.c -o question_33
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ./question_33
Original parent PID: 11480
Parent process waiting. PID: 11480, Child PID: 11481
Child created. Child PID: 11481
Child's original parent PID: 11480
Child sending SIGKILL to parent PID 11480
Killed                     ./question_33
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ Parent has terminated. Child is now an orphan.

Terminal 2:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~/Hands-on List 2$ ps -eo pid,ppid,stat,cmd | grep '[q]uestion_33'
  11498    2794 S    ./question_33

Child's new parent PID: 2794
Orphan child exiting. PID: 11481
*/