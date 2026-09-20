#include <stdio.h>
#include <unistd.h>

int main(void)
{
    const char *oldpath = "original.txt";
    const char *newpath = "hardlink_syscall.txt";

    if (link(oldpath, newpath) == -1)
    {
        perror("link");
        return 1;
    }

    printf("Hard link created successfully.\n");

    return 0;
}