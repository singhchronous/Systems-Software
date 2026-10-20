/*
 * ledger.c - Every operation that moves money. This is where ACID is enforced.
 *
 *  A - Atomicity  : all records of one operation go through journal_commit()
 *                   (write-ahead undo journal), so a crash rolls them back.
 *  C - Consistency: checks before writing - account must exist and be ACTIVE,
 *                   balance can never go negative, amount limits, a loan can be
 *                   approved only once; every balance change has a matching
 *                   transaction record (verified by tools/bank_verify).
 *  I - Isolation  : fcntl() write locks on exactly the records involved, taken
 *                   in a fixed order (lower account id first) -> no deadlock.
 *  D - Durability : fsync() of journal and data files before we answer "OK".
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "db.h"
#include "journal.h"
#include "locking.h"
#include "server.h"
#include "util.h"

#define RETRIES 5

static void set_err(char *err, size_t cap, const char *msg) { str_copy(err, msg, cap); }

static int check_account_usable(const CustomerRecord *c, const char *who, char *err, size_t cap)
{
    if (c->status == STATUS_CLOSED) { snprintf(err, cap, "%s account %d is CLOSED.", who, c->id); return -1; }
    if (c->status != STATUS_ACTIVE) { snprintf(err, cap, "%s account %d is DEACTIVATED.", who, c->id); return -1; }
    return 0;
}

/* Build a PENDING transaction record and append it. The record only becomes
 * visible (COMMITTED) through the journal. */
static int append_pending_txn(int32_t acct, int type, int64_t amount, int64_t bal_after,
                              int32_t counterparty, int32_t by, const char *desc,
                              TransactionRecord *out)
{
    memset(out, 0, sizeof *out);
    out->account_id = acct;
    out->type = type;
    out->state = TXN_STATE_PENDING;
    out->amount = amount;
    out->balance_after = bal_after;
    out->counterparty = counterparty;
    out->performed_by = by;
    out->timestamp = (int64_t)time(NULL);
    str_copy(out->description, desc, sizeof out->description);
    int32_t id;
    if (db_append(T_TXNS, out, &id) == -1) return -1;   /* log_transaction slot */
    out->id = id;
    return 0;
}

/* --------------------------------------------- deposit / withdraw (one account) */

static int post_single(int32_t acct, int type, int64_t amount, int32_t by,
                       const char *desc, int64_t *new_bal, char *err, size_t cap)
{
    if (amount <= 0) { set_err(err, cap, "Amount must be greater than zero."); return -1; }
    if (amount > MAX_TXN_AMOUNT) { set_err(err, cap, "Amount exceeds the per-transaction limit (Rs. 10 crore)."); return -1; }

    for (int attempt = 0; attempt < RETRIES; attempt++) {
        if (!db_valid_id(T_CUSTOMERS, acct)) { set_err(err, cap, "Account not found."); return -1; }
        gate_shared();
        db_lock(T_CUSTOMERS, acct, F_WRLCK);                 /* I: exclusive lock on this account */
        if (journal_orphans_exist()) {                       /* a crashed op must be undone first */
            db_unlock(T_CUSTOMERS, acct);
            gate_release();
            journal_recover(0);
            continue;
        }

        int rc = -1;
        CustomerRecord before, after;
        TransactionRecord tpending, tcommitted;
        if (db_read_raw(T_CUSTOMERS, acct, &before) == -1) {
            set_err(err, cap, "Account not found.");
            goto done;
        }
        if (check_account_usable(&before, "Your", err, cap) == -1) goto done;

        race_window();                                       /* demo only (--race-delay) */

        after = before;
        if (type == TXN_WITHDRAW) {
            if (before.balance < amount) {                   /* C: never negative */
                char b[40];
                format_money(before.balance, b, sizeof b);
                snprintf(err, cap, "Insufficient balance. Available: %s", b);
                goto done;
            }
            after.balance -= amount;
        } else {
            after.balance += amount;
        }
        after.updated_at = (int64_t)time(NULL);

        if (append_pending_txn(acct, type, amount, after.balance, 0, by, desc, &tpending) == -1) {
            set_err(err, cap, "Could not write transaction log.");
            goto done;
        }
        db_lock(T_TXNS, tpending.id, F_WRLCK);
        tcommitted = tpending;
        tcommitted.state = TXN_STATE_COMMITTED;

        Journal j;
        char jdesc[96];
        snprintf(jdesc, sizeof jdesc, "%s acct %d amount %lld", txn_type_name(type), acct, (long long)amount);
        journal_begin(&j, jdesc);
        journal_add(&j, T_CUSTOMERS, acct, &before, &after);
        journal_add(&j, T_TXNS, tpending.id, &tpending, &tcommitted);
        if (journal_commit(&j) == 0) {                       /* A + D */
            rc = 0;
            if (new_bal) *new_bal = after.balance;
        } else {
            set_err(err, cap, "Operation failed and was rolled back.");
        }
        db_unlock(T_TXNS, tpending.id);
    done:
        db_unlock(T_CUSTOMERS, acct);
        gate_release();
        if (rc == -1 && journal_orphans_exist()) journal_recover(0);
        return rc;
    }
    set_err(err, cap, "System busy, please try again.");
    return -1;
}

int ledger_deposit(int32_t acct, int64_t amount, int32_t by, int64_t *new_bal, char *err, size_t cap)
{
    return post_single(acct, TXN_DEPOSIT, amount, by, "Cash deposit", new_bal, err, cap);
}

int ledger_withdraw(int32_t acct, int64_t amount, int32_t by, int64_t *new_bal, char *err, size_t cap)
{
    return post_single(acct, TXN_WITHDRAW, amount, by, "Cash withdrawal", new_bal, err, cap);
}

/* ----------------------------------------------------------------- transfer */

int ledger_transfer(int32_t from, int32_t to, int64_t amount, int32_t by,
                    int64_t *new_bal, char *err, size_t cap)
{
    if (amount <= 0) { set_err(err, cap, "Amount must be greater than zero."); return -1; }
    if (amount > MAX_TXN_AMOUNT) { set_err(err, cap, "Amount exceeds the per-transaction limit (Rs. 10 crore)."); return -1; }
    if (from == to) { set_err(err, cap, "Cannot transfer to your own account."); return -1; }

    /* Deadlock prevention: ALWAYS lock the lower account number first. Two
     * opposite transfers (A->B and B->A) then queue on the same first lock
     * instead of each holding one lock and waiting for the other. */
    int32_t first = from < to ? from : to;
    int32_t second = from < to ? to : from;

    for (int attempt = 0; attempt < RETRIES; attempt++) {
        if (!db_valid_id(T_CUSTOMERS, from)) { set_err(err, cap, "Your account was not found."); return -1; }
        if (!db_valid_id(T_CUSTOMERS, to)) {
            snprintf(err, cap, "Target account %d does not exist. No money was moved.", to);
            return -1;
        }
        gate_shared();
        db_lock(T_CUSTOMERS, first, F_WRLCK);
        db_lock(T_CUSTOMERS, second, F_WRLCK);
        if (journal_orphans_exist()) {
            db_unlock(T_CUSTOMERS, second);
            db_unlock(T_CUSTOMERS, first);
            gate_release();
            journal_recover(0);
            continue;
        }

        int rc = -1;
        CustomerRecord src_b, src_a, dst_b, dst_a;
        TransactionRecord out_p, out_c, in_p, in_c;
        if (db_read_raw(T_CUSTOMERS, from, &src_b) == -1 || db_read_raw(T_CUSTOMERS, to, &dst_b) == -1) {
            set_err(err, cap, "Account not found.");
            goto done;
        }
        if (check_account_usable(&src_b, "Your", err, cap) == -1) goto done;
        if (check_account_usable(&dst_b, "Target", err, cap) == -1) goto done;
        if (src_b.balance < amount) {
            char b[40];
            format_money(src_b.balance, b, sizeof b);
            snprintf(err, cap, "Insufficient balance. Available: %s", b);
            goto done;
        }

        race_window();

        src_a = src_b;
        dst_a = dst_b;
        src_a.balance -= amount;
        dst_a.balance += amount;
        src_a.updated_at = dst_a.updated_at = (int64_t)time(NULL);

        char d1[TXN_DESC_LEN], d2[TXN_DESC_LEN];
        snprintf(d1, sizeof d1, "Transfer to A/c %d (%s %s)", to, dst_b.first_name, dst_b.last_name);
        snprintf(d2, sizeof d2, "Transfer from A/c %d (%s %s)", from, src_b.first_name, src_b.last_name);
        if (append_pending_txn(from, TXN_TRANSFER_OUT, amount, src_a.balance, to, by, d1, &out_p) == -1 ||
            append_pending_txn(to, TXN_TRANSFER_IN, amount, dst_a.balance, from, by, d2, &in_p) == -1) {
            set_err(err, cap, "Could not write transaction log.");
            goto done;
        }
        db_lock(T_TXNS, out_p.id, F_WRLCK);
        db_lock(T_TXNS, in_p.id, F_WRLCK);
        out_c = out_p; out_c.state = TXN_STATE_COMMITTED;
        in_c = in_p;   in_c.state = TXN_STATE_COMMITTED;

        Journal j;
        char jdesc[96];
        snprintf(jdesc, sizeof jdesc, "TRANSFER %d -> %d amount %lld", from, to, (long long)amount);
        journal_begin(&j, jdesc);
        journal_add(&j, T_CUSTOMERS, from, &src_b, &src_a);   /* debit  */
        journal_add(&j, T_CUSTOMERS, to, &dst_b, &dst_a);     /* credit */
        journal_add(&j, T_TXNS, out_p.id, &out_p, &out_c);
        journal_add(&j, T_TXNS, in_p.id, &in_p, &in_c);
        if (journal_commit(&j) == 0) {
            rc = 0;
            if (new_bal) *new_bal = src_a.balance;
        } else {
            set_err(err, cap, "Transfer failed and was rolled back.");
        }
        db_unlock(T_TXNS, in_p.id);
        db_unlock(T_TXNS, out_p.id);
    done:
        db_unlock(T_CUSTOMERS, second);
        db_unlock(T_CUSTOMERS, first);
        gate_release();
        if (rc == -1 && journal_orphans_exist()) journal_recover(0);
        return rc;
    }
    set_err(err, cap, "System busy, please try again.");
    return -1;
}

/* ------------------------------------------------------------ open account */

int ledger_open_account(CustomerRecord *c, int64_t opening, int32_t by,
                        int32_t *new_id, char *err, size_t cap)
{
    if (opening < 0 || opening > MAX_TXN_AMOUNT) { set_err(err, cap, "Invalid opening deposit."); return -1; }
    c->balance = 0;
    c->loan_amount = 0;
    c->status = STATUS_ACTIVE;
    c->created_at = c->updated_at = (int64_t)time(NULL);
    int32_t id;
    if (db_append(T_CUSTOMERS, c, &id) == -1) {           /* new account number */
        set_err(err, cap, "Could not create the account record.");
        return -1;
    }
    c->id = id;
    if (new_id) *new_id = id;
    if (opening > 0) {
        int64_t bal;
        if (post_single(id, TXN_OPENING, opening, by, "Account opening deposit", &bal, err, cap) == -1)
            return -2;                                      /* account exists, deposit failed */
        c->balance = bal;
    }
    return 0;
}

/* ------------------------------------------------------------ loan approval */

int ledger_approve_loan(int32_t loan_id, int32_t emp_id, const char *remark,
                        LoanRecord *out, char *err, size_t cap)
{
    for (int attempt = 0; attempt < RETRIES; attempt++) {
        if (!db_valid_id(T_LOANS, loan_id)) { set_err(err, cap, "Loan not found."); return -1; }
        gate_shared();
        /* lock order: loans before customers */
        db_lock(T_LOANS, loan_id, F_WRLCK);
        LoanRecord lb;
        if (db_read_raw(T_LOANS, loan_id, &lb) == -1) {
            db_unlock(T_LOANS, loan_id);
            gate_release();
            set_err(err, cap, "Loan not found.");
            return -1;
        }
        int32_t cust = lb.customer_id;
        db_lock(T_CUSTOMERS, cust, F_WRLCK);
        if (journal_orphans_exist()) {
            db_unlock(T_CUSTOMERS, cust);
            db_unlock(T_LOANS, loan_id);
            gate_release();
            journal_recover(0);
            continue;
        }
        db_read_raw(T_LOANS, loan_id, &lb);                  /* re-read under lock */

        int rc = -1;
        CustomerRecord cb, ca;
        LoanRecord la;
        TransactionRecord tp, tc;
        if (lb.assigned_to != emp_id) { set_err(err, cap, "This loan is not assigned to you."); goto done; }
        if (lb.status == LOAN_APPROVED || lb.status == LOAN_REJECTED) {
            snprintf(err, cap, "Loan %d was already %s.", loan_id, loan_status_name(lb.status));
            goto done;
        }
        if (lb.status != LOAN_UNDER_REVIEW) {
            set_err(err, cap, "Process the application first (menu: Process Loan Application).");
            goto done;
        }
        if (db_read_raw(T_CUSTOMERS, cust, &cb) == -1) { set_err(err, cap, "Customer not found."); goto done; }
        if (cb.status == STATUS_CLOSED) { set_err(err, cap, "Customer account is closed - reject this loan instead."); goto done; }

        race_window();

        ca = cb;
        ca.balance += lb.amount;
        ca.loan_amount += lb.amount;
        ca.updated_at = (int64_t)time(NULL);
        la = lb;
        la.status = LOAN_APPROVED;
        la.decided_by = emp_id;
        la.updated_at = ca.updated_at;
        str_copy(la.remark, remark && *remark ? remark : "Approved", sizeof la.remark);

        char d[TXN_DESC_LEN];
        snprintf(d, sizeof d, "Loan %d disbursed", loan_id);
        if (append_pending_txn(cust, TXN_LOAN_CREDIT, lb.amount, ca.balance, loan_id, emp_id, d, &tp) == -1) {
            set_err(err, cap, "Could not write transaction log.");
            goto done;
        }
        db_lock(T_TXNS, tp.id, F_WRLCK);
        tc = tp;
        tc.state = TXN_STATE_COMMITTED;

        Journal j;
        char jdesc[96];
        snprintf(jdesc, sizeof jdesc, "LOAN_APPROVE loan %d acct %d amount %lld", loan_id, cust, (long long)lb.amount);
        journal_begin(&j, jdesc);
        journal_add(&j, T_LOANS, loan_id, &lb, &la);
        journal_add(&j, T_CUSTOMERS, cust, &cb, &ca);
        journal_add(&j, T_TXNS, tp.id, &tp, &tc);
        if (journal_commit(&j) == 0) {
            rc = 0;
            if (out) *out = la;
        } else {
            set_err(err, cap, "Approval failed and was rolled back.");
        }
        db_unlock(T_TXNS, tp.id);
    done:
        db_unlock(T_CUSTOMERS, cust);
        db_unlock(T_LOANS, loan_id);
        gate_release();
        if (rc == -1 && journal_orphans_exist()) journal_recover(0);
        return rc;
    }
    set_err(err, cap, "System busy, please try again.");
    return -1;
}
