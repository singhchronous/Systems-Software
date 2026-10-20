/*
 * shared_ops.c - Operations used by more than one role module
 *                (customer listing/details/modification, passbook, employee list).
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "db.h"
#include "password.h"
#include "server.h"
#include "session.h"
#include "util.h"

/* ------------------------------------------------------- list customers */

static int list_cust_cb(const void *rec, void *ctx)
{
    const CustomerRecord *c = rec;
    Response *rs = ctx;
    char bal[40], name[70];
    format_money(c->balance, bal, sizeof bal);
    snprintf(name, sizeof name, "%s %s", c->first_name, c->last_name);
    int online = session_find(c->id, NULL) > 0;
    resp_append(rs, "%-6d %-24.24s %-12s %20s  %-9s%s\n", c->id, name, c->phone, bal,
                status_name(c->status), online ? " (online)" : "");
    return 0;
}

void op_list_customers(Response *rs)
{
    resp_ok(rs, "%-6s %-24s %-12s %20s  %s\n", "A/c", "Name", "Phone", "Balance", "Status");
    resp_append(rs, "------------------------------------------------------------------------------\n");
    db_scan(T_CUSTOMERS, list_cust_cb, rs);
}

/* ---------------------------------------------------- customer details */

typedef struct {
    int32_t cust;
    Response *rs;
    int count;
} LoanListCtx;

static int cust_loans_cb(const void *rec, void *ctx)
{
    const LoanRecord *l = rec;
    LoanListCtx *lc = ctx;
    if (l->customer_id != lc->cust) return 0;
    char amt[40];
    format_money(l->amount, amt, sizeof amt);
    resp_append(lc->rs, "  Loan %-5d %18s  %-12s  %s\n", l->id, amt, loan_status_name(l->status), l->purpose);
    lc->count++;
    return 0;
}

/* UML: get_customer_details */
void op_customer_details(int32_t id, Response *rs)
{
    CustomerRecord c;
    if (db_get(T_CUSTOMERS, id, &c) == -1) {
        resp_err(rs, "Customer %d not found.", id);
        return;
    }
    char bal[40], loan[40], created[32], updated[32];
    format_money(c.balance, bal, sizeof bal);
    format_money(c.loan_amount, loan, sizeof loan);
    format_time(c.created_at, created, sizeof created);
    format_time(c.updated_at, updated, sizeof updated);
    resp_ok(rs,
            "Account number : %d\n"
            "Name           : %s %s\n"
            "Phone          : %s\n"
            "Email          : %s\n"
            "Status         : %s%s\n"
            "Balance        : %s\n"
            "Loans approved : %s\n"
            "Opened on      : %s\n"
            "Last updated   : %s\n"
            "Loan applications:\n",
            c.id, c.first_name, c.last_name, c.phone, c.email, status_name(c.status),
            session_find(c.id, NULL) > 0 ? "  (currently logged in)" : "",
            bal, loan, created, updated);
    LoanListCtx lc = {id, rs, 0};
    db_scan(T_LOANS, cust_loans_cb, &lc);
    if (lc.count == 0) resp_append(rs, "  (none)\n");
}

/* --------------------------------------------------- modify customer */

typedef struct {
    int field;
    const char *value;
    char old_value[64];
} ModCtx;

static int modify_cust_cb(void *rec, void *ctx, char *err, size_t cap)
{
    CustomerRecord *c = rec;
    ModCtx *m = ctx;
    if (c->status == STATUS_CLOSED) {
        str_copy(err, "This account is CLOSED and cannot be modified.", cap);
        return -1;
    }
    switch (m->field) {
    case FIELD_FIRST_NAME:
        str_copy(m->old_value, c->first_name, sizeof m->old_value);
        str_copy(c->first_name, m->value, sizeof c->first_name);
        break;
    case FIELD_LAST_NAME:
        str_copy(m->old_value, c->last_name, sizeof m->old_value);
        str_copy(c->last_name, m->value, sizeof c->last_name);
        break;
    case FIELD_PHONE:
        str_copy(m->old_value, c->phone, sizeof m->old_value);
        str_copy(c->phone, m->value, sizeof c->phone);
        break;
    case FIELD_EMAIL:
        str_copy(m->old_value, c->email, sizeof m->old_value);
        str_copy(c->email, m->value, sizeof c->email);
        break;
    case FIELD_PASSWORD:
        str_copy(m->old_value, "********", sizeof m->old_value);
        make_salt(c->salt);
        hash_password(c->salt, m->value, c->pass_hash);
        break;
    default:
        str_copy(err, "Unknown field.", cap);
        return -1;
    }
    c->updated_at = (int64_t)time(NULL);
    return 0;
}

int validate_field(int field, const char *v, Response *rs)
{
    switch (field) {
    case FIELD_FIRST_NAME:
    case FIELD_LAST_NAME:
        if (!valid_name(v)) { resp_err(rs, "Invalid name (letters, spaces, . - ' only; max 31 chars)."); return -1; }
        break;
    case FIELD_PHONE:
        if (!valid_phone(v)) { resp_err(rs, "Invalid phone number (7-15 digits)."); return -1; }
        break;
    case FIELD_EMAIL:
        if (!valid_email(v)) { resp_err(rs, "Invalid email address."); return -1; }
        break;
    case FIELD_PASSWORD:
        if (!valid_password(v)) { resp_err(rs, "Password must be 4-32 characters with no spaces."); return -1; }
        break;
    default:
        resp_err(rs, "Unknown field.");
        return -1;
    }
    return 0;
}

static const char *field_name(int f)
{
    switch (f) {
    case FIELD_FIRST_NAME: return "first name";
    case FIELD_LAST_NAME:  return "last name";
    case FIELD_PHONE:      return "phone";
    case FIELD_EMAIL:      return "email";
    case FIELD_PASSWORD:   return "password";
    default:               return "?";
    }
}

/* UML: update_customer_details */
void op_modify_customer(ClientCtx *c, const Request *rq, Response *rs)
{
    if (validate_field(rq->id2, rq->s1, rs) == -1) return;
    ModCtx m = {rq->id2, rq->s1, ""};
    char err[128] = "";
    int rc = db_update(T_CUSTOMERS, rq->id1, modify_cust_cb, &m, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Customer %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err[0] ? err : "Update failed."); return; }
    if (rq->id2 == FIELD_PASSWORD) {
        resp_ok(rs, "Password of customer %d has been reset.", rq->id1);
        log_transaction(c, "reset password of customer %d", rq->id1);
    } else {
        resp_ok(rs, "Customer %d %s changed from '%s' to '%s'.", rq->id1, field_name(rq->id2), m.old_value, rq->s1);
        log_transaction(c, "modified customer %d %s", rq->id1, field_name(rq->id2));
    }
}

/* ------------------------------------------------------ list employees */

typedef struct {
    Response *rs;
    int only_employees;
} EmpListCtx;

typedef struct {
    int32_t emp;
    int open_loans;
} WorkCtx;

static int workload_cb(const void *rec, void *ctx)
{
    const LoanRecord *l = rec;
    WorkCtx *w = ctx;
    if (l->assigned_to == w->emp && (l->status == LOAN_ASSIGNED || l->status == LOAN_UNDER_REVIEW))
        w->open_loans++;
    return 0;
}

static int list_emp_cb(const void *rec, void *ctx)
{
    const EmployeeRecord *e = rec;
    EmpListCtx *ec = ctx;
    if (ec->only_employees && (e->role != ROLE_EMPLOYEE || e->status != STATUS_ACTIVE)) return 0;
    char name[70];
    snprintf(name, sizeof name, "%s %s", e->first_name, e->last_name);
    resp_append(ec->rs, "%-6d %-22.22s %-9s %-9s %-26.26s%s\n", e->id, name, role_name(e->role),
                status_name(e->status), e->email, session_find(e->id, NULL) > 0 ? " (online)" : "");
    return 0;
}

void op_list_employees(Response *rs, int only_employees)
{
    resp_ok(rs, "%-6s %-22s %-9s %-9s %s\n", "ID", "Name", "Role", "Status", "Email");
    resp_append(rs, "------------------------------------------------------------------------------\n");
    EmpListCtx ec = {rs, only_employees};
    db_scan(T_EMPLOYEES, list_emp_cb, &ec);
    if (only_employees) {
        /* add the current workload so the manager can balance assignments */
        resp_append(rs, "\nOpen loans per employee: ");
        int n = db_count(T_EMPLOYEES);
        for (int i = 0; i < n; i++) {
            EmployeeRecord e;
            if (db_get(T_EMPLOYEES, EMPLOYEE_ID_BASE + i, &e) == 0 &&
                e.role == ROLE_EMPLOYEE && e.status == STATUS_ACTIVE) {
                WorkCtx w = {e.id, 0};
                db_scan(T_LOANS, workload_cb, &w);
                resp_append(rs, "%d=%d  ", e.id, w.open_loans);
            }
        }
        resp_append(rs, "\n");
    }
}

/* ------------------------------------------------------------ passbook */

#define PASSBOOK_MAX 60

typedef struct {
    int32_t acct;
    TransactionRecord ring[PASSBOOK_MAX];
    int max_rows;
    int total;
    int64_t credits, debits;
} PassCtx;

static int passbook_cb(const void *rec, void *ctx)
{
    const TransactionRecord *t = rec;
    PassCtx *p = ctx;
    if (t->account_id != p->acct || t->state != TXN_STATE_COMMITTED) return 0;  /* only committed */
    p->ring[p->total % p->max_rows] = *t;
    p->total++;
    if (t->type == TXN_WITHDRAW || t->type == TXN_TRANSFER_OUT) p->debits += t->amount;
    else p->credits += t->amount;
    return 0;
}

/* UML: view_transaction_history - used by the customer (own account) and by
 * the employee as a passbook for any customer */
void op_passbook(int32_t customer_id, int max_rows, Response *rs)
{
    CustomerRecord c;
    if (db_get(T_CUSTOMERS, customer_id, &c) == -1) {
        resp_err(rs, "Customer %d not found.", customer_id);
        return;
    }
    static PassCtx p;                 /* ~7 KB: keep it off the stack */
    memset(&p, 0, sizeof p);
    p.acct = customer_id;
    p.max_rows = max_rows > PASSBOOK_MAX ? PASSBOOK_MAX : max_rows;
    db_scan(T_TXNS, passbook_cb, &p);

    char bal[40], cr[40], dr[40];
    format_money(c.balance, bal, sizeof bal);
    format_money(p.credits, cr, sizeof cr);
    format_money(p.debits, dr, sizeof dr);
    resp_ok(rs, "PASSBOOK  A/c %d  %s %s  [%s]\n", c.id, c.first_name, c.last_name, status_name(c.status));
    resp_append(rs, "%-6s %-19s %-12s %18s %18s  %s\n", "Txn", "Date/Time", "Type", "Debit / Credit", "Balance", "Description");
    resp_append(rs, "------------------------------------------------------------------------------------------------\n");
    int shown = p.total < p.max_rows ? p.total : p.max_rows;
    int start = p.total - shown;
    for (int k = start; k < p.total; k++) {
        TransactionRecord *t = &p.ring[k % p.max_rows];
        char when[32], amt[40], after[40], signed_amt[44];
        format_time(t->timestamp, when, sizeof when);
        format_money(t->amount, amt, sizeof amt);
        format_money(t->balance_after, after, sizeof after);
        int debit = (t->type == TXN_WITHDRAW || t->type == TXN_TRANSFER_OUT);
        snprintf(signed_amt, sizeof signed_amt, "%s%s", debit ? "-" : "+", amt);
        resp_append(rs, "%-6d %-19s %-12s %18s %18s  %s\n", t->id, when, txn_type_name(t->type),
                    signed_amt, after, t->description);
    }
    if (p.total == 0) resp_append(rs, "(no transactions)\n");
    resp_append(rs, "------------------------------------------------------------------------------------------------\n");
    resp_append(rs, "Showing %d of %d transaction(s).  Total credits: %s   Total debits: %s\n", shown, p.total, cr, dr);
    resp_append(rs, "Current balance: %s\n", bal);
}
