# Testing Guide

There are three levels of testing. Run them in this order inside your Linux VM.

| Level | What | Time |
|---|---|---|
| 1. Automated suite | `./scripts/run_all_tests.sh`: 74 checks covering every menu item of all 4 roles, sessions, concurrency, race demo, crash recovery, idle timeout | ~30 s |
| 2. Live demos | `./scripts/demo_race.sh` and `./scripts/demo_crash.sh`: they demonstrate the locking and the crash recovery | ~1 min each |
| 3. Manual walkthrough | Section C below: multiple terminals, exercising every menu by hand | 15–20 min |

The automated suite and the demos use their **own** port and data folder (`test_data/`, `demo_data/`), so they never change your real `data/`.

---

## A. Automated test suite
```bash
cd ~/BankingManagementSystem
make
./scripts/run_all_tests.sh
```
The run should end with `RESULT: 74 passed, 0 failed`. If anything fails, the failing check names are listed at the end. The server log for the run is in `/tmp/bms_test_server.log`.

## B. Demonstrations

**Race condition with and without locks**
```bash
./scripts/demo_race.sh
```
Run A (locks on) prints `[PASS] money conserved`. Run B (`--no-lock`) prints `[FAIL] total changed by -Rs. 5x,xxx -> RACE CONDITION`, and `bank_verify` lists accounts whose balance no longer matches their history. *Explain:* two processes read the same balance, both add to it, and the second write overwrites the first (a "lost update"). `fcntl(F_WRLCK)` makes the read-modify-write exclusive.

**Crash in the middle of a transfer (atomicity + durability)**
```bash
./scripts/demo_crash.sh          # press Enter at each pause
```
You will see the journal file left behind, `bank_verify` reporting ₹5,000 "missing" while the server is down, then the restart message `Crash recovery: rolled back 1 interrupted operation(s)`, and finally `CONSISTENT` with both balances unchanged.

**Stress test on your normal server** (data stays consistent and is only extended with test accounts):
```bash
./bank_stress -n 30 -m 50            # 30 processes
./bank_stress -n 30 -m 50 --threads  # 30 threads
./bank_verify
```
*Note:* `bank_stress` logs in as employee 2002 with password `emp123`. If you changed that password, add `-w <newpassword>`.

## C. Manual walkthrough (multiple terminals)

Start fresh: `make reset`, then `./bank_server -t 120` in terminal 1. Open terminals 2–5 with `./bank_client`.

| # | Steps | Expected |
|---|---|---|
| 1 | T2: Customer 1001 / cust123 → 1 View balance | Rs. 50,000.00 |
| 2 | T3: try Customer 1001 again | "already logged in from another session" |
| 3 | T2: 2 Deposit 1000 · 3 Withdraw 200 · 3 Withdraw 999999 | new balances; last one says "Insufficient balance" |
| 4 | T2: 4 Transfer to 1002, 2500, y · 4 Transfer to 9999 | success; then "does not exist. No money was moved." |
| 5 | T2: 5 Apply loan 100000, 12, "Car" · 6 View my loans | loan 5002 PENDING |
| 6 | T2: 8 Feedback (try `|`, quotes, `%s`) · 9 History | stored as typed; history shows DEPOSIT/WITHDRAW/TRANSFER_OUT |
| 7 | T3: Manager 2001 / mgr123 → 4 Assign 5001 → 2002 · 4 Assign 5002 → 2003 | "assigned to employee …" (shows workload) |
| 8 | T3: 5 Review feedback → y → #2, note | REVIEWED |
| 9 | T4: Employee 2002 / emp123 → 6 Assigned loans · 8 Approve 5001 | refused: "Process the application first" |
| 10 | T4: 7 Process 5001 · 8 → 5001 → 1 Approve | review sheet; "APPROVED … credited to 1002" |
| 11 | T4: 9 Passbook 1002 | TRANSFER_IN 2,500 and LOAN_CREDIT 2,00,000 |
| 12 | T4: 1 Add customer (Neha Gupta, 9811122233, neha@x.com, pw, 1000) | account 1006 created |
| 13 | T4: 2 Modify 1006 phone · 3 View 1006 · 5 Close 1006 | modify OK; close refused (balance ≠ 0) |
| 14 | T5: Admin 9001 / admin123 → 6 Live sessions | lists 1001, 2001, 2002, 9001 with pids |
| 15 | T5: 7 Force logout 1001 | T2 immediately shows "NOTICE FROM SERVER … forced logout" |
| 16 | T3: Manager → 1 Deactivate 1002 while 1002 is logged in somewhere | that client is kicked; a new login says DEACTIVATED |
| 17 | T5: 1 Add employee (role Employee) · 5 Promote him · log in as Manager | promoted user sees the MANAGER MENU |
| 18 | T5: 8 Audit log 30 | every action above, with pids |
| 19 | Leave a client idle for more than 120 s | "Session closed after 120 seconds of inactivity" |
| 20 | Press Ctrl+C in a client, then log in as that user again | works (the server noticed the disconnect) |
| 21 | Ctrl+C in T1 (server) | clients see "server is shutting down"; `ipcs` shows no leftover objects |
| 22 | `./bank_verify` | CONSISTENT |

Watch terminal 1 while doing this: every connection prints `[server] pid N serving …`. Each client is a separate process. You can confirm this with `ps -ef | grep bank_server` or `pstree -p`.

## D. Useful commands while testing
```bash
ps -ef | grep bank_            # main server, one child per client, audit daemon
pstree -p $(pgrep -o bank_server)
ipcs -m -s -q                  # shared memory, semaphore, message queue created by the server
tail -f logs/audit.log         # live audit log (second terminal)
ls -l data/ data/journal/      # record files; journal is empty except mid-operation
./bank_verify -v               # per-account check
make clean-ipc                 # only if the server was killed with kill -9
```

## E. If you find a bug
Write down: (1) the exact menu choices and inputs, (2) what you expected, (3) what happened, (4) the last 20 lines of the server terminal and `tail -20 logs/audit.log`, (5) the output of `./bank_verify`. Reproduce it once more after `make reset` to make sure it isn't caused by earlier test data.

## F. Troubleshooting
| Problem | Fix |
|---|---|
| `bind: Address already in use` | another server is still running: `pkill bank_server`, or use `-p 8081` (and `./bank_client 127.0.0.1 8081`) |
| `Cannot connect to the bank server` | start `./bank_server` first; check the port |
| `Failed to create IPC objects` | `make clean-ipc` and start again |
| `make: gcc: not found` | `sudo apt install build-essential` |
| Strange lock behaviour when the folder is under `/mnt/hgfs` | copy the project to `~/` (VMware shared folders don't support fcntl locks properly) |
| Forgot a changed password | `make reset` (stop the server first) |
