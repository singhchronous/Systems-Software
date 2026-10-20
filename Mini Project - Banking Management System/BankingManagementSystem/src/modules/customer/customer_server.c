/*
 * customer_server.c - CUSTOMER module, server side (UML class "Customer").
 *
 *   View Account Balance     get_balance()
 *   Deposit Money            deposit_money()
 *   Withdraw Money           withdraw_money()
 *   Transfer Funds           transfer_funds()
 *   Apply for a Loan         apply_for_loan()
 *   View My Loans            (extra: track application status)
 *   Change Password          handled in auth.c (change_customer_password)
 *   Adding Feedback          add_feedback()
 *   View Transaction History view_transaction_history()
 *   Login / Logout / Exit    handled in server.c / auth.c
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "db.h"
#include "server.h"
#include "util.h"

static void get_balance(ClientCtx *c, Response *rs)
{
    CustomerRecord r;
    if (db_get(T_CUSTOMERS, c->uid, &r) == -1) {     /* read lock on this record */
        resp_err(rs, "Account not found.");
        return;
    }
    char bal[40], loan[40];
    format_money(r.balance, bal, sizeof bal);
    format_money(r.loan_amount, loan, sizeof loan);
    rs->value = r.balance;
    resp_ok(rs, "Account number : %d\nAccount holder : %s %s\nAvailable balance: %s\nLoans approved   : %s",
            r.id, r.first_name, r.last_name, bal, loan);
}

static void deposit_money(ClientCtx *c, const Request *rq, Response *rs)
{
    char err[160], amt[40], bal[40];
    int64_t new_bal;
    if (ledger_deposit(c->uid, rq->amount, c->uid, &new_bal, err, sizeof err) == -1) {
        resp_err(rs, "%s", err);
        return;
    }
    format_money(rq->amount, amt, sizeof amt);
    format_money(new_bal, bal, sizeof bal);
    rs->value = new_bal;
    resp_ok(rs, "Deposited %s.\nNew balance: %s", amt, bal);
    log_transaction(c, "DEPOSIT %s -> balance %s", amt, bal);
}

static void withdraw_money(ClientCtx *c, const Request *rq, Response *rs)
{
    char err[160], amt[40], bal[40];
    int64_t new_bal;
    if (ledger_withdraw(c->uid, rq->amount, c->uid, &new_bal, err, sizeof err) == -1) {
        resp_err(rs, "%s", err);
        return;
    }
    format_money(rq->amount, amt, sizeof amt);
    format_money(new_bal, bal, sizeof bal);
    rs->value = new_bal;
    resp_ok(rs, "Withdrawn %s.\nNew balance: %s", amt, bal);
    log_transaction(c, "WITHDRAW %s -> balance %s", amt, bal);
}

static void transfer_funds(ClientCtx *c, const Request *rq, Response *rs)
{
    char err[160], amt[40], bal[40];
    int64_t new_bal;
    if (ledger_transfer(c->uid, rq->id1, rq->amount, c->uid, &new_bal, err, sizeof err) == -1) {
        resp_err(rs, "%s", err);
        return;
    }
    format_money(rq->amount, amt, sizeof amt);
    format_money(new_bal, bal, sizeof bal);
    rs->value = new_bal;
    resp_ok(rs, "Transferred %s to account %d.\nNew balance: %s", amt, rq->id1, bal);
    log_transaction(c, "TRANSFER %s to %d -> balance %s", amt, rq->id1, bal);
}

static void apply_for_loan(ClientCtx *c, const Request *rq, Response *rs)
{
    if (rq->amount <= 0 || rq->amount > MAX_LOAN_AMOUNT) {
        resp_err(rs, "Loan amount must be between Rs. 0.01 and Rs. 1,00,00,000.");
        return;
    }
    if (rq->id1 < 1 || rq->id1 > 360) {
        resp_err(rs, "Tenure must be between 1 and 360 months.");
        return;
    }
    char purpose[PURPOSE_LEN];
    str_copy(purpose, rq->s1, sizeof purpose);
    str_trim(purpose);
    if (strlen(purpose) < 3) {
        resp_err(rs, "Please enter the purpose of the loan (at least 3 characters).");
        return;
    }
    LoanRecord l;
    memset(&l, 0, sizeof l);
    l.customer_id = c->uid;
    l.amount = rq->amount;
    l.tenure_months = rq->id1;
    l.status = LOAN_PENDING;
    str_copy(l.purpose, purpose, sizeof l.purpose);
    l.applied_at = l.updated_at = (int64_t)time(NULL);
    int32_t id;
    if (db_append(T_LOANS, &l, &id) == -1) {          /* append lock + fsync */
        resp_err(rs, "Could not submit the application. Please try again.");
        return;
    }
    char amt[40];
    format_money(l.amount, amt, sizeof amt);
    rs->value = id;
    resp_ok(rs, "Loan application %d submitted for %s (%d months).\n"
                "Status: PENDING - a manager will assign it to an employee for processing.",
            id, amt, l.tenure_months);
    log_transaction(c, "APPLY_LOAN %d for %s", id, amt);
}

typedef struct {
    int32_t cust;
    Response *rs;
    int n;
} MyLoansCtx;

static int my_loans_cb(const void *rec, void *ctx)
{
    const LoanRecord *l = rec;
    MyLoansCtx *m = ctx;
    if (l->customer_id != m->cust) return 0;
    char amt[40], when[32];
    format_money(l->amount, amt, sizeof amt);
    format_time(l->applied_at, when, sizeof when);
    resp_append(m->rs, "%-6d %-19s %18s %5d  %-12s %-20.20s %s\n", l->id, when, amt, l->tenure_months,
                loan_status_name(l->status), l->purpose, l->remark);
    m->n++;
    return 0;
}

static void view_my_loans(ClientCtx *c, Response *rs)
{
    resp_ok(rs, "%-6s %-19s %18s %5s  %-12s %-20s %s\n", "Loan", "Applied on", "Amount", "Months", "Status", "Purpose", "Remark");
    resp_append(rs, "----------------------------------------------------------------------------------------------\n");
    MyLoansCtx m = {c->uid, rs, 0};
    db_scan(T_LOANS, my_loans_cb, &m);
    if (m.n == 0) resp_append(rs, "(you have not applied for any loan)\n");
}

static void add_feedback(ClientCtx *c, const Request *rq, Response *rs)
{
    char msg[FEEDBACK_LEN];
    str_copy(msg, rq->text, sizeof msg);
    str_trim(msg);
    if (strlen(msg) < 5) {
        resp_err(rs, "Feedback is too short (minimum 5 characters).");
        return;
    }
    FeedbackRecord f;
    memset(&f, 0, sizeof f);
    f.customer_id = c->uid;
    f.status = FEEDBACK_NEW;
    str_copy(f.message, msg, sizeof f.message);
    f.created_at = (int64_t)time(NULL);
    int32_t id;
    if (db_append(T_FEEDBACK, &f, &id) == -1) {
        resp_err(rs, "Could not save feedback.");
        return;
    }
    resp_ok(rs, "Thank you! Feedback #%d has been recorded and will be reviewed by a manager.", id);
    log_transaction(c, "ADD_FEEDBACK #%d", id);
}

/* UML: Bank::handle_customer_requests */
int handle_customer_requests(ClientCtx *c, const Request *rq, Response *rs)
{
    switch (rq->op) {
    case OP_VIEW_BALANCE:  get_balance(c, rs); return 1;
    case OP_DEPOSIT:       deposit_money(c, rq, rs); return 1;
    case OP_WITHDRAW:      withdraw_money(c, rq, rs); return 1;
    case OP_TRANSFER:      transfer_funds(c, rq, rs); return 1;
    case OP_APPLY_LOAN:    apply_for_loan(c, rq, rs); return 1;
    case OP_VIEW_MY_LOANS: view_my_loans(c, rs); return 1;
    case OP_ADD_FEEDBACK:  add_feedback(c, rq, rs); return 1;
    case OP_VIEW_HISTORY:  op_passbook(c->uid, 30, rs); return 1;
    default:               return 0;
    }
}
