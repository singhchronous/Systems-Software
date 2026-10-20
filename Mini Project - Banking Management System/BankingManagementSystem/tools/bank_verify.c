/*
 * bank_verify.c - Consistency checker ("C" of ACID).
 *
 * Takes SHARED (read) locks on the loans, customers and transactions tables
 * so it sees a consistent snapshot even while the server is running, then
 * checks every invariant the bank must always satisfy:
 *
 *   1. every account's balance == sum of its COMMITTED transactions
 *   2. no balance is negative
 *   3. every TRANSFER_OUT has a matching TRANSFER_IN (money is conserved)
 *   4. every APPROVED loan was credited exactly once, and the customer's
 *      loan total equals the sum of their approved loans
 *   5. no unrecovered journal is left behind
 *
 * Usage: ./bank_verify [-d datadir] [-v]
 */
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "journal.h"
#include "records.h"
#include "util.h"

int main(int argc, char *argv[])
{
    const char *dir = "data";
    int verbose = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d") && i + 1 < argc) dir = argv[++i];
        else if (!strcmp(argv[i], "-v")) verbose = 1;
        else { printf("Usage: %s [-d datadir] [-v]\n", argv[0]); return 2; }
    }
    if (db_init_dir(dir) == -1 || db_open_all() == -1) return 2;
    if (!db_files_exist()) { printf("No database found in %s\n", db_data_dir()); return 2; }

    /* consistent snapshot: same lock order as the server */
    gate_shared();
    db_lock_table(T_LOANS, F_RDLCK);
    db_lock_table(T_CUSTOMERS, F_RDLCK);
    db_lock_table(T_TXNS, F_RDLCK);

    int nc = db_count(T_CUSTOMERS), nt = db_count(T_TXNS), nl = db_count(T_LOANS);
    CustomerRecord *cust = calloc((size_t)nc + 1, sizeof *cust);
    int64_t *ledger = calloc((size_t)nc + 1, sizeof *ledger);
    int64_t *loan_sum = calloc((size_t)nc + 1, sizeof *loan_sum);
    int *loan_credits = calloc((size_t)nl + 1, sizeof *loan_credits);
    if (!cust || !ledger || !loan_sum || !loan_credits) { perror("calloc"); return 2; }

    for (int i = 0; i < nc; i++) db_read_raw(T_CUSTOMERS, CUSTOMER_ID_BASE + i, &cust[i]);

    int failures = 0, pending = 0, committed = 0;
    int64_t out_sum = 0, in_sum = 0;
    int out_cnt = 0, in_cnt = 0;
    int64_t deposits = 0, withdrawals = 0, loans_paid = 0;

    for (int i = 0; i < nt; i++) {
        TransactionRecord t;
        if (db_read_raw(T_TXNS, TXN_ID_BASE + i, &t) == -1) break;
        if (t.state != TXN_STATE_COMMITTED) { pending++; continue; }
        committed++;
        int idx = t.account_id - CUSTOMER_ID_BASE;
        if (idx < 0 || idx >= nc) {
            printf("FAIL: transaction %d refers to unknown account %d\n", t.id, t.account_id);
            failures++;
            continue;
        }
        switch (t.type) {
        case TXN_DEPOSIT: case TXN_OPENING:
            ledger[idx] += t.amount; deposits += t.amount; break;
        case TXN_WITHDRAW:
            ledger[idx] -= t.amount; withdrawals += t.amount; break;
        case TXN_TRANSFER_OUT:
            ledger[idx] -= t.amount; out_sum += t.amount; out_cnt++; break;
        case TXN_TRANSFER_IN:
            ledger[idx] += t.amount; in_sum += t.amount; in_cnt++; break;
        case TXN_LOAN_CREDIT:
            ledger[idx] += t.amount; loans_paid += t.amount;
            if (t.counterparty >= LOAN_ID_BASE && t.counterparty - LOAN_ID_BASE < nl)
                loan_credits[t.counterparty - LOAN_ID_BASE]++;
            break;
        }
    }

    printf("Banking Management System - consistency check (%s)\n", db_data_dir());
    printf("------------------------------------------------------------------\n");
    printf("Accounts: %d   Transactions: %d committed, %d pending/rolled-back   Loans: %d\n\n",
           nc, committed, pending, nl);

    /* 1 + 2: balance vs ledger, non-negative */
    int64_t total = 0;
    int bad_bal = 0;
    for (int i = 0; i < nc; i++) {
        total += cust[i].balance;
        if (cust[i].balance != ledger[i]) {
            char b[40], l[40];
            format_money(cust[i].balance, b, sizeof b);
            format_money(ledger[i], l, sizeof l);
            printf("FAIL: account %d balance %s but its transactions add up to %s\n", cust[i].id, b, l);
            bad_bal++;
        } else if (verbose) {
            char b[40];
            format_money(cust[i].balance, b, sizeof b);
            printf("  ok  account %d  %s\n", cust[i].id, b);
        }
        if (cust[i].balance < 0) {
            printf("FAIL: account %d has a NEGATIVE balance\n", cust[i].id);
            bad_bal++;
        }
    }
    printf("[%s] 1-2. Every balance matches its transaction history and is >= 0\n", bad_bal ? "FAIL" : "PASS");
    failures += bad_bal;

    /* 3: transfers conserve money */
    int transfer_ok = (out_cnt == in_cnt && out_sum == in_sum);
    char o[40], in[40];
    format_money(out_sum, o, sizeof o);
    format_money(in_sum, in, sizeof in);
    printf("[%s] 3.   Transfers: %d out (%s) / %d in (%s)\n", transfer_ok ? "PASS" : "FAIL", out_cnt, o, in_cnt, in);
    if (!transfer_ok) failures++;

    /* 4: loans */
    int bad_loans = 0;
    for (int i = 0; i < nl; i++) {
        LoanRecord l;
        db_read_raw(T_LOANS, LOAN_ID_BASE + i, &l);
        int idx = l.customer_id - CUSTOMER_ID_BASE;
        if (l.status == LOAN_APPROVED) {
            if (idx >= 0 && idx < nc) loan_sum[idx] += l.amount;
            if (loan_credits[i] != 1) {
                printf("FAIL: approved loan %d credited %d time(s)\n", l.id, loan_credits[i]);
                bad_loans++;
            }
        } else if (loan_credits[i] != 0) {
            printf("FAIL: loan %d is %s but money was credited\n", l.id, loan_status_name(l.status));
            bad_loans++;
        }
    }
    for (int i = 0; i < nc; i++)
        if (cust[i].loan_amount != loan_sum[i]) {
            printf("FAIL: account %d loan total does not match its approved loans\n", cust[i].id);
            bad_loans++;
        }
    printf("[%s] 4.   Every approved loan credited exactly once\n", bad_loans ? "FAIL" : "PASS");
    failures += bad_loans;

    db_unlock_table(T_TXNS);
    db_unlock_table(T_CUSTOMERS);
    db_unlock_table(T_LOANS);
    gate_release();

    /* 5: journals */
    int orphans = journal_orphans_exist();
    printf("[%s] 5.   No unrecovered crash journal%s\n", orphans ? "WARN" : "PASS",
           orphans ? " (start the server to roll it back)" : "");

    char t[40], d[40], w[40], lp[40];
    format_money(total, t, sizeof t);
    format_money(deposits, d, sizeof d);
    format_money(withdrawals, w, sizeof w);
    format_money(loans_paid, lp, sizeof lp);
    printf("\nTotal money held by the bank: %s\n", t);
    printf("  = deposits %s - withdrawals %s + loans disbursed %s\n", d, w, lp);
    printf("------------------------------------------------------------------\n");
    printf("RESULT: %s\n", failures ? "INCONSISTENT - see FAIL lines above" : "CONSISTENT - all invariants hold");
    return failures ? 1 : 0;
}
