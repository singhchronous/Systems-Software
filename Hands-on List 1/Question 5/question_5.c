#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd[5];

    /* Create five files and keep their file descriptors open */
    for (int i = 0; i < 5; i++)
    {
        char filename[20];

        snprintf(filename, sizeof(filename), "file%d.txt", i + 1);

        fd[i] = creat(filename, 0644);

        if (fd[i] == -1)
        {
            perror("creat");
            return 1;
        }

        printf("%s -> FD %d\n", filename, fd[i]);
    }

    printf("PID = %d\n", getpid());

    /* Keep the process alive for /proc/PID/fd inspection */
    while (1)
    {
    }

    return 0;
}

/*
 * Test:
 * gcc question_5.c -o question_5
 * ./question_5 &
 * pgrep question_5
 * ls -l /proc/<PID>/fd
 *
 * Expected: five descriptors pointing to file1.txt ... file5.txt.
 * 
 * Output:
 * shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_5 &
[1] 5985
file1.txt -> FD 3
file2.txt -> FD 4
file3.txt -> FD 5
file4.txt -> FD 6
file5.txt -> FD 7
PID = 5985
* shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l /proc/5985/fd
total 0
lrwx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 0 -> /dev/pts/0
lrwx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 1 -> /dev/pts/0
lrwx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 2 -> /dev/pts/0
l-wx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 3 -> /home/shubham-singh/file1.txt
l-wx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 4 -> /home/shubham-singh/file2.txt
l-wx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 5 -> /home/shubham-singh/file3.txt
l-wx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 6 -> /home/shubham-singh/file4.txt
l-wx------ 1 shubham-singh shubham-singh 64 Sep 20 18:10 7 -> /home/shubham-singh/file5.txt
 */