#!/bin/bash

# Create a regular file
echo "Hello from original file" > original.txt

# a. Create soft link
ln -s original.txt softlink.txt

# b. Create hard link
ln original.txt hardlink.txt

# c. Create FIFO
mkfifo myfifo

# Display the created files
ls -li original.txt softlink.txt hardlink.txt myfifo