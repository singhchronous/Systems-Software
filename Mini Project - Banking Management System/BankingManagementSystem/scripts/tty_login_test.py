#!/usr/bin/env python3
"""Logs in through a REAL pseudo-terminal (like a person typing), with all keys
typed ahead at once. Catches bugs in the hidden-password code that piped input
cannot reach. Usage: tty_login_test.py <port>"""
import os, pty, select, sys, time
port = sys.argv[1] if len(sys.argv) > 1 else "8080"
pid, fd = pty.fork()
if pid == 0:
    os.execv("./bank_client", ["./bank_client", "127.0.0.1", port])
time.sleep(0.5)
os.write(fd, b"4\r9001\radmin123\r11\r")
out, end = b"", time.time() + 4
while time.time() < end:
    r, _, _ = select.select([fd], [], [], 0.1)
    if r:
        try:
            out += os.read(fd, 4096)
        except OSError:
            break
sys.exit(0 if b"Login successful" in out else 1)
