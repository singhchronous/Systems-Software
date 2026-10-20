/*
 * protocol.h - Client <-> Server message format.
 *
 * The client keeps ONE TCP connection open for its whole session. Every request
 * is a fixed-size Request struct and every reply is a fixed-size Response
 * struct, written/read completely with writen()/readn() (net.c). Using fixed
 * binary structs (instead of "a|b|c" strings) means user input such as a
 * feedback message containing '|' or quotes can never break the protocol.
 *
 * Security: the client NEVER sends "who it is" with a request. The server child
 * process remembers who logged in on this connection and checks the role for
 * every operation, so a client cannot impersonate another user.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define DEFAULT_PORT 8080
#define FIELD_LEN    64
#define TEXT_LEN     256
#define RESP_TEXT_LEN 12288

typedef enum {
    /* ---- common to every role ---- */
    OP_LOGIN = 1,            /* id1 = role, id2 = user id, s1 = password          */
    OP_LOGOUT,               /* end session, keep connection (back to login menu) */
    OP_EXIT,                 /* end session and close connection                  */
    OP_CHANGE_PASSWORD,      /* s1 = old password, s2 = new password              */

    /* ---- customer ---- */
    OP_VIEW_BALANCE = 20,
    OP_DEPOSIT,              /* amount                                            */
    OP_WITHDRAW,             /* amount                                            */
    OP_TRANSFER,             /* id1 = target account, amount                      */
    OP_APPLY_LOAN,           /* amount, id1 = tenure months, s1 = purpose         */
    OP_VIEW_MY_LOANS,
    OP_ADD_FEEDBACK,         /* text                                              */
    OP_VIEW_HISTORY,

    /* ---- employee ---- */
    OP_ADD_CUSTOMER = 40,    /* s1 first, s2 last, s3 phone, s4 email, text pass, amount = opening deposit */
    OP_GET_CUSTOMER,         /* id1 = customer id                                 */
    OP_MODIFY_CUSTOMER,      /* id1 = customer id, id2 = field, s1 = new value    */
    OP_CLOSE_CUSTOMER,       /* id1 = customer id                                 */
    OP_VIEW_ASSIGNED_LOANS,
    OP_PROCESS_LOAN,         /* id1 = loan id  -> moves ASSIGNED -> UNDER_REVIEW  */
    OP_DECIDE_LOAN,          /* id1 = loan id, id2 = 1 approve / 2 reject, text = remark */
    OP_VIEW_PASSBOOK,        /* id1 = customer id                                 */
    OP_LIST_CUSTOMERS,       /* employee / manager / admin                        */

    /* ---- manager ---- */
    OP_SET_CUSTOMER_STATUS = 60, /* id1 = customer id, id2 = 1 activate / 2 deactivate */
    OP_LIST_LOANS,           /* id1 = 0 all open loans, 1 only PENDING            */
    OP_LIST_EMPLOYEES,       /* manager / admin                                   */
    OP_ASSIGN_LOAN,          /* id1 = loan id, id2 = employee id                  */
    OP_LIST_FEEDBACK,        /* id1 = 0 all, 1 only NEW                           */
    OP_REVIEW_FEEDBACK,      /* id1 = feedback id, text = note                    */

    /* ---- administrator ---- */
    OP_ADD_EMPLOYEE = 80,    /* s1 first, s2 last, s3 phone, s4 email, text pass, id1 = role */
    OP_GET_EMPLOYEE,         /* id1 = employee id                                 */
    OP_MODIFY_EMPLOYEE,      /* id1 = employee id, id2 = field, s1 = new value    */
    OP_CHANGE_ROLE,          /* id1 = employee id, id2 = new role (2 or 3)        */
    OP_SET_EMPLOYEE_STATUS,  /* id1 = employee id, id2 = 1 activate / 2 deactivate */
    OP_LIST_SESSIONS,        /* live sessions from shared memory                  */
    OP_FORCE_LOGOUT,         /* id1 = user id                                     */
    OP_VIEW_AUDIT_LOG        /* id1 = number of lines                             */
} OpCode;

/* Field codes for OP_MODIFY_CUSTOMER / OP_MODIFY_EMPLOYEE */
typedef enum {
    FIELD_FIRST_NAME = 1,
    FIELD_LAST_NAME = 2,
    FIELD_PHONE = 3,
    FIELD_EMAIL = 4,
    FIELD_PASSWORD = 5       /* reset password */
} FieldCode;

typedef struct {
    int32_t op;
    int32_t id1;
    int32_t id2;
    int32_t id3;
    int64_t amount;              /* paise */
    char    s1[FIELD_LEN];
    char    s2[FIELD_LEN];
    char    s3[FIELD_LEN];
    char    s4[FIELD_LEN];
    char    text[TEXT_LEN];
} Request;

typedef enum {
    RESP_OK = 0,
    RESP_ERROR = 1,
    RESP_NOTICE = 2      /* server-initiated message: the session has ended */
} RespStatus;

/* Reason codes for RESP_NOTICE */
typedef enum {
    NOTICE_NONE = 0,
    NOTICE_IDLE_TIMEOUT = 1,
    NOTICE_FORCED_LOGOUT = 2,
    NOTICE_SERVER_SHUTDOWN = 3
} NoticeCode;

typedef struct {
    int32_t status;              /* RespStatus */
    int32_t code;                /* NoticeCode for notices                    */
    int32_t role;                /* filled on successful login                */
    int32_t uid;                 /* filled on successful login                */
    int64_t value;               /* numeric result (e.g. balance in paise)    */
    char    text[RESP_TEXT_LEN]; /* human readable, already formatted         */
} Response;

#endif
