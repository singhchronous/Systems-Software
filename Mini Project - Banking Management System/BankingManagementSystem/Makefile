# Banking Management System - CS 513 Software Systems mini project
#
#   make            build everything (bank_server, bank_client, tools)
#   make reset      wipe the database and create fresh demo data
#   make clean      remove binaries and object files
#   make clean-ipc  remove leftover shared memory / semaphore / message queue
#   make distclean  clean + delete data/ and logs/

CC      := gcc
CFLAGS  := -Wall -Wextra -Wno-format-truncation -O2 -g -Iinclude -D_GNU_SOURCE
LDFLAGS := -pthread

COMMON  := $(wildcard src/common/*.c)
SERVER  := $(wildcard src/server/*.c) \
           src/modules/customer/customer_server.c \
           src/modules/employee/employee_server.c \
           src/modules/manager/manager_server.c \
           src/modules/admin/admin_server.c
CLIENT  := $(wildcard src/client/*.c) \
           src/modules/customer/customer_client.c \
           src/modules/employee/employee_client.c \
           src/modules/manager/manager_client.c \
           src/modules/admin/admin_client.c

OBJDIR  := build
obj      = $(patsubst %.c,$(OBJDIR)/%.o,$(1))

BINS := bank_server bank_client bank_init bank_verify bank_stress

all: $(BINS)

bank_server: $(call obj,$(COMMON) $(SERVER))
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bank_client: $(call obj,$(COMMON) $(CLIENT))
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bank_init: $(call obj,$(COMMON) tools/bank_init.c)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bank_verify: $(call obj,$(COMMON) tools/bank_verify.c)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

bank_stress: $(call obj,$(COMMON) tools/bank_stress.c)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.c $(wildcard include/*.h)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

reset: bank_init
	./bank_init --yes

clean:
	rm -rf $(OBJDIR) $(BINS)

clean-ipc:
	-./scripts/clean_ipc.sh

distclean: clean
	rm -rf data logs

.PHONY: all reset clean clean-ipc distclean
