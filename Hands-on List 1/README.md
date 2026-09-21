# Systems Software — Hands-on List 1

This repository contains the solutions for **Hands-on List 1** of the **Systems Software** course.

The exercises cover Linux system calls, file management, file descriptors, process management, inter-process communication, file locking, process scheduling, and daemon processes.

## Environment

* **OS:** Linux / Ubuntu
* **Language:** C
* **Shell:** Bash
* **Compiler:** GCC
* **System interfaces:** POSIX / Linux system calls and library functions

---

# I. File Management, I/O, Locking and System Calls

### 1. Creating Different File Types

Create different types of filesystem objects using shell commands and system calls:

* Soft link using `symlink()`
* Hard link using `link()`
* FIFO using `mkfifo()` or `mknod()`

**Directory:** `Question 1/`

---

### 2. Background Process Exploration

Develop a program that executes indefinitely in the background and explore the corresponding process information available through the `/proc` filesystem.

**Directory:** `Question 2/`

---

### 3. File Creation and Descriptor Printing

Create a file using the `creat()` system call and print the file descriptor returned by the system call.

**Directory:** `Question 3/`

---

### 4. Opening Existing File

Open an existing file in read-write mode and experiment with the `O_EXCL` flag.

**Directory:** `Question 4/`

---

### 5. Continuous File Creation

Create five files and keep the process running in the background. Inspect the process's file descriptor table using:

```bash
/proc/<pid>/fd
```

**Directory:** `Question 5/`

---

### 6. Input and Output Using System Calls

Read input from `STDIN` and write it to `STDOUT` using only the `read()` and `write()` system calls.

**Directory:** `Question 6/`

---

### 7. File Copying

Copy the contents of `file1` into `file2`, implementing functionality similar to:

```bash
cp file1 file2
```

**Directory:** `Question 7/`

---

### 8. Read-Only File Reading

Open a file in read-only mode, read it line by line, display each line, and close the file after reaching EOF.

**Directory:** `Question 8/`

---

### 9. File Information Extraction

Use file metadata to display:

* Inode number
* Number of hard links
* UID
* GID
* File size
* Block size
* Number of blocks
* Last access time
* Last modification time
* Last status-change time

**Directory:** `Question 9/`

---

### 10. File Write and Seek

Open a file in read-write mode, write 10 bytes, move the file pointer by 10 bytes using `lseek()`, and write another 10 bytes.

Also:

* Check the return value of `lseek()`
* Examine the resulting file using `od`
* Observe the empty/sparse region between the two sets of data

**Directory:** `Question 10/`

---

### 11. File Descriptor Duplication and Appending

Open a file, duplicate its file descriptor, and perform writes using both descriptors.

Implement the operation using:

* `dup()`
* `dup2()`
* `fcntl()`

**Directory:** `Question 11/`

---

### 12. Determining Opening Mode of a File

Use `fcntl()` to determine whether a file was opened in:

* Read-only mode
* Write-only mode
* Read-write mode

**Directory:** `Question 12/`

---

### 13. Waiting for STDIN Using `select()`

Wait for input from `STDIN` for 10 seconds using `select()` and display whether input became available within the specified time.

**Directory:** `Question 13/`

---

### 14. File Type Identification

Accept a filename from the command line and identify the type of filesystem object.

The program recognizes types such as:

* Regular file
* Directory
* Symbolic link
* FIFO
* Character device
* Block device
* Socket

**Directory:** `Question 14/`

---

### 15. Displaying User Environment Variables

Display the environment variables inherited by the process using the `environ` variable.

**Directory:** `Question 15/`

---

### 16. File Locking

Implement file locking using:

* Write lock
* Read lock

The implementation demonstrates locking using `fcntl()` record-locking mechanisms.

**Directory:** `Question 16/`

---

### 17. Online Ticket Reservation Simulation

Simulate concurrent ticket reservation using an exclusive write lock.

The solution consists of:

1. A program that initializes and stores the ticket number.
2. A program that:

   * Obtains a write lock
   * Reads the current ticket number
   * Increments it
   * Writes the updated number
   * Releases the lock

This demonstrates synchronization and prevention of race conditions.

**Directory:** `Question 17/`

---

### 18. Record Locking

Implement record-level locking with:

* Write lock
* Read lock

Create three records in a file and lock the required record before accessing or modifying it.

**Directory:** `Question 18/`

---

# II. Process Management

### 19. Process States

Create processes demonstrating different process states:

* Running
* Sleeping
* Stopped

Verify the state using Linux process-management commands and `/proc`.

**Directory:** `Question 19/`

---

### 20. Parent and Child Process IDs

Use `fork()` to create a child process and display:

* Parent process ID
* Child process ID

**Directory:** `Question 20/`

---

### 21. File Writing by Parent and Child Processes

Open a file, call `fork()`, and allow both the parent and child processes to write to the same file.

Examine the resulting file and observe the effect of process scheduling on the order of writes.

**Directory:** `Question 21/`

---

### 22. Creating a Zombie Process

Create a zombie process by allowing a child process to terminate while its parent remains alive without immediately calling `wait()` or `waitpid()`.

Verify the zombie state using Linux process-management commands.

**Directory:** `Question 22/`

---

### 23. Creating an Orphan Process

Create an orphan process by allowing the parent process to terminate while the child continues running.

Observe the child's parent process ID before and after reparenting.

**Directory:** `Question 23/`

---

### 24. Creating and Waiting for Child Processes

Create three child processes and use `waitpid()` to make the parent wait specifically for one selected child.

**Directory:** `Question 24/`

---

### 25. Executing an Executable Program

Demonstrate execution of another executable using an `exec` system call.

Also pass command-line input to the executable, for example:

```bash
./a.out name
```

**Directory:** `Question 25/`

---

### 26. Executing `ls -Rl` Using Various `exec` Calls

Execute:

```bash
ls -Rl
```

using:

* `execl()`
* `execlp()`
* `execle()`
* `execv()`
* `execvp()`

**Directory:** `Question 26/`

---

### 27. Getting Maximum and Minimum Real-Time Priority

Retrieve the minimum and maximum priority supported by a real-time scheduling policy using:

```c
sched_get_priority_min()
sched_get_priority_max()
```

**Directory:** `Question 27/`

---

### 28. Determining and Modifying Program Priority

Determine the priority/nice value of a running program and modify it using the Linux `nice` command.

The exercise also demonstrates inspection of scheduling information using commands such as:

```bash
ps
```

**Directory:** `Question 28/`

---

### 29. Getting and Modifying Scheduling Policy

Obtain the current scheduling policy and attempt to modify it using scheduling policies such as:

* `SCHED_OTHER`
* `SCHED_FIFO`
* `SCHED_RR`

using:

```c
sched_getscheduler()
sched_setscheduler()
```

**Directory:** `Question 29/`

---

### 30. Running a Script at a Specific Time Using a Daemon

Create a daemon process that waits until a specified time and executes a shell script/task.

The exercise demonstrates:

* Process daemonization
* `fork()`
* `setsid()`
* Background execution
* Time handling
* Script execution

**Directory:** `Question 30/`

---

# Concepts Covered

The exercises in this hands-on list provide practical exposure to the following Systems Software concepts:

### File Systems and File I/O

* Inodes
* Directory entries
* Hard links
* Symbolic links
* FIFOs
* File descriptors
* `open()`, `creat()`, `close()`
* `read()`, `write()`
* `lseek()`
* `stat()`, `lstat()`
* File metadata
* Sparse files

### File Descriptor Management

* `dup()`
* `dup2()`
* `fcntl()`
* File status flags
* File descriptor tables
* `/proc/<pid>/fd`

### Process Management

* `fork()`
* Process IDs
* Parent and child processes
* Process states
* `/proc`
* Zombie processes
* Orphan processes
* `wait()`
* `waitpid()`
* `exec` family of system calls

### Inter-Process Synchronization

* Advisory record locking
* Read locks
* Write locks
* Critical sections
* Race conditions
* Ticket reservation synchronization
* Record-level locking

### Process Scheduling

* Nice values
* Process priority
* Real-time priorities
* `SCHED_OTHER`
* `SCHED_FIFO`
* `SCHED_RR`
* `sched_getscheduler()`
* `sched_setscheduler()`

### Daemon Processes

* Daemonization
* Double-fork technique
* `setsid()`
* Session management
* Detaching from the terminal
* Scheduled background tasks

---

## Repository Structure

```text
Hands-on List 1/
│
├── README.md
│
├── Question 1/
├── Question 2/
├── Question 3/
├── Question 4/
├── Question 5/
├── Question 6/
├── Question 7/
├── Question 8/
├── Question 9/
├── Question 10/
├── Question 11/
├── Question 12/
├── Question 13/
├── Question 14/
├── Question 15/
├── Question 16/
├── Question 17/
├── Question 18/
├── Question 19/
├── Question 20/
├── Question 21/
├── Question 22/
├── Question 23/
├── Question 24/
├── Question 25/
├── Question 26/
├── Question 27/
├── Question 28/
├── Question 29/
└── Question 30/
```

---

## Course

**Course:** Systems Software
**Program:** M.Tech Computer Science and Engineering
**Institute:** IIIT Bangalore

---

> **Note:** The programs in this repository are intended for educational use and demonstrate Linux/POSIX system programming concepts.
