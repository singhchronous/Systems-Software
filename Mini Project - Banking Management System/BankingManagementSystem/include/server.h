/*
 * server.h - Shared declarations for the server process and the four role
 *            modules (UML class "Bank" + Customer/Employee/Manager/Admin).
 */
#ifndef SERVER_H
#define SERVER_H

#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"
#include "records.h"

/* State of ONE connected client, owned by the server child serving it. */
typedef struct {
    int     sock;
    int     logged_in;
    int     role;                   /* Role */
    int32_t uid;
    int     slot;                   /* index in the shared-memory session table */
    char    name[64];
    char    addr[32];               /* client ip:port */
} ClientCtx;

/* Flags set by signal handlers in the server child (see server.c). */
extern volatile sig_atomic_t g_idle_timed_out;   /* SIGALRM */
extern volatile sig_atomic_t g_forced_logout;    /* SIGUSR1 */
extern volatile sig_atomic_t g_shutting_down;    /* SIGTERM / SIGINT */
extern int g_idle_timeout_sec;

/* ---- response helpers ---- */
void resp_ok(Response *rs, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void resp_err(Response *rs, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void resp_append(Response *rs, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* ---- Bank (server.c / auth.c) ---- */
int  validate_login(ClientCtx *c, const Request *rq, Response *rs);
void handle_change_password(ClientCtx *c, const Request *rq, Response *rs);
void log_transaction(const ClientCtx *c, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

/* Each returns 1 if it recognised the op, 0 if the op is not allowed for the role. */
int handle_customer_requests(ClientCtx *c, const Request *rq, Response *rs);
int handle_employee_requests(ClientCtx *c, const Request *rq, Response *rs);
int handle_manager_requests(ClientCtx *c, const Request *rq, Response *rs);
int handle_admin_requests(ClientCtx *c, const Request *rq, Response *rs);

/* ---- shared helpers used by several modules (shared_ops.c) ---- */
void op_list_customers(Response *rs);
void op_customer_details(int32_t id, Response *rs);
void op_modify_customer(ClientCtx *c, const Request *rq, Response *rs);
void op_list_employees(Response *rs, int only_employees);
void op_passbook(int32_t customer_id, int max_rows, Response *rs);
int  validate_field(int field, const char *value, Response *rs);   /* FieldCode */

/* Force-logout a user who is currently online (sends SIGUSR1 to the server
 * child that serves them). Returns 1 if a session was found. */
int  force_logout_user(int32_t uid);

/* ---- ledger.c : every money movement, journaled + locked (ACID) ---- */
int ledger_deposit(int32_t acct, int64_t amount, int32_t by, int64_t *new_bal, char *err, size_t cap);
int ledger_withdraw(int32_t acct, int64_t amount, int32_t by, int64_t *new_bal, char *err, size_t cap);
int ledger_transfer(int32_t from, int32_t to, int64_t amount, int32_t by, int64_t *new_bal, char *err, size_t cap);
int ledger_open_account(CustomerRecord *c, int64_t opening, int32_t by, int32_t *new_id, char *err, size_t cap);
int ledger_approve_loan(int32_t loan_id, int32_t emp_id, const char *remark,
                        LoanRecord *out, char *err, size_t cap);

#endif
