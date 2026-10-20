/*
 * records.h - On-disk record layouts for the Banking Management System.
 *
 * Every "table" is a binary file of FIXED-SIZE records. Record number N lives at
 * byte offset N * sizeof(record), so a record can be reached directly with
 * lseek() and locked on its own with an fcntl() record lock covering exactly
 * those bytes (advisory / record locking, slides 44-51).
 *
 * IDs map to record numbers:  record_index = id - <TABLE>_ID_BASE
 * Money is stored as integer PAISE (1 rupee = 100 paise) so there are never
 * floating point rounding errors in balances.
 */
#ifndef RECORDS_H
#define RECORDS_H

#include <stdint.h>
#include <time.h>

/* ---------- ID bases (first ID handed out in each file) ---------- */
#define CUSTOMER_ID_BASE   1001
#define EMPLOYEE_ID_BASE   2001   /* employees AND managers live in one file */
#define ADMIN_ID_BASE      9001
#define LOAN_ID_BASE       5001
#define TXN_ID_BASE        1
#define FEEDBACK_ID_BASE   1

/* ---------- Roles ---------- */
typedef enum {
    ROLE_NONE = 0,
    ROLE_CUSTOMER = 1,
    ROLE_EMPLOYEE = 2,
    ROLE_MANAGER = 3,
    ROLE_ADMIN = 4
} Role;

/* ---------- Account / user status ---------- */
typedef enum {
    STATUS_ACTIVE = 1,
    STATUS_INACTIVE = 2,   /* deactivated by manager/admin - cannot log in   */
    STATUS_CLOSED = 3      /* closed by employee - permanent, cannot log in */
} UserStatus;

#define NAME_LEN   32
#define PHONE_LEN  16
#define EMAIL_LEN  48
#define SALT_LEN   17      /* 16 hex chars + NUL */
#define HASH_LEN   65      /* SHA-256 hex digest + NUL */

/* Customer == bank account (UML class "Customer") */
typedef struct {
    int32_t  id;                    /* account number, e.g. 1001 */
    char     first_name[NAME_LEN];
    char     last_name[NAME_LEN];
    char     phone[PHONE_LEN];
    char     email[EMAIL_LEN];
    char     salt[SALT_LEN];
    char     pass_hash[HASH_LEN];
    int32_t  status;                /* UserStatus */
    int64_t  balance;               /* paise */
    int64_t  loan_amount;           /* total approved loan amount, paise */
    int64_t  created_at;
    int64_t  updated_at;
} CustomerRecord;

/* Employee / Manager (UML classes "Employee" and "Manager") */
typedef struct {
    int32_t  id;                    /* e.g. 2001 */
    int32_t  role;                  /* ROLE_EMPLOYEE or ROLE_MANAGER */
    char     first_name[NAME_LEN];
    char     last_name[NAME_LEN];
    char     phone[PHONE_LEN];
    char     email[EMAIL_LEN];
    char     salt[SALT_LEN];
    char     pass_hash[HASH_LEN];
    int32_t  status;                /* UserStatus */
    int64_t  created_at;
    int64_t  updated_at;
} EmployeeRecord;

/* Administrator (UML class "Admin") */
typedef struct {
    int32_t  id;                    /* e.g. 9001 */
    char     first_name[NAME_LEN];
    char     last_name[NAME_LEN];
    char     salt[SALT_LEN];
    char     pass_hash[HASH_LEN];
    int32_t  status;
    int64_t  created_at;
    int64_t  updated_at;
} AdminRecord;

/* ---------- Loans (UML class "Loan") ---------- */
typedef enum {
    LOAN_PENDING = 1,       /* applied by customer, waiting for manager      */
    LOAN_ASSIGNED = 2,      /* manager assigned it to an employee            */
    LOAN_UNDER_REVIEW = 3,  /* employee started processing it                */
    LOAN_APPROVED = 4,      /* amount credited to the customer's account     */
    LOAN_REJECTED = 5
} LoanStatus;

#define PURPOSE_LEN 64
#define REMARK_LEN  96

typedef struct {
    int32_t  id;                    /* e.g. 5001 */
    int32_t  customer_id;
    int64_t  amount;                /* paise */
    int32_t  tenure_months;
    int32_t  status;                /* LoanStatus */
    int32_t  assigned_to;           /* employee id, 0 = unassigned */
    int32_t  assigned_by;           /* manager id */
    int32_t  decided_by;            /* employee who approved/rejected */
    char     purpose[PURPOSE_LEN];
    char     remark[REMARK_LEN];
    int64_t  applied_at;
    int64_t  updated_at;
} LoanRecord;

/* ---------- Transactions (UML class "Transaction") ---------- */
typedef enum {
    TXN_DEPOSIT = 1,
    TXN_WITHDRAW = 2,
    TXN_TRANSFER_OUT = 3,
    TXN_TRANSFER_IN = 4,
    TXN_LOAN_CREDIT = 5,
    TXN_OPENING = 6         /* opening deposit when the account was created */
} TxnType;

/* A transaction record is appended as PENDING and only flipped to COMMITTED
 * inside the journaled (atomic) operation. If the operation is rolled back the
 * record stays PENDING and is never shown - see journal.c. */
typedef enum {
    TXN_STATE_PENDING = 0,
    TXN_STATE_COMMITTED = 1
} TxnState;

#define TXN_DESC_LEN 64

typedef struct {
    int32_t  id;
    int32_t  account_id;            /* customer id */
    int32_t  type;                  /* TxnType */
    int32_t  state;                 /* TxnState */
    int32_t  counterparty;          /* other account for transfers, loan id for loan credit */
    int32_t  performed_by;          /* user id who caused it */
    int64_t  amount;                /* paise, always positive */
    int64_t  balance_after;         /* paise */
    int64_t  timestamp;
    char     description[TXN_DESC_LEN];
} TransactionRecord;

/* ---------- Feedback (UML class "Feedback") ---------- */
typedef enum {
    FEEDBACK_NEW = 1,
    FEEDBACK_REVIEWED = 2
} FeedbackStatus;

#define FEEDBACK_LEN 256
#define FEEDBACK_NOTE_LEN 128

typedef struct {
    int32_t  id;
    int32_t  customer_id;
    int32_t  status;                /* FeedbackStatus */
    int32_t  reviewed_by;           /* manager id */
    char     message[FEEDBACK_LEN];
    char     manager_note[FEEDBACK_NOTE_LEN];
    int64_t  created_at;
    int64_t  reviewed_at;
} FeedbackRecord;

#endif
