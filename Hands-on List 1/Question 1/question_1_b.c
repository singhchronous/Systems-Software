#include <stdio.h>
#include <unistd.h>

/*
 * Question 1(b): Create a hard link.
 *
 * link() creates another directory entry for the same inode
 * as the existing file. It does not create a copy.
 */

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

    /*
     * Test:
     *   gcc question_1_b.c -o question_1_b
     *   ./question_1_b
     *   ls -li original.txt hardlink_syscall.txt
     *
     * Expected:
     *   Both names should have the same inode number.
     */

    return 0;
}