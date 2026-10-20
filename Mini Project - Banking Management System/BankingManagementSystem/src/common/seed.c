/*
 * seed.c - Creates a fresh database with demo users (used by bank_init and by
 *          the server on first start).
 */
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "db.h"
#include "password.h"
#include "records.h"
#include "seed.h"
#include "util.h"

static void set_pass(char *salt, char *hash, const char *pw)
{
    make_salt(salt);
    hash_password(salt, pw, hash);
}

static int32_t add_txn(int32_t acct, int type, int64_t amount, int64_t bal, int32_t cp, int32_t by, const char *desc)
{
    TransactionRecord t;
    memset(&t, 0, sizeof t);
    t.account_id = acct;
    t.type = type;
    t.state = TXN_STATE_COMMITTED;
    t.amount = amount;
    t.balance_after = bal;
    t.counterparty = cp;
    t.performed_by = by;
    t.timestamp = (int64_t)time(NULL);
    str_copy(t.description, desc, sizeof t.description);
    int32_t id = 0;
    db_append(T_TXNS, &t, &id);
    return id;
}

static void add_customer(const char *first, const char *last, const char *phone,
                         const char *email, const char *pw, int64_t rupees)
{
    CustomerRecord c;
    memset(&c, 0, sizeof c);
    str_copy(c.first_name, first, sizeof c.first_name);
    str_copy(c.last_name, last, sizeof c.last_name);
    str_copy(c.phone, phone, sizeof c.phone);
    str_copy(c.email, email, sizeof c.email);
    set_pass(c.salt, c.pass_hash, pw);
    c.status = STATUS_ACTIVE;
    c.balance = rupees * 100;
    c.created_at = c.updated_at = (int64_t)time(NULL);
    int32_t id;
    db_append(T_CUSTOMERS, &c, &id);
    add_txn(id, TXN_OPENING, c.balance, c.balance, 0, 2002, "Account opening deposit");
}

static void add_employee(int role, const char *first, const char *last, const char *phone,
                         const char *email, const char *pw)
{
    EmployeeRecord e;
    memset(&e, 0, sizeof e);
    e.role = role;
    str_copy(e.first_name, first, sizeof e.first_name);
    str_copy(e.last_name, last, sizeof e.last_name);
    str_copy(e.phone, phone, sizeof e.phone);
    str_copy(e.email, email, sizeof e.email);
    set_pass(e.salt, e.pass_hash, pw);
    e.status = STATUS_ACTIVE;
    e.created_at = e.updated_at = (int64_t)time(NULL);
    db_append(T_EMPLOYEES, &e, NULL);
}

int seed_database(int verbose)
{
    /* wipe every table (O_TRUNC) and any leftover journal */
    char p[PATH_MAX];
    for (int t = 0; t < T_COUNT; t++) {
        db_path(db_table_file((Table)t), p, sizeof p);
        int fd = open(p, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0) { perror(p); return -1; }
        close(fd);
    }
    db_close_all();
    if (db_open_all() == -1) return -1;

    AdminRecord a;
    memset(&a, 0, sizeof a);
    str_copy(a.first_name, "System", sizeof a.first_name);
    str_copy(a.last_name, "Admin", sizeof a.last_name);
    set_pass(a.salt, a.pass_hash, "admin123");
    a.status = STATUS_ACTIVE;
    a.created_at = a.updated_at = (int64_t)time(NULL);
    db_append(T_ADMINS, &a, NULL);                                            /* 9001 */

    add_employee(ROLE_MANAGER,  "Rahul", "Mehta",  "9800000001", "rahul.mehta@bank.in",  "mgr123"); /* 2001 */
    add_employee(ROLE_EMPLOYEE, "Priya", "Sharma", "9800000002", "priya.sharma@bank.in", "emp123"); /* 2002 */
    add_employee(ROLE_EMPLOYEE, "Arjun", "Nair",   "9800000003", "arjun.nair@bank.in",   "emp123"); /* 2003 */

    add_customer("Shubham", "Singh",  "9721000001", "shubham@example.com", "cust123", 50000); /* 1001 */
    add_customer("Ananya",  "Iyer",   "9721000002", "ananya@example.com",  "cust123", 25000); /* 1002 */
    add_customer("Rohan",   "Das",    "9721000003", "rohan@example.com",   "cust123", 10000); /* 1003 */
    add_customer("Meera",   "Kapoor", "9721000004", "meera@example.com",   "cust123", 75000); /* 1004 */
    add_customer("Vikram",  "Rao",    "9721000005", "vikram@example.com",  "cust123", 5000);  /* 1005 */

    LoanRecord l;                                                             /* 5001 */
    memset(&l, 0, sizeof l);
    l.customer_id = 1002;
    l.amount = 200000LL * 100;
    l.tenure_months = 24;
    l.status = LOAN_PENDING;
    str_copy(l.purpose, "Home renovation", sizeof l.purpose);
    l.applied_at = l.updated_at = (int64_t)time(NULL);
    db_append(T_LOANS, &l, NULL);

    FeedbackRecord f;                                                         /* 1 */
    memset(&f, 0, sizeof f);
    f.customer_id = 1003;
    f.status = FEEDBACK_NEW;
    str_copy(f.message, "The mobile passbook feature is great, but please add SMS alerts.", sizeof f.message);
    f.created_at = (int64_t)time(NULL);
    db_append(T_FEEDBACK, &f, NULL);

    if (verbose) {
        printf("Database created in %s\n\n", db_data_dir());
        printf("  Role           ID     Password   Name\n");
        printf("  -------------  -----  ---------  ------------------\n");
        printf("  Administrator  9001   admin123   System Admin\n");
        printf("  Manager        2001   mgr123     Rahul Mehta\n");
        printf("  Employee       2002   emp123     Priya Sharma\n");
        printf("  Employee       2003   emp123     Arjun Nair\n");
        printf("  Customer       1001   cust123    Shubham Singh  (Rs. 50,000)\n");
        printf("  Customer       1002   cust123    Ananya Iyer    (Rs. 25,000)\n");
        printf("  Customer       1003   cust123    Rohan Das      (Rs. 10,000)\n");
        printf("  Customer       1004   cust123    Meera Kapoor   (Rs. 75,000)\n");
        printf("  Customer       1005   cust123    Vikram Rao     (Rs. 5,000)\n\n");
        printf("  Sample data: loan 5001 (PENDING, customer 1002), feedback #1 (customer 1003)\n");
    }
    return 0;
}
