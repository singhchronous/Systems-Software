/*
 * employee_server.c - BANK EMPLOYEE module, server side (UML class "Employee").
 *
 *   Add New Customer                 add_customer()        (opens an account)
 *   Modify Customer Details          update_customer_details()
 *   Close Customer Account           delete_customer()     (spec: "opening, modifying, or closing accounts")
 *   View Assigned Loan Applications  get_assigned_loans()
 *   Process Loan Applications        process_loan()        ASSIGNED -> UNDER_REVIEW + review sheet
 *   Approve/Reject Loans             update_loan_status()
 *   View Customer Transactions       view_transaction_history()  (passbook)
 *   Change Password / Logout / Exit  auth.c / server.c
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "db.h"
#include "password.h"
#include "server.h"
#include "session.h"
#include "util.h"

/* --------------------------------------------------------- add customer */

static void add_customer(ClientCtx *c, const Request *rq, Response *rs)
{
    char first[NAME_LEN], last[NAME_LEN];
    str_copy(first, rq->s1, sizeof first);
    str_copy(last, rq->s2, sizeof last);
    str_trim(first);
    str_trim(last);
    if (!valid_name(first) || !valid_name(last)) { resp_err(rs, "Invalid first/last name."); return; }
    if (!valid_phone(rq->s3)) { resp_err(rs, "Invalid phone number (7-15 digits)."); return; }
    if (!valid_email(rq->s4)) { resp_err(rs, "Invalid email address."); return; }
    if (!valid_password(rq->text)) { resp_err(rs, "Password must be 4-32 characters with no spaces."); return; }
    if (rq->amount < 0) { resp_err(rs, "Opening deposit cannot be negative."); return; }

    CustomerRecord nc;
    memset(&nc, 0, sizeof nc);
    str_copy(nc.first_name, first, sizeof nc.first_name);
    str_copy(nc.last_name, last, sizeof nc.last_name);
    str_copy(nc.phone, rq->s3, sizeof nc.phone);
    str_copy(nc.email, rq->s4, sizeof nc.email);
    make_salt(nc.salt);
    hash_password(nc.salt, rq->text, nc.pass_hash);

    char err[160] = "";
    int32_t id = 0;
    int rc = ledger_open_account(&nc, rq->amount, c->uid, &id, err, sizeof err);
    if (rc == -1) { resp_err(rs, "%s", err); return; }
    char bal[40];
    format_money(nc.balance, bal, sizeof bal);
    rs->value = id;
    if (rc == -2)
        resp_ok(rs, "Account %d created for %s %s, but the opening deposit failed: %s", id, first, last, err);
    else
        resp_ok(rs, "New account opened.\n  Account number : %d\n  Customer       : %s %s\n  Opening balance: %s\n"
                    "The customer can now log in with this account number and the password you set.",
                id, first, last, bal);
    log_transaction(c, "ADD_CUSTOMER %d (%s %s) opening %s", id, first, last, bal);
}

/* ------------------------------------------------------- close account */

typedef struct {
    int32_t cust;
    int open;
} OpenLoansCtx;

static int open_loans_cb(const void *rec, void *ctx)
{
    const LoanRecord *l = rec;
    OpenLoansCtx *o = ctx;
    if (l->customer_id == o->cust &&
        (l->status == LOAN_PENDING || l->status == LOAN_ASSIGNED || l->status == LOAN_UNDER_REVIEW))
        o->open++;
    return 0;
}

static int close_cb(void *rec, void *ctx, char *err, size_t cap)
{
    (void)ctx;
    CustomerRecord *r = rec;
    if (r->status == STATUS_CLOSED) { str_copy(err, "Account is already closed.", cap); return -1; }
    if (r->balance != 0) {
        char b[40];
        format_money(r->balance, b, sizeof b);
        snprintf(err, cap, "Balance must be zero before closing (current balance %s). "
                           "Ask the customer to withdraw or transfer it first.", b);
        return -1;
    }
    r->status = STATUS_CLOSED;
    r->updated_at = (int64_t)time(NULL);
    return 0;
}

static void delete_customer(ClientCtx *c, const Request *rq, Response *rs)
{
    OpenLoansCtx o = {rq->id1, 0};
    db_scan(T_LOANS, open_loans_cb, &o);
    if (o.open > 0) {
        resp_err(rs, "Customer %d has %d open loan application(s). Resolve them before closing the account.",
                 rq->id1, o.open);
        return;
    }
    char err[200] = "";
    int rc = db_update(T_CUSTOMERS, rq->id1, close_cb, NULL, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Customer %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }
    int kicked = force_logout_user(rq->id1);
    resp_ok(rs, "Account %d has been CLOSED.%s", rq->id1, kicked ? " The customer's active session was ended." : "");
    log_transaction(c, "CLOSE_ACCOUNT %d", rq->id1);
}

/* ------------------------------------------------------ assigned loans */

typedef struct {
    int32_t emp;
    Response *rs;
    int open, decided;
} AssignedCtx;

static int assigned_cb(const void *rec, void *ctx)
{
    const LoanRecord *l = rec;
    AssignedCtx *a = ctx;
    if (l->assigned_to != a->emp) return 0;
    if (l->status == LOAN_APPROVED || l->status == LOAN_REJECTED) { a->decided++; return 0; }
    char amt[40], when[32];
    format_money(l->amount, amt, sizeof amt);
    format_time(l->applied_at, when, sizeof when);
    resp_append(a->rs, "%-6d %-8d %18s %6d  %-13s %-19s %s\n", l->id, l->customer_id, amt,
                l->tenure_months, loan_status_name(l->status), when, l->purpose);
    a->open++;
    return 0;
}

static void get_assigned_loans(ClientCtx *c, Response *rs)
{
    resp_ok(rs, "Loan applications assigned to you (%s):\n", c->name);
    resp_append(rs, "%-6s %-8s %18s %6s  %-13s %-19s %s\n", "Loan", "A/c", "Amount", "Months", "Status", "Applied on", "Purpose");
    resp_append(rs, "-----------------------------------------------------------------------------------------\n");
    AssignedCtx a = {c->uid, rs, 0, 0};
    db_scan(T_LOANS, assigned_cb, &a);
    if (a.open == 0) resp_append(rs, "(no open applications assigned to you)\n");
    resp_append(rs, "\nOpen: %d   Already decided by you: %d\n", a.open, a.decided);
    resp_append(rs, "Workflow: ASSIGNED -> 'Process Loan Application' -> UNDER_REVIEW -> 'Approve/Reject'\n");
}

/* ------------------------------------------------------- process loan */

typedef struct {
    int32_t emp;
    LoanRecord result;
} ProcessCtx;

static int process_cb(void *rec, void *ctx, char *err, size_t cap)
{
    LoanRecord *l = rec;
    ProcessCtx *p = ctx;
    if (l->assigned_to != p->emp) { str_copy(err, "This loan is not assigned to you.", cap); return -1; }
    if (l->status == LOAN_APPROVED || l->status == LOAN_REJECTED) {
        snprintf(err, cap, "Loan %d was already %s.", l->id, loan_status_name(l->status));
        return -1;
    }
    if (l->status == LOAN_UNDER_REVIEW) { p->result = *l; return 1; }   /* already processing */
    l->status = LOAN_UNDER_REVIEW;
    l->updated_at = (int64_t)time(NULL);
    p->result = *l;
    return 0;
}

typedef struct {
    int32_t acct;
    int count;
    int64_t credits, debits;
} TxnStatCtx;

static int txn_stat_cb(const void *rec, void *ctx)
{
    const TransactionRecord *t = rec;
    TxnStatCtx *s = ctx;
    if (t->account_id != s->acct || t->state != TXN_STATE_COMMITTED) return 0;
    s->count++;
    if (t->type == TXN_WITHDRAW || t->type == TXN_TRANSFER_OUT) s->debits += t->amount;
    else s->credits += t->amount;
    return 0;
}

static void process_loan(ClientCtx *c, const Request *rq, Response *rs)
{
    ProcessCtx p;
    memset(&p, 0, sizeof p);
    p.emp = c->uid;
    char err[160] = "";
    int rc = db_update(T_LOANS, rq->id1, process_cb, &p, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Loan %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }

    LoanRecord *l = &p.result;
    CustomerRecord cu;
    if (db_get(T_CUSTOMERS, l->customer_id, &cu) == -1) { resp_err(rs, "Customer not found."); return; }
    TxnStatCtx s = {cu.id, 0, 0, 0};
    db_scan(T_TXNS, txn_stat_cb, &s);

    char amt[40], bal[40], loans[40], cr[40], dr[40], applied[32], opened[32];
    format_money(l->amount, amt, sizeof amt);
    format_money(cu.balance, bal, sizeof bal);
    format_money(cu.loan_amount, loans, sizeof loans);
    format_money(s.credits, cr, sizeof cr);
    format_money(s.debits, dr, sizeof dr);
    format_time(l->applied_at, applied, sizeof applied);
    format_time(cu.created_at, opened, sizeof opened);
    double emi_rupees = (double)l->amount / 100.0 / (l->tenure_months > 0 ? l->tenure_months : 1);

    resp_ok(rs, "Loan %d is now UNDER_REVIEW.\n\n"
                "=========== LOAN REVIEW SHEET ===========\n"
                "Loan id        : %d\n"
                "Amount         : %s for %d months (approx. Rs. %.2f / month, before interest)\n"
                "Purpose        : %s\n"
                "Applied on     : %s\n"
                "--- Applicant ---\n"
                "Account        : %d  %s %s  [%s]\n"
                "Phone / email  : %s / %s\n"
                "Account opened : %s\n"
                "Balance        : %s\n"
                "Existing loans : %s\n"
                "Transactions   : %d (credits %s, debits %s)\n"
                "=========================================\n"
                "Next step: choose 'Approve/Reject Loan' to record your decision.",
            l->id, l->id, amt, l->tenure_months, emi_rupees, l->purpose, applied,
            cu.id, cu.first_name, cu.last_name, status_name(cu.status), cu.phone, cu.email, opened,
            bal, loans, s.count, cr, dr);
    if (rc == 0) log_transaction(c, "PROCESS_LOAN %d (now UNDER_REVIEW)", l->id);
}

/* --------------------------------------------------- approve / reject */

typedef struct {
    int32_t emp;
    const char *remark;
} RejectCtx;

static int reject_cb(void *rec, void *ctx, char *err, size_t cap)
{
    LoanRecord *l = rec;
    RejectCtx *r = ctx;
    if (l->assigned_to != r->emp) { str_copy(err, "This loan is not assigned to you.", cap); return -1; }
    if (l->status == LOAN_APPROVED || l->status == LOAN_REJECTED) {
        snprintf(err, cap, "Loan %d was already %s.", l->id, loan_status_name(l->status));
        return -1;
    }
    if (l->status != LOAN_UNDER_REVIEW) {
        str_copy(err, "Process the application first (menu: Process Loan Application).", cap);
        return -1;
    }
    l->status = LOAN_REJECTED;
    l->decided_by = r->emp;
    str_copy(l->remark, r->remark, sizeof l->remark);
    l->updated_at = (int64_t)time(NULL);
    return 0;
}

/* UML: update_loan_status */
static void update_loan_status(ClientCtx *c, const Request *rq, Response *rs)
{
    char remark[REMARK_LEN];
    str_copy(remark, rq->text, sizeof remark);
    str_trim(remark);
    char err[160] = "";

    if (rq->id2 == 1) {                                   /* APPROVE: money moves -> ledger */
        LoanRecord l;
        if (ledger_approve_loan(rq->id1, c->uid, remark, &l, err, sizeof err) == -1) {
            resp_err(rs, "%s", err);
            return;
        }
        char amt[40];
        format_money(l.amount, amt, sizeof amt);
        resp_ok(rs, "Loan %d APPROVED. %s has been credited to account %d.", l.id, amt, l.customer_id);
        log_transaction(c, "APPROVE_LOAN %d: %s credited to %d", l.id, amt, l.customer_id);
    } else if (rq->id2 == 2) {                            /* REJECT */
        if (strlen(remark) < 3) { resp_err(rs, "Please give a reason for the rejection."); return; }
        RejectCtx r = {c->uid, remark};
        int rc = db_update(T_LOANS, rq->id1, reject_cb, &r, err, sizeof err);
        if (rc == -1) { resp_err(rs, "Loan %d not found.", rq->id1); return; }
        if (rc != 0) { resp_err(rs, "%s", err); return; }
        resp_ok(rs, "Loan %d REJECTED. Reason recorded: %s", rq->id1, remark);
        log_transaction(c, "REJECT_LOAN %d (%s)", rq->id1, remark);
    } else {
        resp_err(rs, "Invalid decision.");
    }
}

/* UML: Bank::handle_employee_requests */
int handle_employee_requests(ClientCtx *c, const Request *rq, Response *rs)
{
    switch (rq->op) {
    case OP_ADD_CUSTOMER:         add_customer(c, rq, rs); return 1;
    case OP_GET_CUSTOMER:         op_customer_details(rq->id1, rs); return 1;
    case OP_MODIFY_CUSTOMER:      op_modify_customer(c, rq, rs); return 1;
    case OP_CLOSE_CUSTOMER:       delete_customer(c, rq, rs); return 1;
    case OP_VIEW_ASSIGNED_LOANS:  get_assigned_loans(c, rs); return 1;
    case OP_PROCESS_LOAN:         process_loan(c, rq, rs); return 1;
    case OP_DECIDE_LOAN:          update_loan_status(c, rq, rs); return 1;
    case OP_VIEW_PASSBOOK:        op_passbook(rq->id1, 60, rs); return 1;
    case OP_LIST_CUSTOMERS:       op_list_customers(rs); return 1;
    default:                      return 0;
    }
}
