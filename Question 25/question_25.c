#include <stdio.h>
#include <unistd.h>

int main(void)
{
    /* Execute the target executable and pass a name as an argument */
    printf("Executing target program...\n");

    if (execl("./target", "./target", "Shubham", NULL) == -1)
    {
        perror("execl");
        return 1;
    }

    return 0;
}

/*
 * Test:
 * gcc target.c -o target
 * gcc question_25.c -o question_25
 * ./question_25
 *
 * Expected output:
 * Executing target program...
 * Hello, Shubham!
 *
 * The target program replaces question_25 in the same process.
 *
 * To test the target directly:
 * ./target Shubham
 *
 * Expected:
 * Hello, Shubham!
 */

/*
Output:
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim target.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_25.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc target.c -o target
gcc question_25.c -o question_25
./question_25
Executing target program...
Hello, Shubham!
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./target Shubham
Hello, Shubham!
*/