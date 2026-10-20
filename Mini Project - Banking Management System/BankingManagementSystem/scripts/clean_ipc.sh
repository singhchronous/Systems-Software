#!/bin/bash
# Removes System V IPC objects (shared memory, semaphores, message queues)
# owned by you that were left behind if the server was killed with kill -9.
# Normally NOT needed: Ctrl+C shuts the server down cleanly and removes them.
echo "Before:"; ipcs -m -s -q | grep -E "^0x|key" 
for id in $(ipcs -m | awk -v u="$(whoami)" '$3==u {print $2}'); do ipcrm -m "$id"; done
for id in $(ipcs -s | awk -v u="$(whoami)" '$3==u {print $2}'); do ipcrm -s "$id"; done
for id in $(ipcs -q | awk -v u="$(whoami)" '$3==u {print $2}'); do ipcrm -q "$id"; done
echo "Removed leftover IPC objects owned by $(whoami)."
