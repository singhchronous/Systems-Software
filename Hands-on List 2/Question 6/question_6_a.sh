#!/bin/bash

# Refuse to overwrite an existing filesystem entry.
if [ -e myfifo_a ] || [ -L myfifo_a ]; then
    echo "myfifo_a already exists"
    exit 1
fi

# Create a named pipe using the mknod command.
mknod myfifo_a p || exit 1

# Verify that the resulting entry is a FIFO.
if [ -p myfifo_a ]; then
    echo "FIFO created successfully: myfifo_a"
else
    echo "FIFO verification failed"
    exit 1
fi

# Test:
# chmod +x question_6_a.sh
# ./question_6_a.sh
# Expected output:
# FIFO created successfully: myfifo_a
