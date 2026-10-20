/*
 * client.h - Client-side helpers shared by the four role menus.
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>
#include "protocol.h"

/* What a role menu returns to the main loop */
typedef enum {
    MENU_LOGOUT = 0,          /* back to the login screen            */
    MENU_EXIT = 1,            /* quit the program                    */
    MENU_SESSION_ENDED = 2,   /* server ended the session (notice)   */
    MENU_CONN_LOST = 3        /* server unreachable                  */
} MenuResult;

/* Input results */
#define IN_OK            0
#define IN_EOF          -1    /* Ctrl+D / end of input                */
#define IN_SESSION_END  -2    /* a server notice arrived while waiting */
#define IN_CONN_LOST    -3

extern int g_sock;

/* ---------- input (select() on keyboard AND socket, so server notices such
 *            as "forced logout" are shown immediately) ---------- */
int read_line(const char *prompt, char *buf, int cap);
int read_int(const char *prompt, int *out);
int read_money(const char *prompt, int64_t *paise);
int read_password(const char *prompt, char *buf, int cap);
int read_choice(int max);                 /* "Enter your choice: " */
int confirm(const char *question);        /* returns 1 for y/Y */

/* ---------- requests ---------- */
void req_init(Request *rq, int op);
/* Send and receive. Returns IN_OK, IN_SESSION_END or IN_CONN_LOST. */
int  transact(const Request *rq, Response *rs);
/* transact() + print the response. Returns IN_OK / IN_SESSION_END / IN_CONN_LOST. */
int  do_request(const Request *rq);

/* ---------- output ---------- */
void print_header(const char *title, const char *who);
void print_ok(const char *text);
void print_err(const char *text);
void print_notice(const Response *rs);
void pause_enter(void);

/* Map an input / transact status to the menu result. */
int  menu_result_for(int status);

/* Common menu actions */
int  change_password_flow(void);
int  logout_flow(void);                   /* returns MENU_LOGOUT or other */
int  exit_flow(void);                     /* returns MENU_EXIT */

/* ---------- role menus (src/modules/<role>/<role>_client.c) ---------- */
int customer_menu(const char *name, int uid);
int employee_menu(const char *name, int uid);
int manager_menu(const char *name, int uid);
int admin_menu(const char *name, int uid);

#endif
