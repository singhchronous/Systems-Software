/*
 * ui.c - Terminal input/output helpers for the client.
 *
 * Input is read with the read() system call, one byte at a time, after
 * select() reports that the keyboard (fd 0) is readable. select() also watches
 * the server socket, so if the server pushes a notice while the user is sitting
 * at a menu (forced logout, idle timeout, shutdown) it is printed at once.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "client.h"
#include "net.h"
#include "util.h"

int g_sock = -1;

static int use_color(void) { return isatty(STDOUT_FILENO); }
#define C_GREEN  (use_color() ? "\033[32m" : "")
#define C_RED    (use_color() ? "\033[31m" : "")
#define C_YELLOW (use_color() ? "\033[33m" : "")
#define C_BOLD   (use_color() ? "\033[1m" : "")
#define C_RESET  (use_color() ? "\033[0m" : "")

/* ------------------------------------------------------------------ output */

void print_header(const char *title, const char *who)
{
    printf("\n%s==================================================%s\n", C_BOLD, C_RESET);
    printf("%s  %s%s\n", C_BOLD, title, C_RESET);
    if (who && *who) printf("  %s\n", who);
    printf("%s==================================================%s\n", C_BOLD, C_RESET);
}

void print_ok(const char *text)
{
    printf("\n%s[OK]%s %s\n", C_GREEN, C_RESET, text);
}

void print_err(const char *text)
{
    printf("\n%s[ERROR]%s %s\n", C_RED, C_RESET, text);
}

void print_notice(const Response *rs)
{
    printf("\n\n%s**************************************************%s\n", C_YELLOW, C_RESET);
    printf("%s  NOTICE FROM SERVER:%s %s\n", C_YELLOW, C_RESET, rs->text);
    printf("%s**************************************************%s\n", C_YELLOW, C_RESET);
}

/* ------------------------------------------------------------------- input */

/* Called when the socket became readable while we were waiting for input. */
static int handle_unsolicited(void)
{
    Response rs;
    int r = recv_response(g_sock, &rs);
    if (r <= 0) {
        print_err("Lost connection to the server.");
        return IN_CONN_LOST;
    }
    if (rs.status == RESP_NOTICE) {
        print_notice(&rs);
        return IN_SESSION_END;
    }
    return IN_OK;   /* unexpected data - ignore */
}

int read_line(const char *prompt, char *buf, int cap)
{
    if (prompt) { printf("%s", prompt); fflush(stdout); }
    int n = 0;
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        int maxfd = STDIN_FILENO;
        if (g_sock >= 0) {
            FD_SET(g_sock, &rfds);
            if (g_sock > maxfd) maxfd = g_sock;
        }
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            return IN_EOF;
        }
        if (g_sock >= 0 && FD_ISSET(g_sock, &rfds)) {
            int r = handle_unsolicited();
            if (r != IN_OK) return r;
        }
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char ch;
            ssize_t r = read(STDIN_FILENO, &ch, 1);
            if (r <= 0) {                       /* Ctrl+D or end of piped input */
                if (n > 0) break;
                return IN_EOF;
            }
            if (ch == '\n') break;
            if (ch == '\r') continue;
            if (n < cap - 1) buf[n++] = ch;
        }
    }
    buf[n] = '\0';
    str_trim(buf);
    return IN_OK;
}

int read_int(const char *prompt, int *out)
{
    char buf[64];
    for (;;) {
        int r = read_line(prompt, buf, sizeof buf);
        if (r != IN_OK) return r;
        if (parse_int(buf, out) == 0) return IN_OK;
        print_err("Please enter a whole number.");
    }
}

int read_money(const char *prompt, int64_t *paise)
{
    char buf[64];
    for (;;) {
        int r = read_line(prompt, buf, sizeof buf);
        if (r != IN_OK) return r;
        if (parse_money(buf, paise) == 0 && *paise > 0) return IN_OK;
        print_err("Enter a positive amount with at most 2 decimals, e.g. 1500 or 1500.50");
    }
}

int read_password(const char *prompt, char *buf, int cap)
{
    struct termios old, quiet;
    int tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &old) == 0;
    /* TCSANOW (not TCSAFLUSH): TCSAFLUSH would DISCARD keys the user already
     * typed ahead, silently cutting the start of the password. */
    if (tty) {                                  /* hide what is typed */
        quiet = old;
        quiet.c_lflag &= (tcflag_t)~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &quiet);
    }
    int r = read_line(prompt, buf, cap);
    if (tty) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        printf("\n");
    }
    return r;
}

int read_choice(int max)
{
    int choice;
    for (;;) {
        int r = read_int("\nEnter your choice: ", &choice);
        if (r != IN_OK) return r;
        if (choice >= 1 && choice <= max) return choice;
        printf("Please choose a number between 1 and %d.\n", max);
    }
}

int confirm(const char *question)
{
    char buf[16];
    printf("%s (y/n): ", question);
    if (read_line(NULL, buf, sizeof buf) != IN_OK) return 0;
    return buf[0] == 'y' || buf[0] == 'Y';
}

void pause_enter(void)
{
    char buf[8];
    if (!isatty(STDIN_FILENO)) return;          /* scripted test input: don't wait */
    read_line("\nPress Enter to continue...", buf, sizeof buf);
}

/* ---------------------------------------------------------------- requests */

void req_init(Request *rq, int op)
{
    memset(rq, 0, sizeof *rq);
    rq->op = op;
}

int transact(const Request *rq, Response *rs)
{
    if (g_sock < 0 || send_request(g_sock, rq) == -1) {
        print_err("Lost connection to the server.");
        return IN_CONN_LOST;
    }
    int r = recv_response(g_sock, rs);
    if (r <= 0) {
        print_err("Lost connection to the server.");
        return IN_CONN_LOST;
    }
    if (rs->status == RESP_NOTICE) {             /* e.g. timed out just before the request */
        print_notice(rs);
        return IN_SESSION_END;
    }
    return IN_OK;
}

int do_request(const Request *rq)
{
    static Response rs;
    int r = transact(rq, &rs);
    if (r != IN_OK) return r;
    if (rs.status == RESP_OK) print_ok(rs.text);
    else print_err(rs.text);
    return IN_OK;
}

int menu_result_for(int status)
{
    switch (status) {
    case IN_SESSION_END: return MENU_SESSION_ENDED;
    case IN_CONN_LOST:   return MENU_CONN_LOST;
    case IN_EOF:         return MENU_EXIT;
    default:             return -1;
    }
}

/* ------------------------------------------------------- shared menu items */

int change_password_flow(void)
{
    Request rq;
    req_init(&rq, OP_CHANGE_PASSWORD);
    char again[FIELD_LEN];
    int r;
    if ((r = read_password("Current password: ", rq.s1, sizeof rq.s1)) != IN_OK) return r;
    if ((r = read_password("New password (4-32 chars, no spaces): ", rq.s2, sizeof rq.s2)) != IN_OK) return r;
    if ((r = read_password("Re-enter new password: ", again, sizeof again)) != IN_OK) return r;
    if (strcmp(rq.s2, again) != 0) {
        print_err("The two new passwords do not match.");
        return IN_OK;
    }
    return do_request(&rq);
}

int logout_flow(void)
{
    Request rq;
    Response rs;
    req_init(&rq, OP_LOGOUT);
    int r = transact(&rq, &rs);
    if (r != IN_OK) return menu_result_for(r);
    print_ok(rs.text);
    return MENU_LOGOUT;
}

int exit_flow(void)
{
    return MENU_EXIT;     /* main() sends OP_EXIT and closes the connection */
}
