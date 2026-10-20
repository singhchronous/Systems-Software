/*
 * auth.c - Login (validate_login), change password, forced logout.
 */
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "audit.h"
#include "db.h"
#include "password.h"
#include "server.h"
#include "session.h"
#include "util.h"

void log_transaction(const ClientCtx *c, const char *fmt, ...)
{
    char msg[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (c && c->logged_in)
        audit_log("%s %d: %s", role_name(c->role), c->uid, msg);
    else
        audit_log("%s", msg);
}

/* -------------------------------------------------------------- login */

int validate_login(ClientCtx *c, const Request *rq, Response *rs)
{
    int role = rq->id1;
    int32_t uid = rq->id2;
    const char *pw = rq->s1;
    char name[64] = "";
    int status = 0, pw_ok = 0, stored_role = role, found = 0;

    if (role == ROLE_CUSTOMER) {
        CustomerRecord r;
        if (db_get(T_CUSTOMERS, uid, &r) == 0) {
            found = 1;
            pw_ok = check_password(r.salt, r.pass_hash, pw);
            status = r.status;
            snprintf(name, sizeof name, "%s %s", r.first_name, r.last_name);
        }
    } else if (role == ROLE_EMPLOYEE || role == ROLE_MANAGER) {
        EmployeeRecord r;
        if (db_get(T_EMPLOYEES, uid, &r) == 0) {
            found = 1;
            pw_ok = check_password(r.salt, r.pass_hash, pw);
            status = r.status;
            stored_role = r.role;
            snprintf(name, sizeof name, "%s %s", r.first_name, r.last_name);
        }
    } else if (role == ROLE_ADMIN) {
        AdminRecord r;
        if (db_get(T_ADMINS, uid, &r) == 0) {
            found = 1;
            pw_ok = check_password(r.salt, r.pass_hash, pw);
            status = r.status;
            snprintf(name, sizeof name, "%s %s", r.first_name, r.last_name);
        }
    } else {
        resp_err(rs, "Invalid role selected.");
        return -1;
    }

    if (!pw_ok) {
        audit_log("LOGIN FAILED: %s id %d from %s (wrong id or password)", role_name(role), uid, c->addr);
        printf("[server] pid %d: login failed for %s id %d (%s)\n", (int)getpid(), role_name(role), uid,
               found ? "wrong password" : "no such id for this login type");
        sleep(1);                         /* slow down password guessing */
        resp_err(rs, "Invalid ID or password.\n"
                     "        Check that you chose the right login type: Customer = 1xxx, "
                     "Employee/Manager = 2xxx, Administrator = 9xxx.");
        return -1;
    }
    if (stored_role != role) {
        resp_err(rs, "ID %d is registered as %s. Please use the %s login.",
                 uid, role_name(stored_role), role_name(stored_role));
        return -1;
    }
    if (status == STATUS_CLOSED) {
        resp_err(rs, "This account is CLOSED. Please contact the bank.");
        audit_log("LOGIN DENIED: %s %d (closed)", role_name(role), uid);
        return -1;
    }
    if (status != STATUS_ACTIVE) {
        resp_err(rs, "This account is DEACTIVATED. Please contact the bank.");
        audit_log("LOGIN DENIED: %s %d (deactivated)", role_name(role), uid);
        return -1;
    }

    /* one session per user - checked atomically in shared memory */
    pid_t holder = 0;
    int slot = session_acquire(role, uid, name, c->addr, &holder);
    if (slot == -1) {
        resp_err(rs, "User %d is already logged in from another session. "
                     "Only one session per user is allowed.", uid);
        audit_log("LOGIN DENIED: %s %d already logged in (session pid %d)", role_name(role), uid, (int)holder);
        return -1;
    }
    if (slot < 0) {
        resp_err(rs, "Server is full, please try again later.");
        return -1;
    }

    c->logged_in = 1;
    c->role = role;
    c->uid = uid;
    c->slot = slot;
    str_copy(c->name, name, sizeof c->name);
    rs->role = role;
    rs->uid = uid;
    resp_ok(rs, "%s", name);
    audit_log("LOGIN: %s %d (%s) from %s", role_name(role), uid, name, c->addr);
    return 0;
}

/* ----------------------------------------------------- change password */

typedef struct {
    const char *old_pw;
    const char *new_pw;
} PwCtx;

static int set_pw(char *salt, char *hash, const PwCtx *p, char *err, size_t cap)
{
    if (!check_password(salt, hash, p->old_pw)) {
        str_copy(err, "Current password is incorrect.", cap);
        return -1;
    }
    make_salt(salt);
    hash_password(salt, p->new_pw, hash);
    return 0;
}

static int cust_pw(void *rec, void *ctx, char *err, size_t cap)
{
    CustomerRecord *r = rec;
    r->updated_at = (int64_t)time(NULL);
    return set_pw(r->salt, r->pass_hash, ctx, err, cap);
}

static int emp_pw(void *rec, void *ctx, char *err, size_t cap)
{
    EmployeeRecord *r = rec;
    r->updated_at = (int64_t)time(NULL);
    return set_pw(r->salt, r->pass_hash, ctx, err, cap);
}

static int admin_pw(void *rec, void *ctx, char *err, size_t cap)
{
    AdminRecord *r = rec;
    r->updated_at = (int64_t)time(NULL);
    return set_pw(r->salt, r->pass_hash, ctx, err, cap);
}

/* change_customer_password / change_employee_password /
 * change_manager_password / change_admin_password (UML) */
void handle_change_password(ClientCtx *c, const Request *rq, Response *rs)
{
    if (!valid_password(rq->s2)) {
        resp_err(rs, "New password must be 4-32 characters with no spaces.");
        return;
    }
    if (strcmp(rq->s1, rq->s2) == 0) {
        resp_err(rs, "New password must be different from the current one.");
        return;
    }
    PwCtx p = {rq->s1, rq->s2};
    char err[128] = "";
    int rc;
    if (c->role == ROLE_CUSTOMER)      rc = db_update(T_CUSTOMERS, c->uid, cust_pw, &p, err, sizeof err);
    else if (c->role == ROLE_ADMIN)    rc = db_update(T_ADMINS, c->uid, admin_pw, &p, err, sizeof err);
    else                               rc = db_update(T_EMPLOYEES, c->uid, emp_pw, &p, err, sizeof err);

    if (rc == 0) {
        resp_ok(rs, "Password changed successfully.");
        log_transaction(c, "changed own password");
    } else {
        resp_err(rs, "%s", err[0] ? err : "Could not change password.");
    }
}

/* --------------------------------------------------------- forced logout */

int force_logout_user(int32_t uid)
{
    int role = 0;
    pid_t pid = session_find(uid, &role);
    if (pid <= 0 || pid == getpid()) return 0;
    /* SIGUSR1 -> the server child serving that user sets g_forced_logout,
     * tells its client "you have been logged out" and ends the session */
    return kill(pid, SIGUSR1) == 0;
}
