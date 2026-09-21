#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

int main(void)
{
    struct stat st;
    char *filename = "file1";

    /* Get file metadata */
    if (stat(filename, &st) == -1)
    {
        perror("stat");
        return 1;
    }

    printf("Inode: %lu\n", st.st_ino);
    printf("Hard links: %lu\n", st.st_nlink);
    printf("UID: %u\n", st.st_uid);
    printf("GID: %u\n", st.st_gid);
    printf("Size: %ld bytes\n", st.st_size);
    printf("Block size: %ld bytes\n", st.st_blksize);
    printf("Blocks: %ld\n", st.st_blocks);

    printf("Last access: %s", ctime(&st.st_atime));
    printf("Last modification: %s", ctime(&st.st_mtime));
    printf("Last status change: %s", ctime(&st.st_ctime));

    return 0;
}

/*
 * Test:
 * echo "Hello from Linux" > file1
 * gcc question_9.c -o question_9
 * ./question_9
 * stat file1
 *
 * The program should display the same basic metadata
 * reported by the stat command.
 */

/*
Output:

shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ vim question_9.c
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ echo "Hello from Linux" > file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ gcc question_9.c -o question_9
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_9
Inode: 2360487
Hard links: 1
UID: 1000
GID: 1000
Size: 17 bytes
Block size: 4096 bytes
Blocks: 8
Last access: Sun Sep 20 18:41:54 2026
Last modification: Sun Sep 20 18:51:49 2026
Last status change: Sun Sep 20 18:51:49 2026
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ stat file1
  File: file1
  size: 17        	Blocks: 8          IO Block: 4096   regular file
Device: 8,2	Inode: 2360487     Links: 1
Access: (0664/-rw-rw-r--)  Uid: ( 1000/shubham-singh)   Gid: ( 1000/shubham-singh)
Access: 2026-09-20 18:41:54.637776671 +0530
Modify: 2026-09-20 18:51:49.895256873 +0530
Change: 2026-09-20 18:51:49.895256873 +0530
 Birth: 2026-09-20 18:33:22.740503742 +0530
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -i file1
2360487 file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ wc -c file1
17 file1
shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ls -l file1
-rw-rw-r-- 1 shubham-singh shubham-singh 17 Sep 20 18:51 file1


*/