#!/bin/bash

# Create the target file.
echo "Hello from the original file." > original.txt

# Create a symbolic link to the target.
ln -s original.txt softlink.txt

# Create a hard link to the same inode.
ln original.txt hardlink.txt

# Create a FIFO (named pipe) for IPC.
mkfifo myfifo

# Display inode numbers and file types for verification.
ls -li original.txt softlink.txt hardlink.txt myfifo