#include <stdio.h>
#include <unistd.h>

/*
 * Question 1(a): Create a soft/symbolic link.
 *
 * symlink() creates a new filesystem object that stores
 * the pathname of the target file. It does not copy the
 * target's data.
 */

int main(void)
{
    const char *target = "original.txt";
    const char *linkname = "softlink_syscall.txt";

    if (symlink(target, linkname) == -1)
    {
        perror("symlink");
        return 1;
    }

    printf("Symbolic link created successfully.\n");

    /*
     * Test:
     *   gcc question_1_a.c -o question_1_a
     *   ./question_1_a
     *   ls -li original.txt softlink_syscall.txt
     *
     * Expected:
     *   The link should have a different inode from the target
     *   and should point to original.txt.
     */

    return 0;
}