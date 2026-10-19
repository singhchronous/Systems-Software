# Systems Software — Hands-on List 2

This repository contains the solutions for **Hands-on List 2** of the **Systems Software** course.

The exercises focus on Linux/POSIX systems programming, inter-process communication (IPC), pipes and FIFOs, System V IPC mechanisms, sockets, timers, process resource limits, CPU timing, multithreading, and signal handling.

## Environment

* **OS:** Linux / Ubuntu
* **Language:** C
* **Shell:** Bash
* **Compiler:** GCC
* **System interfaces:** POSIX / Linux system calls and library functions
* **Virtualization:** Ubuntu Linux VM (VMware Workstation)

---

# I. Pipes and Pipeline Processing

### 1. Pipe Creation and Communication

Create an unnamed pipe, transmit data through it, read the data at the other end, and display the received message.

**Directory:** `Question 1/`

---

### 2. Data Transmission from Parent to Child

Create a pipe and use `fork()` to establish communication from the parent process to the child process. The parent writes a message, and the child reads and displays it.

**Directory:** `Question 2/`

---

### 3. Two-Way Communication Using Pipes

Use two unnamed pipes to enable communication in both directions between a parent and child process. The parent sends a message to the child, and the child replies to the parent.

**Directory:** `Question 3/`

---

### 4. Executing `ls -l | wc` Using File Descriptor Redirection

Implement the equivalent of the shell pipeline:

```bash
ls -l | wc
```

Use two pipes and separate implementations based on:

* `dup()`
* `dup2()`
* `fcntl()`

**Directory:** `Question 4/`

---

### 5. Counting Directories Using a Pipeline

Implement the equivalent of:

```bash
ls -l | grep ^d | wc
```

Connect three commands using pipes and redirect standard input/output using `dup2()`.

**Directory:** `Question 5/`

---

# II. FIFO (Named Pipe) Operations

### 6. FIFO File Creation

Create a FIFO using different approaches:

* Shell command using `mknod`
* Shell command using `mkfifo`
* Compare the commands using `strace`
* C program using `mknod()`
* C program using `mkfifo()`

**Directory:** `Question 6/`

---

### 7. One-Way Communication Using a FIFO

Create separate writer and reader programs that communicate through a named pipe. The writer sends a message, and the reader receives and displays it.

**Directory:** `Question 7/`

---

### 8. Two-Way Communication Using FIFOs

Use two named pipes to enable bidirectional communication between a client and a server. The client sends a request, and the server sends a response.

**Directory:** `Question 8/`

---

### 9. Waiting for FIFO Data Using `select()`

Wait up to 10 seconds for data to become available on a FIFO using `select()`. Report whether data was received before the timeout.

**Directory:** `Question 9/`

---

# III. System V Message Queues

### 10. Process File Limit and Pipe Size

Display the process's open-file resource limits using `getrlimit()` and retrieve the capacity of a pipe using `fcntl()` and `F_GETPIPE_SZ`.

**Directory:** `Question 10/`

---

### 11. Message Queue Creation

Create a System V message queue using:

* `ftok()`
* `msgget()`

Display the key and message queue identifier.

**Directory:** `Question 11/`

---

### 12. Retrieving Message Queue Information

Use `msgctl()` with `IPC_STAT` and inspect the queue metadata in `struct msqid_ds` and `struct ipc_perm`, including:

* Queue permissions
* Owner and creator UID/GID
* Last send and receive times
* Last sender and receiver PIDs
* Last change time
* Current message bytes and message count
* Maximum queue capacity

**Directory:** `Question 12/`

---

### 13. Sending Messages to a Message Queue

Send a message to a System V message queue using `msgsnd()`. The message structure contains a message type and a text payload.

**Directory:** `Question 13/`

---

### 14. Receiving Messages from a Message Queue

Implement message reception using `msgrcv()` in two modes:

* Blocking receive
* Non-blocking receive using `IPC_NOWAIT`

**Directory:** `Question 14/`

---

### 15. Changing Message Queue Permissions

Use `msgctl()` with `IPC_STAT` and `IPC_SET` to inspect and change the permissions of an existing message queue.

**Directory:** `Question 15/`

---

### 16. Removing a Message Queue

Locate an existing System V message queue and remove it using `msgctl()` with `IPC_RMID`.

**Directory:** `Question 16/`

---

# IV. Shared Memory and Semaphores

### 17. Shared Memory Operations

Create and attach a System V shared memory segment. Write data to it, detach and reattach it in read-only mode, demonstrate that writing through the read-only mapping causes a memory-access fault, and remove the segment.

**Directory:** `Question 17/`

---

### 18. Semaphore Creation and Initialization

Create System V semaphore sets and initialize a semaphore to specified values:

* Initialize the semaphore to `1`
* Initialize the semaphore to `5`

Use `semget()` and `semctl()`.

**Directory:** `Question 18/`

---

### 19. Semaphore Implementation

Explore semaphore-based synchronization through four exercises:

* **(a) Ticket generation:** Protect a shared ticket counter using a semaphore.
* **(b) Shared data structure:** Protect updates to shared data using a semaphore.
* **(c) Counting semaphore:** Limit the number of processes concurrently accessing a simulated resource.
* **(d) Semaphore removal:** Remove a semaphore set using its identifier.

The implementations use System V semaphores and shared memory where required.

**Directory:** `Question 19/`

---

### 20. Intentionally Inducing Deadlock Using Semaphores

Create two semaphores and two processes. Each process acquires one semaphore and then attempts to acquire the semaphore held by the other process, demonstrating deadlock. Remove the semaphore set to release the blocked operations and clean up.

**Directory:** `Question 20/`

---

# V. Socket Programming and Concurrent Servers

### 21. Inter-Machine Communication Using Sockets

Create a TCP server and client using IPv4 sockets. The server accepts a connection and exchanges data with the client.

The client can use `127.0.0.1` for local testing or the server's reachable IP address for communication between machines.

**Directory:** `Question 21/`

---

### 22. Concurrent Server Creation Using `fork()` and `pthread_create()`

Implement two concurrent TCP server variants:

* **(a) Process-per-client server:** Create a child process for each client using `fork()`.
* **(b) Thread-per-client server:** Create a separate POSIX thread for each client using `pthread_create()`.

**Directory:** `Question 22/`

---

# VI. Interval Timers and Resource Limits

### 23. Interval Timer Programming

Use interval timers and signal handlers to demonstrate:

* **(a) `ITIMER_REAL`** — generates `SIGALRM` based on elapsed real time.
* **(b) `ITIMER_VIRTUAL`** — generates `SIGVTALRM` based on user CPU time.
* **(c) `ITIMER_PROF`** — generates `SIGPROF` based on user plus system CPU time.

Use `setitimer()` and `sigaction()` or signal-handling mechanisms as required by the implementation.

**Directory:** `Question 23/`

---

### 24. System Resource Limits Using `getrlimit()`

Retrieve and display the soft and hard limits for supported process resources using `getrlimit()`.

**Directory:** `Question 24/`

---

### 25. Setting System Resource Limits Using `setrlimit()`

Modify the process's soft open-file limit using `setrlimit()`, preserve the hard limit, and verify the resulting values using `getrlimit()`.

**Directory:** `Question 25/`

---

### 26. Measuring Execution Time of `getppid()` Using the TSC

Measure the time-stamp counter before and after 100 calls to `getppid()`. Report the total and average TSC ticks.

This exercise uses architecture-specific x86/x86-64 instructions and the result is expressed in TSC ticks, not necessarily nanoseconds or core clock cycles.

**Directory:** `Question 26/`

---

### 27. System Limitation Exploration

Explore system limits and hardware-related values using `sysconf()` and `getrlimit()`, including:

* Maximum argument/environment size for `exec`
* Maximum child processes per user, where supported
* Clock ticks per second
* Open-file limits
* Memory page size
* Total physical pages
* Available physical pages

**Directory:** `Question 27/`

---

# VII. Multithreading

### 28. Multithreading Exploration

Create three POSIX threads using `pthread_create()`. Print thread identifiers from the main thread and worker threads, and wait for the workers using `pthread_join()`.

**Directory:** `Question 28/`

---

# VIII. Signal Handling and Process Control

### 29. Signal Handling Using `signal()`

Write separate programs to handle the following signals:

* **(a) `SIGSEGV`**
* **(b) `SIGINT`**
* **(c) `SIGFPE`**
* **(d) `SIGALRM` using `alarm()`**
* **(e) `SIGALRM` using `setitimer()`**
* **(f) `SIGVTALRM` using `setitimer()`**
* **(g) `SIGPROF` using `setitimer()`**

**Directory:** `Question 29/`

---

### 30. Ignoring and Resetting Signals Using `signal()`

Write a program that ignores `SIGINT` using `SIG_IGN` and subsequently restores its default disposition using `SIG_DFL`.

**Directory:** `Question 30/`

---

### 31. Signal Handling Using `sigaction()`

Write separate programs using `sigaction()` to catch:

* **(a) `SIGSEGV`**
* **(b) `SIGINT`**
* **(c) `SIGFPE`**

**Directory:** `Question 31/`

---

### 32. Ignoring and Resetting Signals Using `sigaction()`

Write a program that ignores `SIGINT` using `sigaction()` and then restores its default disposition.

**Directory:** `Question 32/`

---

### 33. Creating an Orphan Process

Create a child process that sends `SIGKILL` to its parent using `kill()`. Allow the child to continue executing after the parent terminates and observe its new parent process ID.

**Directory:** `Question 33/`

---

### 34. `SIGSTOP` Handling

Create two programs:

* **Receiver:** Attempts to register a handler for `SIGSTOP` and waits for a signal.
* **Sender:** Sends `SIGSTOP` to the receiver using `kill()`.

Determine whether the receiver catches the signal and inspect its process state. `SIGSTOP` cannot be caught, blocked, or ignored.

**Directory:** `Question 34/`

---

# Concepts Covered

The exercises in this hands-on list provide practical exposure to the following Systems Software concepts.

### Pipes and FIFO Communication

* Unnamed pipes
* Named pipes (FIFOs)
* Parent-child communication
* Bidirectional IPC
* Pipeline construction
* File descriptor redirection
* `pipe()`, `mkfifo()`, `mknod()`
* `dup()`, `dup2()`, `fcntl()`
* `select()`

### System V IPC

* Message queues
* `ftok()`, `msgget()`, `msgsnd()`, `msgrcv()`
* Queue metadata using `msgctl()` and `msqid_ds`
* Queue permissions and removal
* Shared memory using `shmget()`, `shmat()`, `shmdt()`, `shmctl()`
* System V semaphores
* `semget()`, `semctl()`, `semop()`
* Mutual exclusion
* Counting semaphores
* Critical sections
* Deadlock conditions and cleanup

### Socket Programming

* IPv4 TCP sockets
* Client-server communication
* `socket()`, `bind()`, `listen()`, `accept()`, `connect()`
* `send()`, `recv()`
* Process-per-client servers
* Thread-per-client servers
* `fork()` and POSIX threads

### Timers and Resource Limits

* `alarm()`
* `setitimer()`
* `ITIMER_REAL`
* `ITIMER_VIRTUAL`
* `ITIMER_PROF`
* `getrlimit()`
* `setrlimit()`
* `sysconf()`
* File descriptor limits
* Pipe capacity
* Physical memory page information

### CPU Timing and Multithreading

* Time Stamp Counter (TSC)
* x86/x86-64 inline assembly
* Measuring system-call execution
* `pthread_create()`
* `pthread_self()`
* `pthread_join()`
* Thread identifiers
* Concurrent execution

### Signal Handling and Process Control

* `signal()`
* `sigaction()`
* `SIGSEGV`
* `SIGINT`
* `SIGFPE`
* `SIGALRM`
* `SIGVTALRM`
* `SIGPROF`
* `SIG_IGN` and `SIG_DFL`
* `raise()`
* `kill()`
* `SIGSTOP`, `SIGCONT`, and `SIGKILL`
* Orphan processes
* Signal dispositions
* Signal handlers and async-signal safety

---

## Repository Structure

```text
Hands-on List 2/
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
├── Question 30/
├── Question 31/
├── Question 32/
├── Question 33/
└── Question 34/
```

---

## Course

**Course:** Systems Software

**Program:** M.Tech Computer Science and Engineering

**Institute:** IIIT Bangalore

---

> **Note:** The programs in this repository are intended for educational use and demonstrate Linux/POSIX system programming concepts. Some exercises use Linux-specific interfaces or architecture-specific instructions. Test programs carefully in a controlled environment, especially those that deliberately generate faults, consume CPU time, alter IPC objects, or send process-control signals.
