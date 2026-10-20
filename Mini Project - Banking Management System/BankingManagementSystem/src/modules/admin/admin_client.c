/*
 * admin_client.c - ADMINISTRATOR module, client side (menu).
 */
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "util.h"

int modify_details_flow(int op, const char *what);   /* employee_client.c */

static int manage_roles_flow(void)
{
    Request rq;
    int r;
    req_init(&rq, OP_LIST_EMPLOYEES);
    if ((r = do_request(&rq)) != IN_OK) return r;
    printf("\n  1. Promote Employee to Manager\n  2. Demote Manager to Employee\n"
           "  3. Deactivate an Employee/Manager\n  4. Re-activate an Employee/Manager\n  5. Back\n");
    int c = read_choice(5);
    if (c < 0) return c;
    if (c == 5) return IN_OK;
    int id;
    if ((r = read_int("Employee ID: ", &id)) != IN_OK) return r;
    if (c <= 2) {
        req_init(&rq, OP_CHANGE_ROLE);
        rq.id1 = id;
        rq.id2 = c == 1 ? 3 : 2;            /* ROLE_MANAGER : ROLE_EMPLOYEE */
    } else {
        req_init(&rq, OP_SET_EMPLOYEE_STATUS);
        rq.id1 = id;
        rq.id2 = c == 4 ? 1 : 2;
    }
    return do_request(&rq);
}

int admin_menu(const char *name, int uid)
{
    char who[128];
    snprintf(who, sizeof who, "Administrator: %s   |   Admin ID: %d", name, uid);

    for (;;) {
        print_header("ADMINISTRATOR MENU", who);
        printf("  1. Add New Bank Employee\n");
        printf("  2. Modify Customer Details\n");
        printf("  3. Modify Employee Details\n");
        printf("  4. View Customer / Employee Details\n");
        printf("  5. Manage User Roles\n");
        printf("  6. View Live Sessions\n");
        printf("  7. Force Logout a User\n");
        printf("  8. View Audit Log\n");
        printf("  9. Change Password\n");
        printf(" 10. Logout\n");
        printf(" 11. Exit\n");

        int ch = read_choice(11);
        if (ch < 0) return menu_result_for(ch);

        Request rq;
        int r = IN_OK;
        switch (ch) {
        case 1: {
            req_init(&rq, OP_ADD_EMPLOYEE);
            char again[TEXT_LEN];
            if ((r = read_line("First name: ", rq.s1, sizeof rq.s1)) != IN_OK) break;
            if ((r = read_line("Last name: ", rq.s2, sizeof rq.s2)) != IN_OK) break;
            if ((r = read_line("Phone: ", rq.s3, sizeof rq.s3)) != IN_OK) break;
            if ((r = read_line("Email: ", rq.s4, sizeof rq.s4)) != IN_OK) break;
            printf("Role:\n  1. Bank Employee\n  2. Manager\n");
            int role = read_choice(2);
            if (role < 0) { r = role; break; }
            rq.id1 = role == 2 ? 3 : 2;
            if ((r = read_password("Initial password: ", rq.text, FIELD_LEN)) != IN_OK) break;
            if ((r = read_password("Re-enter password: ", again, FIELD_LEN)) != IN_OK) break;
            if (strcmp(rq.text, again) != 0) { print_err("Passwords do not match."); break; }
            r = do_request(&rq);
            break;
        }
        case 2:
            r = modify_details_flow(OP_MODIFY_CUSTOMER, "Customer account");
            break;
        case 3:
            r = modify_details_flow(OP_MODIFY_EMPLOYEE, "Employee");
            break;
        case 4: {
            printf("  1. One customer\n  2. One employee/manager\n  3. List all customers\n  4. List all employees/managers\n");
            int c = read_choice(4);
            if (c < 0) { r = c; break; }
            if (c == 1) {
                req_init(&rq, OP_GET_CUSTOMER);
                if ((r = read_int("Customer account number: ", &rq.id1)) != IN_OK) break;
            } else if (c == 2) {
                req_init(&rq, OP_GET_EMPLOYEE);
                if ((r = read_int("Employee ID: ", &rq.id1)) != IN_OK) break;
            } else {
                req_init(&rq, c == 3 ? OP_LIST_CUSTOMERS : OP_LIST_EMPLOYEES);
            }
            r = do_request(&rq);
            break;
        }
        case 5:
            r = manage_roles_flow();
            break;
        case 6:
            req_init(&rq, OP_LIST_SESSIONS);
            r = do_request(&rq);
            break;
        case 7:
            req_init(&rq, OP_LIST_SESSIONS);
            if ((r = do_request(&rq)) != IN_OK) break;
            req_init(&rq, OP_FORCE_LOGOUT);
            if ((r = read_int("User ID to log out: ", &rq.id1)) != IN_OK) break;
            r = do_request(&rq);
            break;
        case 8:
            req_init(&rq, OP_VIEW_AUDIT_LOG);
            if ((r = read_int("How many recent entries (1-80): ", &rq.id1)) == IN_OK) r = do_request(&rq);
            break;
        case 9:
            r = change_password_flow();
            break;
        case 10:
            return logout_flow();
        case 11:
            return exit_flow();
        }
        if (r != IN_OK) return menu_result_for(r);
        pause_enter();
    }
}
