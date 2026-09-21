#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * Question 1(c): Create a FIFO (named pipe).
 *
 * mkfifo() creates a special filesystem object used for
 * inter-process communication.
 */

int main(void)
{
    const char *fifo_name = "myfifo_syscall";

    if (mkfifo(fifo_name, 0666) == -1)
    {
        perror("mkfifo");
        return 1;
    }

    printf("FIFO created successfully.\n");

    /*
     * Test:
     *   gcc question_1_c.c -o question_1_c
     *   ./question_1_c
     *   ls -l myfifo_syscall
     *
     * Expected:
     *   The file type should be 'p', indicating a FIFO.
     *
     * IPC test:
     *   Terminal 1: cat myfifo_syscall
     *   Terminal 2: echo "Hello" > myfifo_syscall
     */

    return 0;
}