#!/bin/bash
# =============================================================================
#  DEMO 1 - Race conditions, with and without fcntl() record locking
#
#  Runs the same concurrent workload twice against a throw-away database
#  (demo_data/) on port 9292:
#     run A: normal server          -> total money unchanged   (locks work)
#     run B: server --no-lock       -> total money changes     (race condition)
#  Your real data/ folder is not touched.
# =============================================================================
cd "$(dirname "$0")/.." || exit 1
make -s || exit 1
PORT=9292
run() {
    rm -rf demo_data
    ./bank_server -p $PORT -d demo_data "$@" > /tmp/bms_demo_server.log 2>&1 &
    SPID=$!; disown $SPID
    sleep 1
    ./bank_stress -p $PORT -n 20 -m 30
    ./bank_verify -d demo_data | tail -6
    kill -INT $SPID; sleep 2
}
echo "################  RUN A: record locking ON (normal server)  ################"
run --race-delay 3000
echo
echo "################  RUN B: record locking OFF (./bank_server --no-lock)  ######"
run --no-lock --race-delay 3000
rm -rf demo_data
echo
echo "Same workload, same delay - only the fcntl() locks differ."
