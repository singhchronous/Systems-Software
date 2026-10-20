/*
 * admin_server.c - ADMINISTRATOR module, server side (UML class "Admin").
 *
 *   Add New Bank Employee            add_employee()
 *   Modify Customer/Employee Details update_customer_details() / update_employee_details()
 *   Manage User Roles                promote_to_manager() / demote_to_employee()
 *                                    + deactivate (delete_employee) / re-activate
 *   Live Sessions (extra)            reads the shared-memory session table
 *   Force Logout (extra)             kill(pid, SIGUSR1) on the user's server process
 *   View Audit Log (extra)           tail of logs/audit.log written by the audit daemon
 *   Change Password / Logout / Exit  auth.c / server.c
 */
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "db.h"
#include "password.h"
#include "server.h"
#include "session.h"
#include "util.h"

/* --------------------------------------------------------- add employee */

static void add_employee(ClientCtx *c, const Request *rq, Response *rs)
{
    char first[NAME_LEN], last[NAME_LEN];
    str_copy(first, rq->s1, sizeof first);
    str_copy(last, rq->s2, sizeof last);
    str_trim(first);
    str_trim(last);
    int role = rq->id1 == ROLE_MANAGER ? ROLE_MANAGER : ROLE_EMPLOYEE;
    if (!valid_name(first) || !valid_name(last)) { resp_err(rs, "Invalid first/last name."); return; }
    if (!valid_phone(rq->s3)) { resp_err(rs, "Invalid phone number (7-15 digits)."); return; }
    if (!valid_email(rq->s4)) { resp_err(rs, "Invalid email address."); return; }
    if (!valid_password(rq->text)) { resp_err(rs, "Password must be 4-32 characters with no spaces."); return; }

    EmployeeRecord e;
    memset(&e, 0, sizeof e);
    e.role = role;
    str_copy(e.first_name, first, sizeof e.first_name);
    str_copy(e.last_name, last, sizeof e.last_name);
    str_copy(e.phone, rq->s3, sizeof e.phone);
    str_copy(e.email, rq->s4, sizeof e.email);
    make_salt(e.salt);
    hash_password(e.salt, rq->text, e.pass_hash);
    e.status = STATUS_ACTIVE;
    e.created_at = e.updated_at = (int64_t)time(NULL);
    int32_t id;
    if (db_append(T_EMPLOYEES, &e, &id) == -1) { resp_err(rs, "Could not create the employee."); return; }
    rs->value = id;
    resp_ok(rs, "New %s added.\n  Employee ID: %d\n  Name       : %s %s\n"
                "They can log in with this ID and the password you set.",
            role_name(role), id, first, last);
    log_transaction(c, "ADD_EMPLOYEE %d (%s %s, %s)", id, first, last, role_name(role));
}

/* ----------------------------------------------------- employee details */

static void get_employee_details(const Request *rq, Response *rs)
{
    EmployeeRecord e;
    if (db_get(T_EMPLOYEES, rq->id1, &e) == -1) { resp_err(rs, "Employee %d not found.", rq->id1); return; }
    char created[32], updated[32];
    format_time(e.created_at, created, sizeof created);
    format_time(e.updated_at, updated, sizeof updated);
    resp_ok(rs, "Employee ID : %d\nName        : %s %s\nRole        : %s\nStatus      : %s%s\n"
                "Phone       : %s\nEmail       : %s\nCreated     : %s\nUpdated     : %s",
            e.id, e.first_name, e.last_name, role_name(e.role), status_name(e.status),
            session_find(e.id, NULL) > 0 ? "  (currently logged in)" : "",
            e.phone, e.email, created, updated);
}

typedef struct {
    int field;
    const char *value;
} EmpModCtx;

static int emp_modify_cb(void *rec, void *ctx, char *err, size_t cap)
{
    EmployeeRecord *e = rec;
    EmpModCtx *m = ctx;
    switch (m->field) {
    case FIELD_FIRST_NAME: str_copy(e->first_name, m->value, sizeof e->first_name); break;
    case FIELD_LAST_NAME:  str_copy(e->last_name, m->value, sizeof e->last_name); break;
    case FIELD_PHONE:      str_copy(e->phone, m->value, sizeof e->phone); break;
    case FIELD_EMAIL:      str_copy(e->email, m->value, sizeof e->email); break;
    case FIELD_PASSWORD:
        make_salt(e->salt);
        hash_password(e->salt, m->value, e->pass_hash);
        break;
    default: str_copy(err, "Unknown field.", cap); return -1;
    }
    e->updated_at = (int64_t)time(NULL);
    return 0;
}

static void update_employee_details(ClientCtx *c, const Request *rq, Response *rs)
{
    if (validate_field(rq->id2, rq->s1, rs) == -1) return;
    EmpModCtx m = {rq->id2, rq->s1};
    char err[128] = "";
    int rc = db_update(T_EMPLOYEES, rq->id1, emp_modify_cb, &m, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Employee %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }
    resp_ok(rs, "Employee %d updated.", rq->id1);
    log_transaction(c, "MODIFY_EMPLOYEE %d field %d", rq->id1, rq->id2);
}

/* -------------------------------------------- loans of a removed employee */

static int release_loans_cb(void *rec, void *ctx)
{
    LoanRecord *l = rec;
    int32_t emp = *(int32_t *)ctx;
    if (l->assigned_to == emp && (l->status == LOAN_ASSIGNED || l->status == LOAN_UNDER_REVIEW)) {
        l->assigned_to = 0;
        l->status = LOAN_PENDING;
        str_copy(l->remark, "Returned to queue (employee unavailable)", sizeof l->remark);
        l->updated_at = (int64_t)time(NULL);
        return 1;
    }
    return 0;
}

/* --------------------------------------------------------- manage roles */

typedef struct {
    int new_role;
    int old_role;
} RoleCtx;

static int role_cb(void *rec, void *ctx, char *err, size_t cap)
{
    EmployeeRecord *e = rec;
    RoleCtx *r = ctx;
    r->old_role = e->role;
    if (e->status != STATUS_ACTIVE) { str_copy(err, "Re-activate this employee before changing the role.", cap); return -1; }
    if (e->role == r->new_role) {
        snprintf(err, cap, "%d is already a %s.", e->id, role_name(e->role));
        return -1;
    }
    e->role = r->new_role;
    e->updated_at = (int64_t)time(NULL);
    return 0;
}

/* UML: manage_user_roles -> promote_to_manager / demote_to_employee */
static void manage_user_roles(ClientCtx *c, const Request *rq, Response *rs)
{
    if (rq->id2 != ROLE_EMPLOYEE && rq->id2 != ROLE_MANAGER) {
        resp_err(rs, "New role must be Employee (2) or Manager (3).");
        return;
    }
    RoleCtx r = {rq->id2, 0};
    char err[160] = "";
    int rc = db_update(T_EMPLOYEES, rq->id1, role_cb, &r, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Employee %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }

    int released = 0;
    if (r.new_role == ROLE_MANAGER) {        /* a manager does not process loans */
        int32_t emp = rq->id1;
        released = db_scan_update(T_LOANS, release_loans_cb, &emp);
    }
    int kicked = force_logout_user(rq->id1); /* new role takes effect at next login */
    resp_ok(rs, "%d %s from %s to %s.", rq->id1,
            r.new_role == ROLE_MANAGER ? "PROMOTED" : "DEMOTED", role_name(r.old_role), role_name(r.new_role));
    if (released > 0) resp_append(rs, "\n%d open loan application(s) returned to the PENDING queue.", released);
    if (kicked) resp_append(rs, "\nThe user was online and has been logged out so the new role applies.");
    log_transaction(c, "%s %d: %s -> %s", r.new_role == ROLE_MANAGER ? "PROMOTE" : "DEMOTE",
                    rq->id1, role_name(r.old_role), role_name(r.new_role));
}

typedef struct {
    int new_status;
} EmpStatusCtx;

static int emp_status_cb(void *rec, void *ctx, char *err, size_t cap)
{
    EmployeeRecord *e = rec;
    EmpStatusCtx *s = ctx;
    if (e->status == s->new_status) {
        snprintf(err, cap, "Employee %d is already %s.", e->id, status_name(e->status));
        return -1;
    }
    e->status = s->new_status;
    e->updated_at = (int64_t)time(NULL);
    return 0;
}

/* UML: delete_employee (soft delete = deactivate; history is kept) */
static void set_employee_status(ClientCtx *c, const Request *rq, Response *rs)
{
    if (rq->id2 != 1 && rq->id2 != 2) { resp_err(rs, "Choose 1 (activate) or 2 (deactivate)."); return; }
    EmpStatusCtx s = {rq->id2 == 1 ? STATUS_ACTIVE : STATUS_INACTIVE};
    char err[160] = "";
    int rc = db_update(T_EMPLOYEES, rq->id1, emp_status_cb, &s, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Employee %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }
    int released = 0, kicked = 0;
    if (s.new_status == STATUS_INACTIVE) {
        int32_t emp = rq->id1;
        released = db_scan_update(T_LOANS, release_loans_cb, &emp);
        kicked = force_logout_user(rq->id1);
    }
    resp_ok(rs, "Employee %d is now %s.", rq->id1, status_name(s.new_status));
    if (released > 0) resp_append(rs, "\n%d open loan application(s) returned to the PENDING queue.", released);
    if (kicked) resp_append(rs, "\nThe employee was online and has been logged out.");
    log_transaction(c, "SET_EMPLOYEE_STATUS %d -> %s", rq->id1, status_name(s.new_status));
}

/* ------------------------------------------------------- live sessions */

static void list_sessions(Response *rs)
{
    static SessionTable t;
    int n = session_snapshot(&t);
    int64_t now = (int64_t)time(NULL);
    char started[32];
    format_time(t.server_start, started, sizeof started);
    long up = (long)(now - t.server_start);
    resp_ok(rs, "LIVE SESSIONS (System V shared memory, protected by a semaphore)\n");
    resp_append(rs, "Server pid %d, up since %s (%ldh %02ldm %02lds)\n", t.server_pid, started,
                up / 3600, (up / 60) % 60, up % 60);
    resp_append(rs, "Connections: %d   Logins: %d   Stale sessions reclaimed: %d\n\n",
                t.total_connections, t.total_logins, t.reclaimed_stale);
    resp_append(rs, "%-6s %-14s %-22s %-7s %-21s %-8s %s\n", "User", "Role", "Name", "Pid", "Client", "Idle(s)", "Logged in at");
    resp_append(rs, "----------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < MAX_SESSIONS; i++) {
        SessionSlot *s = &t.slot[i];
        if (!s->in_use) continue;
        char at[32];
        format_time(s->login_time, at, sizeof at);
        resp_append(rs, "%-6d %-14s %-22.22s %-7d %-21s %-8ld %s\n", s->uid, role_name(s->role), s->name,
                    s->pid, s->client_addr, (long)(now - s->last_active), at);
    }
    resp_append(rs, "\n%d user(s) online.\n", n);
}

static void force_logout(ClientCtx *c, const Request *rq, Response *rs)
{
    if (rq->id1 == c->uid) { resp_err(rs, "You cannot force-logout yourself. Use Logout."); return; }
    if (force_logout_user(rq->id1)) {
        resp_ok(rs, "User %d has been sent a forced-logout signal (SIGUSR1).", rq->id1);
        log_transaction(c, "FORCE_LOGOUT user %d", rq->id1);
    } else {
        resp_err(rs, "User %d is not logged in.", rq->id1);
    }
}

/* ----------------------------------------------------------- audit log */

static void view_audit_log(const Request *rq, Response *rs)
{
    int want = rq->id1 > 0 && rq->id1 <= 80 ? rq->id1 : 25;
    char path[PATH_MAX - 64];
    if (!getcwd(path, sizeof path)) { resp_err(rs, "Cannot locate log."); return; }
    char full[PATH_MAX];
    snprintf(full, sizeof full, "%s/logs/audit.log", path);
    int fd = open(full, O_RDONLY);
    if (fd < 0) { resp_err(rs, "Audit log is empty."); return; }
    struct stat st;
    fstat(fd, &st);
    static char buf[RESP_TEXT_LEN];
    off_t start = st.st_size > (off_t)(sizeof buf - 1) ? st.st_size - (off_t)(sizeof buf - 1) : 0;
    lseek(fd, start, SEEK_SET);                 /* random access to the end of the file */
    ssize_t n = read(fd, buf, sizeof buf - 1);
    close(fd);
    if (n <= 0) { resp_ok(rs, "(audit log is empty)"); return; }
    buf[n] = '\0';
    /* walk back 'want' lines */
    int lines = 0;
    char *p = buf + n;
    if (p > buf && p[-1] == '\n') p--;
    while (p > buf) {
        if (p[-1] == '\n' && ++lines == want) break;
        p--;
    }
    resp_ok(rs, "Last %d audit log entries (written by the audit daemon via message queue):\n", want);
    resp_append(rs, "%s", p);
}

/* UML: Bank::handle_admin_requests */
int handle_admin_requests(ClientCtx *c, const Request *rq, Response *rs)
{
    switch (rq->op) {
    case OP_ADD_EMPLOYEE:         add_employee(c, rq, rs); return 1;
    case OP_GET_EMPLOYEE:         get_employee_details(rq, rs); return 1;
    case OP_MODIFY_EMPLOYEE:      update_employee_details(c, rq, rs); return 1;
    case OP_GET_CUSTOMER:         op_customer_details(rq->id1, rs); return 1;
    case OP_MODIFY_CUSTOMER:      op_modify_customer(c, rq, rs); return 1;
    case OP_CHANGE_ROLE:          manage_user_roles(c, rq, rs); return 1;
    case OP_SET_EMPLOYEE_STATUS:  set_employee_status(c, rq, rs); return 1;
    case OP_LIST_EMPLOYEES:       op_list_employees(rs, 0); return 1;
    case OP_LIST_CUSTOMERS:       op_list_customers(rs); return 1;
    case OP_LIST_SESSIONS:        list_sessions(rs); return 1;
    case OP_FORCE_LOGOUT:         force_logout(c, rq, rs); return 1;
    case OP_VIEW_AUDIT_LOG:       view_audit_log(rq, rs); return 1;
    default:                      return 0;
    }
}
