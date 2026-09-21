#include <stdio.h>
#include <unistd.h>

/*
 * Question 2: Execute an infinite-loop process and inspect
 * its process information through /proc/<PID>.
 *
 * getpid() returns the PID assigned to this process.
 * The infinite loop keeps the process running so that its
 * corresponding /proc/<PID> directory can be inspected.
 */

int main(void)
{
    printf("Process started. PID = %d\n", getpid());

    while (1)
    {
    }

    return 0;
}

/*
 * Test:
 *
 *     gcc question_2.c -o question_2
 *     ./question_2 &
 *
 * Find PID:
 *
 *     pgrep question_2
 *
 * Inspect:
 *
 *     ls /proc/<PID>
 *     cat /proc/<PID>/status
 *     cat /proc/<PID>/stat
 *     cat /proc/<PID>/statm
 *     cat /proc/<PID>/maps
 *     ls -l /proc/<PID>/fd
 *
 * Terminate:
 *
 *     kill <PID>
 * 
 * Output:
 * 
 * shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ ./question_2 &
[1] 5583
Process started. PID = 5583
* shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ pgrep question_2
5583
* shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /proc/5583/status
Name:	question_2
Umask:	0002
State:	S (sleeping)
Tgid:	5583
Ngid:	0
Pid:	5583
PPid:	4401
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	4 24 27 30 46 100 111 114 1000 
NStgid:	5583
NSpid:	5583
NSpgid:	5583
NSsid:	4401
Kthread:	0
VmPeak:	    2768 kB
VmSize:	    2768 kB
VmLck:	       0 kB
VmPin:	       0 kB
VmHWM:	    1936 kB
VmRSS:	    1936 kB
RssAnon:	     100 kB
RssFile:	    1836 kB
RssShmem:	       0 kB
VmData:	     224 kB
VmStk:	     132 kB
VmExe:	       4 kB
VmLib:	    1828 kB
VmPTE:	      56 kB
VmSwap:	       0 kB
HugetlbPages:	       0 kB
CoreDumping:	0
THP_enabled:	1
untag_mask:	0xffffffffffffffff
Threads:	1
SigQ:	0/12756
SigPnd:	0000000000000000
ShdPnd:	0000000000000000
SigBlk:	0000000000000000
SigIgn:	0000000000000000
SigCgt:	0000000000000000
CapInh:	0000000800000000
CapPrm:	0000000000000000
CapEff:	0000000000000000
CapBnd:	000001ffffffffff
CapAmb:	0000000000000000
NoNewPrivs:	0
Seccomp:	0
Seccomp_filters:	0
Speculation_Store_Bypass:	thread vulnerable
SpeculationIndirectBranch:	conditional enabled
Cpus_allowed:	ffffffff,ffffffff,ffffffff,ffffffff
Cpus_allowed_list:	0-127
Mems_allowed:	00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000000,00000001
Mems_allowed_list:	0
voluntary_ctxt_switches:	49
nonvoluntary_ctxt_switches:	0
x86_Thread_features:	
x86_Thread_features_locked:	
* shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /proc/5583/stat
5583 (question_2) S 4401 5583 4401 34816 5594 4194304 144 0 0 0 0 0 0 0 20 0 1 0 615468 2834432 459 18446744073709551615 106986552643584 106986552644041 140722661925456 0 0 0 0 0 0 1 0 0 17 2 0 0 0 0 0 106986552655272 106986552655888 106987537862656 140722661933521 140722661933534 140722661933534 140722661937131 0
* shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /proc/5583/statm
692 484 459 1 0 89 0
* shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ cat /proc/5583/maps
614dbf774000-614dbf775000 r--p 00000000 08:02 2359609                    /home/shubham-singh/question_2
614dbf775000-614dbf776000 r-xp 00001000 08:02 2359609                    /home/shubham-singh/question_2
614dbf776000-614dbf777000 r--p 00002000 08:02 2359609                    /home/shubham-singh/question_2
614dbf777000-614dbf778000 r--p 00002000 08:02 2359609                    /home/shubham-singh/question_2
614dbf778000-614dbf779000 rw-p 00003000 08:02 2359609                    /home/shubham-singh/question_2
614dfa309000-614dfa32a000 rw-p 00000000 00:00 0                          [heap]
73f93fe00000-73f93fe28000 r--p 00000000 08:02 656945                     /usr/lib/x86_64-linux-gnu/libc.so.6
73f93fe28000-73f93ffc0000 r-xp 00028000 08:02 656945                     /usr/lib/x86_64-linux-gnu/libc.so.6
73f93ffc0000-73f94000e000 r--p 001c0000 08:02 656945                     /usr/lib/x86_64-linux-gnu/libc.so.6
73f94000e000-73f940012000 r--p 0020e000 08:02 656945                     /usr/lib/x86_64-linux-gnu/libc.so.6
73f940012000-73f940014000 rw-p 00212000 08:02 656945                     /usr/lib/x86_64-linux-gnu/libc.so.6
73f940014000-73f940021000 rw-p 00000000 00:00 0 
73f9401c4000-73f9401c7000 rw-p 00000000 00:00 0 
73f9401d7000-73f9401d9000 rw-p 00000000 00:00 0 
73f9401d9000-73f9401dd000 r--p 00000000 00:00 0                          [vvar]
73f9401dd000-73f9401df000 r--p 00000000 00:00 0                          [vvar_vclock]
73f9401df000-73f9401e1000 r-xp 00000000 00:00 0                          [vdso]
73f9401e1000-73f9401e2000 r--p 00000000 08:02 656942                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
73f9401e2000-73f940211000 r-xp 00001000 08:02 656942                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
73f940211000-73f94021c000 r--p 00030000 08:02 656942                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
73f94021c000-73f94021e000 r--p 0003b000 08:02 656942                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
73f94021e000-73f94021f000 rw-p 0003d000 08:02 656942                     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
73f94021f000-73f940220000 rw-p 00000000 00:00 0 
7ffc8c44c000-7ffc8c46d000 rw-p 00000000 00:00 0                          [stack]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
* shubham-singh@shubham-singh-VMware-Virtual-Platform:~$ kill 5583
[1]+  Terminated                 ./question_2

 */