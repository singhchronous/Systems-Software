#!/bin/bash
# =============================================================================
#  DEMO 2 - Atomicity & durability: crash in the MIDDLE of a transfer
#
#  1. starts a server with --crash-test (each transfer kills its server
#     process right after the DEBIT is written, before the CREDIT)
#  2. customer 1001 transfers Rs. 5,000 to 1002 -> the client loses its connection
#  3. the WHOLE server is killed with kill -9 before it can clean up
#  4. bank_verify shows the half-done state (money missing) + the journal file
#  5. the server is restarted -> start-up recovery rolls the transfer back
#  6. bank_verify shows everything consistent again, balances unchanged
#  Uses a throw-away database (demo_data/) on port 9393.
# =============================================================================
cd "$(dirname "$0")/.." || exit 1
make -s || exit 1
PORT=9393
rm -rf demo_data
pause() { echo; read -r -p "---- press Enter to continue ----" _ ; echo; }

echo "Step 1: start server with --crash-test"
./bank_server -p $PORT -d demo_data --crash-test > /tmp/bms_crash_server.log 2>&1 &
SPID=$!; disown $SPID
sleep 1
echo "Step 2: customer 1001 transfers Rs. 5,000 to 1002"
printf '1\n1001\ncust123\n4\n1002\n5000\ny\n' | ./bank_client 127.0.0.1 $PORT 2>&1 | grep -E "ERROR|Lost"
echo "Step 3: kill -9 the whole server (no chance to clean up)"
pkill -9 -f "bank_server -p $PORT"
sleep 0.5
echo "Journal files left on disk:"; ls -l demo_data/journal/
pause
echo "Step 4: consistency check while the server is DOWN"
./bank_verify -d demo_data
pause
echo "Step 5: restart the server (normal mode)"
./bank_server -p $PORT -d demo_data > /tmp/bms_crash_server2.log 2>&1 &
SPID=$!; disown $SPID
sleep 1.5
head -2 /tmp/bms_crash_server2.log
echo
echo "Step 6: consistency check after recovery"
./bank_verify -d demo_data -v
printf '1\n1001\ncust123\n1\n11\n' | ./bank_client 127.0.0.1 $PORT 2>&1 | grep "Available"
printf '1\n1002\ncust123\n1\n11\n' | ./bank_client 127.0.0.1 $PORT 2>&1 | grep "Available"
kill -INT $SPID; sleep 2
rm -rf demo_data
