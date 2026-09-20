#include <stdio.h>
#include <unistd.h>

int main(void)
{
    if (symlink("original.txt", "softlink_syscall.txt") == -1)
    {
        perror("symlink");
        return 1;
    }

    printf("Symbolic link created successfully.\n");

    return 0;
}