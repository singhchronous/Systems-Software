#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

void daemonize(void)
{
    pid_t pid;

    /* First fork detaches the child from the original process */
    pid = fork();

    if (pid < 0)
        exit(1);

    if (pid > 0)
        exit(0);

    /* Create a new session and detach from the controlling terminal */
    if (setsid() == -1)
        exit(1);

    /* Second fork prevents the daemon from being a session leader */
    pid = fork();

    if (pid < 0)
        exit(1);

    if (pid > 0)
        exit(0);

    /* Set a predictable environment for the daemon */
    umask(0);
    chdir("/");

    /* Redirect standard file descriptors to /dev/null */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

int main(int argc, char *argv[])
{
    int hour, minute;
    time_t now, target;
    struct tm target_time;
    long wait_seconds;

    /* Check that the target hour and minute were provided */
    if (argc != 3)
    {
        printf("Usage: %s <hour> <minute>\n", argv[0]);
        return 1;
    }

    hour = atoi(argv[1]);
    minute = atoi(argv[2]);

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        printf("Invalid time.\n");
        return 1;
    }

    /* Convert the program into a daemon */
    daemonize();

    /* Obtain the current local time */
    time(&now);
    target_time = *localtime(&now);

    /* Construct today's target time */
    target_time.tm_hour = hour;
    target_time.tm_min = minute;
    target_time.tm_sec = 0;

    target = mktime(&target_time);

    /* If today's time has passed, schedule it for tomorrow */
    if (target <= now)
    {
        target_time.tm_mday++;
        target = mktime(&target_time);
    }

    /* Calculate the number of seconds until the target time */
    wait_seconds = (long)difftime(target, now);

    /* Wait until the scheduled time */
    sleep(wait_seconds);

    /* Execute the required task */
    system("/home/shubham-singhtask.sh");

    return 0;
}

/*
 * Test:
 * gcc question_30.c -o question_30
 *
 * Create the task:
 * nano task.sh
 *
 * Add:
 * #!/bin/bash
 * echo "Task executed at $(date)" >> /tmp/q30_task.log
 *
 * Make it executable:
 * chmod +x task.sh
 *
 * Replace USER in the system() path with the Linux username.
 *
 * Run the daemon:
 * ./question_30 <hour> <minute>
 *
 * Check the daemon:
 * pgrep -a question_30
 *
 * After the specified time, verify:
 * cat /tmp/q30_task.log
 *
 * The task should contain a timestamp showing when the script
 * was executed.
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim task.sh
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ chmod +x task.sh
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_30.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_30.c -o question_30
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_30 02 39
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ps -ef | grep question_30
shubham+    8329    2754  0 02:37 ?        00:00:00 ./question_30 02 39
shubham+    8332    4264  0 02:37 pts/0    00:00:00 grep --color=auto question_30
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ pgrep -a question_30
8329 ./question_30 02 39
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /tmp/q30_task.log
Task executed at Tue Sep 22 02:39:00 AM IST 2026
*/