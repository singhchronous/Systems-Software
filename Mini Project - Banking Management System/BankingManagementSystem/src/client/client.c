/*
 * client.c - Banking Management System client.
 *
 * Usage: ./bank_client [server_ip] [port]      (default 127.0.0.1 8080)
 *
 * The client is "thin": it only shows menus, reads input and prints what the
 * server answers. All rules, checks and data live in the server.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "net.h"
#include "util.h"

static const char *g_ip = "127.0.0.1";
static int g_port = DEFAULT_PORT;

static int reconnect(void)
{
    if (g_sock >= 0) close(g_sock);
    g_sock = -1;
    for (int attempt = 0; attempt < 3; attempt++) {
        g_sock = connect_to_server(g_ip, g_port);
        if (g_sock >= 0) return 0;
        if (attempt < 2) sleep(1);
    }
    return -1;
}

static void say_goodbye(void)
{
    if (g_sock >= 0) {
        Request rq;
        Response rs;
        req_init(&rq, OP_EXIT);
        if (send_request(g_sock, &rq) == 0) recv_response(g_sock, &rs);
        close(g_sock);
        g_sock = -1;
    }
    printf("\nThank you for banking with us. Goodbye!\n");
}

static int login_flow(int role, char *name_out, int *uid_out)
{
    Request rq;
    Response rs;
    req_init(&rq, OP_LOGIN);
    rq.id1 = role;
    int r;
    const char *label = role == 1 ? "Account number: " : role == 4 ? "Admin ID: " : "Employee ID: ";
    if ((r = read_int(label, &rq.id2)) != IN_OK) return r;
    if ((r = read_password("Password: ", rq.s1, sizeof rq.s1)) != IN_OK) return r;
    if ((r = transact(&rq, &rs)) != IN_OK) return r;
    if (rs.status != RESP_OK) {
        print_err(rs.text);
        return 1;                                  /* login refused */
    }
    str_copy(name_out, rs.text, 64);
    *uid_out = rs.uid;
    printf("\nLogin successful. Welcome, %s!\n", name_out);
    return IN_OK;
}

int main(int argc, char *argv[])
{
    if (argc > 1) g_ip = argv[1];
    if (argc > 2) g_port = atoi(argv[2]);
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (reconnect() == -1) {
        fprintf(stderr, "Cannot connect to the bank server at %s:%d. Is ./bank_server running?\n", g_ip, g_port);
        return 1;
    }

    for (;;) {
        print_header("WELCOME TO THE BANKING MANAGEMENT SYSTEM", "Please choose how you want to log in");
        printf("  1. Customer\n");
        printf("  2. Bank Employee\n");
        printf("  3. Manager\n");
        printf("  4. Administrator\n");
        printf("  5. Exit\n");

        int ch = read_choice(5);
        if (ch == IN_EOF || ch == 5) break;
        if (ch == IN_CONN_LOST || ch == IN_SESSION_END) {
            if (reconnect() == -1) { print_err("Server is not reachable. Exiting."); return 1; }
            continue;
        }

        char name[64];
        int uid = 0;
        int r = login_flow(ch, name, &uid);
        if (r == IN_EOF) break;
        if (r == IN_CONN_LOST || r == IN_SESSION_END) {
            if (reconnect() == -1) { print_err("Server is not reachable. Exiting."); return 1; }
            continue;
        }
        if (r != IN_OK) continue;                  /* wrong password etc. */

        int res;
        switch (ch) {
        case 1:  res = customer_menu(name, uid); break;
        case 2:  res = employee_menu(name, uid); break;
        case 3:  res = manager_menu(name, uid); break;
        default: res = admin_menu(name, uid); break;
        }

        if (res == MENU_EXIT) break;
        if (res == MENU_SESSION_ENDED || res == MENU_CONN_LOST) {
            /* the server closed this connection - open a fresh one for the login screen */
            printf("Reconnecting to the server...\n");
            if (reconnect() == -1) { print_err("Server is not reachable. Exiting."); return 1; }
        }
        /* MENU_LOGOUT: same connection, back to the login screen */
    }
    say_goodbye();
    return 0;
}
