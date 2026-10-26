#!/bin/bash
# =============================================================================
#  Automated test-suite for the Banking Management System
#
#  Usage:   ./scripts/run_all_tests.sh          (from the project folder)
#
#  It builds the project, starts its OWN server on port 9191 with a separate
#  data folder (test_data/), so your normal data/ folder is never touched, and
#  then drives the real client exactly like a user would (input is piped in).
#  Every check prints PASS or FAIL; a summary is printed at the end.
# =============================================================================
cd "$(dirname "$0")/.." || exit 1

PORT=9191
DATA=test_data
PASS=0
FAIL=0
FAILED_NAMES=()
GREEN=$'\033[32m'; RED=$'\033[31m'; BOLD=$'\033[1m'; RESET=$'\033[0m'
SERVER_PID=""

section() { echo; echo "${BOLD}== $1 ==${RESET}"; }

ok()   { PASS=$((PASS+1)); echo "  ${GREEN}PASS${RESET}  $1"; }
bad()  { FAIL=$((FAIL+1)); FAILED_NAMES+=("$1"); echo "  ${RED}FAIL${RESET}  $1"; [ -n "$2" ] && echo "        $2"; }

# run_client "<input lines>"  -> prints client output
run_client() { printf "$1" | ./bank_client 127.0.0.1 $PORT 2>&1; }

# check "<name>" "<input>" "<regex expected in output>"
check() {
    local out
    out=$(run_client "$2")
    if echo "$out" | grep -qE -- "$3"; then ok "$1"; else bad "$1" "expected: $3"; fi
}

start_server() {   # start_server [extra options]
    ./bank_server -p $PORT -d $DATA "$@" > /tmp/bms_test_server.log 2>&1 &
    SERVER_PID=$!; disown "$SERVER_PID" 2>/dev/null
    for _ in $(seq 1 30); do
        sleep 0.2
        printf '5\n' | ./bank_client 127.0.0.1 $PORT >/dev/null 2>&1 && return 0
    done
    echo "server did not start:"; cat /tmp/bms_test_server.log; exit 1
}

stop_server() {
    [ -n "$SERVER_PID" ] && kill -INT "$SERVER_PID" 2>/dev/null
    for _ in $(seq 1 40); do kill -0 "$SERVER_PID" 2>/dev/null || break; sleep 0.2; done
    SERVER_PID=""
}

# The server derives its System V IPC keys with ftok(data_dir, 'S'/'M'/'Q'):
#   key = (proj_id << 24) | ((st_dev & 0xff) << 16) | (st_ino & 0xffff)
# so we can compute exactly which keys belong to OUR test server and ignore
# IPC objects owned by other programs (the Ubuntu desktop uses shared memory too).
our_ipc_count() {
    local dev ino n=0
    read -r dev ino < <(stat -c '%d %i' "$DATA")
    for proj in 0x53 0x4d 0x51; do        # 'S' shm, 'M' semaphore, 'Q' message queue
        local key
        key=$(printf '0x%08x' $(( (proj << 24) | ((dev & 0xff) << 16) | (ino & 0xffff) )))
        ipcs -m -s -q | grep -qi "^$key " && n=$((n+1))
    done
    echo $n
}

cleanup() { [ -n "$SERVER_PID" ] && kill -9 "$SERVER_PID" 2>/dev/null; pkill -9 -f "bank_server -p $PORT" 2>/dev/null; }
trap cleanup EXIT

# ----------------------------------------------------------------------------
section "Build"
if make -s 2>&1 | grep -E "error|warning"; then bad "compiles without errors/warnings"; else ok "compiles without errors/warnings"; fi

rm -rf $DATA
start_server -t 60
ok "server started on port $PORT (pid $SERVER_PID)"

# ----------------------------------------------------------------------------
section "Customer module"
check "login + view balance"                  '1\n1001\ncust123\n1\n11\n'                 "Rs\. 50,000\.00"
if command -v python3 >/dev/null; then
    if python3 scripts/tty_login_test.py $PORT; then ok "login typed in a real terminal (hidden password, typed ahead)"; else bad "login typed in a real terminal (hidden password, typed ahead)"; fi
fi
check "wrong password is rejected"            '1\n1001\nnope\n5\n'                        "Invalid ID or password"
check "deposit Rs. 1500.50"                   '1\n1001\ncust123\n2\n1500.50\n11\n'       "New balance: Rs\. 51,500\.50"
check "withdraw Rs. 500"                      '1\n1001\ncust123\n3\n500\n11\n'           "New balance: Rs\. 51,000\.50"
check "withdraw more than balance refused"    '1\n1001\ncust123\n3\n9999999\n11\n'       "Insufficient balance"
check "invalid amount (3 decimals) refused"   '1\n1001\ncust123\n2\n10.123\n10\n11\n'    "at most 2 decimals"
check "transfer Rs. 1000 to 1002"             '1\n1001\ncust123\n4\n1002\n1000\ny\n11\n' "Transferred Rs\. 1,000\.00 to account 1002"
check "transfer to non-existent account"      '1\n1001\ncust123\n4\n9999\n10\ny\n11\n'   "does not exist. No money was moved"
check "transfer to own account refused"       '1\n1001\ncust123\n4\n1001\n10\ny\n11\n'   "own account"
check "receiver got the money"                '1\n1002\ncust123\n1\n11\n'                 "Rs\. 26,000\.00"
check "apply for a loan"                      '1\n1001\ncust123\n5\n75000\n24\nNew car\n11\n' "Loan application 5002 submitted"
check "view my loan applications"             '1\n1001\ncust123\n6\n11\n'                 "5002 .*PENDING"
check "add feedback (with | and quotes)"      '1\n1001\ncust123\n8\nNice app | add "UPI" please\n11\n' "Feedback #2 has been recorded"
check "transaction history"                   '1\n1001\ncust123\n9\n11\n'                 "TRANSFER_OUT .*1,000\.00"
check "change password (wrong old refused)"   '1\n1001\ncust123\n7\nbad\nnew1234\nnew1234\n11\n' "Current password is incorrect"
check "change password"                       '1\n1001\ncust123\n7\ncust123\nnew1234\nnew1234\n11\n' "Password changed successfully"
check "login with the new password"           '1\n1001\nnew1234\n1\n11\n'                 "Login successful"
check "logout returns to login screen"        '1\n1001\nnew1234\n10\n5\n'                 "Logged out successfully"

# ----------------------------------------------------------------------------
section "One session per user"
( (printf '1\n1003\ncust123\n'; sleep 4) | ./bank_client 127.0.0.1 $PORT > /dev/null 2>&1 & )
sleep 1
check "second login of the same user is refused" '1\n1003\ncust123\n5\n'                 "already logged in"
sleep 4
check "after the first session ends, login works again" '1\n1003\ncust123\n11\n'          "Login successful"
check "login with the wrong role is refused"  '2\n2001\nmgr123\n5\n'                      "registered as Manager"

# ----------------------------------------------------------------------------
section "Manager module"
check "list loan applications"                '3\n2001\nmgr123\n3\n1\n8\n'                "5001 .*PENDING"
check "assign loan 5001 to employee 2002"     '3\n2001\nmgr123\n4\n5001\n2002\n8\n'       "Loan 5001 assigned to employee 2002"
check "cannot assign a loan to a manager"     '3\n2001\nmgr123\n4\n5002\n2001\n8\n'       "not a bank employee"
check "assign loan 5002 to employee 2003"     '3\n2001\nmgr123\n4\n5002\n2003\n8\n'       "Loan 5002 assigned to employee 2003"
check "review feedback"                       '3\n2001\nmgr123\n5\n1\ny\n2\nForwarded to IT\n8\n' "Feedback #2 marked as REVIEWED"
check "deactivate customer 1005"              '3\n2001\nmgr123\n1\n1005\n2\n8\n'          "1005 is now INACTIVE"
check "deactivated customer cannot log in"    '1\n1005\ncust123\n5\n'                     "DEACTIVATED"
check "re-activate customer 1005"             '3\n2001\nmgr123\n1\n1005\n1\n8\n'          "1005 is now ACTIVE"

# ----------------------------------------------------------------------------
section "Employee module"
check "view assigned loans"                   '2\n2002\nemp123\n6\n12\n'                  "5001 .*ASSIGNED"
check "approve before processing is refused"  '2\n2002\nemp123\n8\n5001\n1\nok\n12\n'     "Process the application first"
check "cannot process someone else's loan"    '2\n2002\nemp123\n7\n5002\n12\n'            "not assigned to you"
check "process loan 5001 (review sheet)"      '2\n2002\nemp123\n7\n5001\n12\n'            "LOAN REVIEW SHEET"
check "approve loan 5001"                     '2\n2002\nemp123\n8\n5001\n1\nGood record\n12\n' "APPROVED. Rs\. 2,00,000\.00 has been credited"
check "loan cannot be approved twice"         '2\n2002\nemp123\n8\n5001\n1\nagain\n12\n'  "already APPROVED"
check "customer received the loan amount"     '1\n1002\ncust123\n1\n11\n'                 "Rs\. 2,26,000\.00"
check "reject needs a reason / reject works"  '2\n2003\nemp123\n7\n5002\n8\n5002\n2\nIncome proof missing\n12\n' "5002 REJECTED"
check "add new customer"                      '2\n2002\nemp123\n1\nNeha\nGupta\n9811122233\nneha@example.com\nneha123\nneha123\n2500\n12\n' "Account number : 1006"
check "invalid customer data is rejected"     '2\n2002\nemp123\n1\nN3ha\nGupta\n98\nbad\nx\nx\n0\n12\n' "Invalid first/last name"
check "modify customer phone"                 '2\n2002\nemp123\n2\n1006\n3\n9800000000\n12\n' "phone changed"
check "view customer details"                 '2\n2002\nemp123\n3\n1006\n12\n'            "9800000000"
check "passbook of customer 1002"             '2\n2002\nemp123\n9\n1002\n12\n'            "LOAN_CREDIT"
check "closing an account with balance refused" '2\n2002\nemp123\n5\n1006\ny\n12\n'        "Balance must be zero"
check "new customer can log in"               '1\n1006\nneha123\n3\n2500\n11\n'           "New balance: Rs\. 0\.00"
check "close account with zero balance"       '2\n2002\nemp123\n5\n1006\ny\n12\n'         "1006 has been CLOSED"
check "closed account cannot log in"          '1\n1006\nneha123\n5\n'                     "CLOSED"
check "cannot transfer into a closed account" '1\n1002\ncust123\n4\n1006\n10\ny\n11\n'    "is CLOSED"

# ----------------------------------------------------------------------------
section "Administrator module"
check "add new employee"                      '4\n9001\nadmin123\n1\nKiran\nPatel\n9900011111\nkiran@bank.in\n1\nkiran123\nkiran123\n11\n' "Employee ID: 2004"
check "new employee can log in"               '2\n2004\nkiran123\n12\n'                   "Login successful"
check "promote employee to manager"           '4\n9001\nadmin123\n5\n1\n2004\n11\n'       "PROMOTED from Employee to Manager"
check "promoted user logs in as manager"      '3\n2004\nkiran123\n8\n'                    "MANAGER MENU"
check "demote manager to employee"            '4\n9001\nadmin123\n5\n2\n2004\n11\n'       "DEMOTED from Manager to Employee"
check "deactivate employee"                   '4\n9001\nadmin123\n5\n3\n2004\n11\n'       "2004 is now INACTIVE"
check "modify employee email"                 '4\n9001\nadmin123\n3\n2003\n4\narjun.n@bank.in\n11\n' "Employee 2003 updated"
check "modify customer name (admin)"          '4\n9001\nadmin123\n2\n1004\n2\nKapoor-Shah\n11\n' "last name changed"
check "view audit log"                        '4\n9001\nadmin123\n8\n20\n11\n'            "audit daemon"
( (printf '1\n1004\ncust123\n'; sleep 5) | ./bank_client 127.0.0.1 $PORT > /tmp/bms_forced.out 2>&1 & )
sleep 1
check "live sessions show an online user"     '4\n9001\nadmin123\n6\n11\n'                "1004 +Customer"
check "force logout an online user"           '4\n9001\nadmin123\n7\n1004\n11\n'          "forced-logout signal"
sleep 1
if grep -q "forced logout" /tmp/bms_forced.out; then ok "the kicked client is told immediately"; else bad "the kicked client is told immediately"; fi

# ----------------------------------------------------------------------------
section "Concurrency & ACID"
if ./bank_verify -d $DATA > /tmp/bms_verify.out; then ok "bank_verify: database consistent"; else bad "bank_verify: database consistent" "$(grep FAIL /tmp/bms_verify.out | head -3)"; fi
if ./bank_stress -p $PORT -n 20 -m 40 > /tmp/bms_stress.out 2>&1; then
    ok "stress test: 20 concurrent clients x 40 transfers, money conserved"
else
    bad "stress test" "$(grep -E 'FAIL' /tmp/bms_stress.out)"
fi
if ./bank_verify -d $DATA > /tmp/bms_verify.out; then ok "bank_verify after stress test"; else bad "bank_verify after stress test"; fi
n_before=$(our_ipc_count)
stop_server
n_after=$(our_ipc_count)
if [ "$n_before" = "3" ] && [ "$n_after" = "0" ]; then
    ok "graceful shutdown removed this server's shared memory, semaphore and message queue"
else
    bad "graceful shutdown removed IPC objects" "while running: $n_before of 3 found, after Ctrl+C: $n_after left (expected 3 -> 0)"
fi

section "Race-condition demo (locking DISABLED on purpose)"
rm -rf ${DATA}_race
DATA_SAVE=$DATA; DATA=${DATA}_race
start_server --no-lock --race-delay 3000
if ./bank_stress -p $PORT -n 20 -m 30 --expect-race > /tmp/bms_race.out 2>&1; then
    bad "without locks the race SHOULD appear" "(race not observed this time - try again)"
else
    ok "without locks money is lost/created: $(grep -o 'total changed by [^ ]* [^ ]*' /tmp/bms_race.out)"
fi
stop_server
DATA=$DATA_SAVE

section "Crash recovery (write-ahead journal)"
start_server --crash-test
run_client '1\n1004\ncust123\n4\n1002\n777\ny\n' > /dev/null
sleep 3
if grep -q "rolled back" /tmp/bms_test_server.log; then ok "server child killed mid-transfer -> housekeeper rolled it back"; else bad "housekeeper rollback"; fi
check "sender balance unchanged after the crash" '1\n1004\ncust123\n1\n11\n' "Rs\. 75,000\.00"
if ./bank_verify -d $DATA > /dev/null; then ok "database consistent after crash recovery"; else bad "database consistent after crash recovery"; fi
# kill -9 the WHOLE server in the middle of a transfer, then restart
run_client '1\n1004\ncust123\n4\n1002\n100\ny\n' > /dev/null
kill -9 "$SERVER_PID" 2>/dev/null; pkill -9 -f "bank_server -p $PORT" 2>/dev/null; SERVER_PID=""
sleep 0.5
if ls $DATA/journal/jnl_* >/dev/null 2>&1; then ok "kill -9 left a journal behind (half-done transfer)"; else bad "journal left behind after kill -9" "(the housekeeper may have been faster - fine)"; fi
start_server -t 60
if grep -q "Crash recovery: rolled back" /tmp/bms_test_server.log || ! ls $DATA/journal/jnl_* >/dev/null 2>&1; then ok "restart: interrupted transfer rolled back at start-up"; else bad "start-up recovery"; fi
if ./bank_verify -d $DATA > /dev/null; then ok "database consistent after restart"; else bad "database consistent after restart"; fi
stop_server

section "Idle timeout"
start_server -t 5
( (printf '1\n1001\nnew1234\n'; sleep 8) | ./bank_client 127.0.0.1 $PORT > /tmp/bms_idle.out 2>&1 & )
sleep 8
if grep -q "inactivity" /tmp/bms_idle.out; then ok "idle session closed after 5 s (alarm + SIGALRM)"; else bad "idle timeout"; fi
stop_server

# ----------------------------------------------------------------------------
echo
echo "${BOLD}==============================================${RESET}"
echo "${BOLD} RESULT: $PASS passed, $FAIL failed${RESET}"
for n in "${FAILED_NAMES[@]}"; do echo "   ${RED}x${RESET} $n"; done
echo "${BOLD}==============================================${RESET}"
rm -rf ${DATA}_race
[ $FAIL -eq 0 ]
