#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void)
{
    const char *fifo_name = "myfifo_syscall";

    if (mkfifo(fifo_name, 0666) == -1)
    {
        perror("mkfifo");
        return 1;
    }

    printf("FIFO created successfully.\n");

    return 0;
}