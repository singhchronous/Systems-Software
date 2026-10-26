# Banking Management System
**CS 513 Software Systems — Mini Project (IIIT Bangalore)**

A concurrent client/server banking system written in C. It uses Linux system calls throughout: sockets, `fork()`, `fcntl()` record locking, System V shared memory, semaphores and message queues, signals, `select()`, `alarm()` and POSIX threads. All data is stored in binary record files. No database library is used.

| Deliverable (TA's list) | Where it is |
|---|---|
| **Blueprint (all diagrams in one file)** | **`docs/Blueprint.pdf`** - architecture, class diagram, transfer sequence (ACID), loan life-cycle |
| Blueprint / class diagram | `docs/diagrams/class_diagram.png` (source: `.mmd`) |
| Architecture diagram | `docs/diagrams/architecture.png` (source: `.dot`) |
| Extra diagrams | `docs/diagrams/transfer_sequence.png` (how ACID works) and `docs/diagrams/loan_states.png` |
| Working code for each module | `src/modules/customer/`, `src/modules/employee/`, `src/modules/manager/`, `src/modules/admin/` (each has a `*_server.c` and a `*_client.c`) |
| Concurrency and synchronization | `src/common/locking.c` and `db.c` (fcntl), `src/server/session.c` (shm + semaphore), `scripts/demo_race.sh` |
| ACID | `src/server/ledger.c`, `src/common/journal.c`, `tools/bank_verify.c`, `scripts/demo_crash.sh` |
| Documentation | this README, `docs/DESIGN.md`, `docs/TESTING.md` |
| Dependencies | `gcc`, `make` (and `pthread`, which is part of glibc). Nothing else is needed. |

---

## 1. Quick start (Ubuntu or any Linux, including your VMware VM)

```bash
sudo apt update && sudo apt install -y build-essential   # gcc + make (one-time)
unzip BankingManagementSystem.zip && cd BankingManagementSystem
make                 # builds bank_server, bank_client, bank_init, bank_verify, bank_stress
./bank_init --yes    # creates data/ with demo users (the server also does this on first start)
./bank_server        # terminal 1 - keep it running
./bank_client        # terminal 2, 3, 4 ... - one per user
```

Press **Ctrl+C** in the server terminal to shut it down cleanly. Connected clients are told the server is shutting down, sessions are closed, and the shared memory, semaphore and message queue are removed.

### Demo users

| Role | ID | Password | Name |
|---|---|---|---|
| Administrator | 9001 | admin123 | System Admin |
| Manager | 2001 | mgr123 | Rahul Mehta |
| Bank Employee | 2002 | emp123 | Priya Sharma |
| Bank Employee | 2003 | emp123 | Arjun Nair |
| Customer | 1001 – 1005 | cust123 | Shubham Singh (₹50,000), Ananya Iyer (₹25,000), Rohan Das (₹10,000), Meera Kapoor (₹75,000), Vikram Rao (₹5,000) |

The demo data also includes loan application **5001** (PENDING, customer 1002) and feedback **#1** (from customer 1003).

### Server options
```
./bank_server [-p port] [-t idle_seconds] [-d datadir] [--no-lock] [--race-delay us] [--crash-test]
  -p 8080          TCP port
  -t 300           idle sessions are logged out after this many seconds
  --no-lock        turns record locking OFF (only for the race-condition demo)
  --race-delay us  sleeps between reading and writing a balance, which widens the race window
  --crash-test     the serving process kills itself in the middle of every transfer (recovery demo)
./bank_client [server_ip] [port]          # default 127.0.0.1 8080
```

### Useful make targets
`make` · `make reset` (fresh demo data) · `make clean` · `make clean-ipc` (removes IPC objects left over after `kill -9`) · `make distclean`

---

## 2. Features

**Customer:** login (one session per user) · view balance · deposit · withdraw · transfer funds · apply for a loan · view my loan applications · change password · add feedback · view transaction history · logout · exit

**Bank Employee:** login · add new customer (opens the account, with an optional opening deposit) · modify customer details · view customer details · list customers · close a customer account · view assigned loan applications · process a loan application (produces a review sheet) · approve/reject a loan · view customer transactions (passbook) · change password · logout · exit

**Manager:** login · activate/deactivate customer accounts (a deactivated customer who is online is logged out immediately) · list customers · view loan applications · assign loan applications to employees (shows each employee's workload) · review customer feedback · change password · logout · exit

**Administrator:** login · add a new bank employee (or manager) · modify customer/employee details · view details · manage user roles (promote to manager, demote to employee, deactivate, re-activate) · **view live sessions** · **force-logout a user** · **view the audit log** · change password · logout · exit

### Features added on top of the specification (all based on course topics)
1. **Live session table in shared memory, protected by a semaphore.** It enforces one session per user and cannot get stuck after a crash. The admin can see who is online and force a logout with `kill(pid, SIGUSR1)`. Idle sessions are logged out automatically using `alarm()` and `SIGALRM`.
2. **Write-ahead journal with crash recovery.** If a server process is killed in the middle of a transfer, the operation is rolled back. Recovery runs at start-up, from a housekeeper thread, and in any process that finds an orphan journal.
3. **Concurrency stress tester (`bank_stress`).** It runs 20 or more forked processes (or threads) making simultaneous transfers and proves that total money is conserved. With `--no-lock` the same test shows lost updates.
4. **Audit daemon fed by a message queue.** A daemon process writes every important event to `logs/audit.log`. Ctrl+C performs a graceful shutdown.
5. **Consistency checker (`bank_verify`).** It checks every balance against its transaction history and checks that every transfer and loan balances.
6. Passwords are stored as salted SHA-256 hashes, password input is hidden, and money is stored in integer paise, so there is no floating-point rounding.

---

## 3. Project layout

```
include/            headers: records.h (file layouts), protocol.h (messages), db.h, journal.h ...
src/common/         db.c (record files), locking.c (fcntl), journal.c (WAL), net.c, password.c, util.c, seed.c
src/server/         server.c (Bank: accept/fork/signals/thread), auth.c (login), session.c (shm+sem),
                    audit.c (msg queue + daemon), ledger.c (ACID money ops), shared_ops.c
src/client/         client.c (login screen), ui.c (select()-based input)
src/modules/<role>/ <role>_server.c (business logic) + <role>_client.c (menu)   for customer, employee, manager, admin
tools/              bank_init.c, bank_verify.c, bank_stress.c
scripts/            run_all_tests.sh, demo_race.sh, demo_crash.sh, clean_ipc.sh
docs/               DESIGN.md, TESTING.md, diagrams/
data/  logs/        created at run time (not part of the source)
```

---

## 4. Running it on a Windows machine with a VMware Linux VM

1. **Copy the zip into the VM.** Either drag and drop it into the VM window (this needs VMware Tools / open-vm-tools; on Ubuntu run `sudo apt install open-vm-tools-desktop` and reboot), or use a shared folder (VM → Settings → Options → Shared Folders; it appears under `/mnt/hgfs/<name>`), or download it inside the VM.
2. Open a terminal (**Ctrl+Alt+T**) and run:
   ```bash
   sudo apt update && sudo apt install -y build-essential unzip
   cd ~ && unzip /path/to/BankingManagementSystem.zip && cd BankingManagementSystem
   make
   ```
   Do **not** build inside `/mnt/hgfs/...`, because shared folders do not support file locking properly. Copy the folder to your home directory first.
3. Open **several terminal tabs** (Ctrl+Shift+T): one for `./bank_server` and one per user running `./bank_client`. This shows the concurrency live. For example, log in as customer 1001 in one tab and try the same user in another; the second login is refused.
4. Run the full automated test suite: `./scripts/run_all_tests.sh` (about 30 seconds, 74 checks).
5. **Optional: connecting from Windows.** Find the VM's IP address with `ip addr`. In NAT mode Windows can usually reach the VM directly. Then run a client in a second VM or WSL with `./bank_client <vm-ip> 8080`.

See **docs/TESTING.md** for the step-by-step manual test plan and the two demonstration scripts.
