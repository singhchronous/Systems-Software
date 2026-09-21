#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    struct stat st;

    /* Check whether a filename was provided on the command line */
    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    /* Get file information without following a symbolic link */
    if (lstat(argv[1], &st) == -1)
    {
        perror("lstat");
        return 1;
    }

    /* Identify the type of filesystem object */
    if (S_ISREG(st.st_mode))
        printf("%s: regular file\n", argv[1]);
    else if (S_ISDIR(st.st_mode))
        printf("%s: directory\n", argv[1]);
    else if (S_ISLNK(st.st_mode))
        printf("%s: symbolic link\n", argv[1]);
    else if (S_ISFIFO(st.st_mode))
        printf("%s: FIFO (named pipe)\n", argv[1]);
    else if (S_ISCHR(st.st_mode))
        printf("%s: character device\n", argv[1]);
    else if (S_ISBLK(st.st_mode))
        printf("%s: block device\n", argv[1]);
    else if (S_ISSOCK(st.st_mode))
        printf("%s: socket\n", argv[1]);
    else
        printf("%s: unknown file type\n", argv[1]);

    return 0;
}

/*
 * Test:
 * gcc question_14.c -o question_14
 *
 * Test a regular file:
 * echo "Hello Linux" > file1
 * ./question_14 file1
 *
 * Expected:
 * file1: regular file
 *
 * Test a directory:
 * mkdir mydir
 * ./question_14 mydir
 *
 * Expected:
 * mydir: directory
 *
 * Test a symbolic link:
 * ln -s file1 softlink
 * ./question_14 softlink
 *
 * Expected:
 * softlink: symbolic link
 *
 * Test a FIFO:
 * mkfifo myfifo
 * ./question_14 myfifo
 *
 * Expected:
 * myfifo: FIFO (named pipe)
 *
 * Test a character device:
 * ./question_14 /dev/null
 *
 * Expected:
 * /dev/null: character device
 */

/* 
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_14.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_14.c -o question_14
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ echo "Hello Linux" > file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 file1
file1: regular file
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ mkdir mydir
mkdir: mydir: File exists
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 mydir
mydir: directory
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ln -s file1 softlink
ln: Already exists
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l softlink
lrwxrwxrwx 1 shubham-singh shubham-singh 5 Sep 21 18:46 softlink -> file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 softlink
softlink: symbolic link
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ln file1 hardlink
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 hardlink
hardlink: regular file
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ mkfifo myfifo
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 myfifo
myfifo: FIFO (named pipe)
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 /dev/null
/dev/null: character device
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l /dev/null
crw-rw-rw- 1 root root 1, 3 Sep 21 17:40 /dev/null
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ lsblk
NAME   MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
fd0      2:0    1     4K  0 disk 
loop0    7:0    0     4K  1 loop /snap/bare/5
loop1    7:1    0  66.8M  1 loop /snap/core24/1643
loop2    7:2    0    20M  1 loop /snap/desktop-security-center/151
loop3    7:3    0 260.3M  1 loop /snap/firefox/8763
loop4    7:4    0  16.5M  1 loop /snap/firmware-updater/226
loop5    7:5    0 614.5M  1 loop /snap/gnome-46-2404/164
loop6    7:6    0  91.7M  1 loop /snap/gtk-common-themes/1535
loop7    7:7    0   1.5M  1 loop /snap/hwctl/123
loop8    7:8    0   402M  1 loop /snap/mesa-2404/1839
loop9    7:9    0  18.8M  1 loop /snap/prompting-client/222
loop10   7:10   0  11.8M  1 loop /snap/snap-store/1390
loop11   7:11   0  50.1M  1 loop /snap/snapd/27710
loop12   7:12   0  50.3M  1 loop /snap/snapd/27738
loop13   7:13   0  11.8M  1 loop /snap/snap-store/1419
loop14   7:14   0   828K  1 loop /snap/snapd-desktop-integration/391
sda      8:0    0    40G  0 disk 
├─sda1   8:1    0     1M  0 part 
└─sda2   8:2    0    40G  0 part /
sr0     11:0    1 107.9M  0 rom  /run/media/shubham-singh/CDROM
sr1     11:1    1     6G  0 rom  /run/media/shubham-singh/Ubuntu 26.04.1 LTS amd64
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 /dev/sda
/dev/sda: block device
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14 does_not_exist
lstat: No such file or directory
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_14
Usage: ./question_14 <filename>

*/