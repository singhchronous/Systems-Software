#!/bin/bash

# Refuse to overwrite an existing filesystem entry.
if [ -e myfifo_b ] || [ -L myfifo_b ]; then
    echo "myfifo_b already exists"
    exit 1
fi

# Create a named pipe using the dedicated mkfifo command.
mkfifo myfifo_b || exit 1

# Verify that the resulting entry is a FIFO.
if [ -p myfifo_b ]; then
    echo "FIFO created successfully: myfifo_b"
else
    echo "FIFO verification failed"
    exit 1
fi

# Test:
# chmod +x question_6_b.sh
# ./question_6_b.sh
# Expected output:
# FIFO created successfully: myfifo_b
