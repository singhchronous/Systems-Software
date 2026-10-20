/*
 * manager_client.c - MANAGER module, client side (menu).
 */
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "util.h"

int manager_menu(const char *name, int uid)
{
    char who[128];
    snprintf(who, sizeof who, "Manager: %s   |   Manager ID: %d", name, uid);

    for (;;) {
        print_header("MANAGER MENU", who);
        printf("  1. Activate / Deactivate Customer Account\n");
        printf("  2. List All Customers\n");
        printf("  3. View Loan Applications\n");
        printf("  4. Assign Loan Application to Employee\n");
        printf("  5. Review Customer Feedback\n");
        printf("  6. Change Password\n");
        printf("  7. Logout\n");
        printf("  8. Exit\n");

        int ch = read_choice(8);
        if (ch < 0) return menu_result_for(ch);

        Request rq;
        int r = IN_OK;
        switch (ch) {
        case 1:
            req_init(&rq, OP_SET_CUSTOMER_STATUS);
            if ((r = read_int("Customer account number: ", &rq.id1)) != IN_OK) break;
            printf("  1. Activate\n  2. Deactivate\n");
            rq.id2 = read_choice(2);
            if (rq.id2 < 0) { r = rq.id2; break; }
            r = do_request(&rq);
            break;
        case 2:
            req_init(&rq, OP_LIST_CUSTOMERS);
            r = do_request(&rq);
            break;
        case 3:
            req_init(&rq, OP_LIST_LOANS);
            printf("  1. All open applications\n  2. Only unassigned (PENDING)\n");
            rq.id1 = read_choice(2);
            if (rq.id1 < 0) { r = rq.id1; break; }
            rq.id1 = rq.id1 == 2;
            r = do_request(&rq);
            break;
        case 4: {
            /* show what can be assigned, and to whom, before asking */
            req_init(&rq, OP_LIST_LOANS);
            rq.id1 = 1;
            if ((r = do_request(&rq)) != IN_OK) break;
            req_init(&rq, OP_LIST_EMPLOYEES);
            if ((r = do_request(&rq)) != IN_OK) break;
            req_init(&rq, OP_ASSIGN_LOAN);
            if ((r = read_int("\nLoan ID to assign: ", &rq.id1)) != IN_OK) break;
            if ((r = read_int("Assign to employee ID: ", &rq.id2)) != IN_OK) break;
            r = do_request(&rq);
            break;
        }
        case 5: {
            req_init(&rq, OP_LIST_FEEDBACK);
            printf("  1. Only unreviewed feedback\n  2. All feedback\n");
            int c = read_choice(2);
            if (c < 0) { r = c; break; }
            rq.id1 = c == 1;
            if ((r = do_request(&rq)) != IN_OK) break;
            if (!confirm("\nMark a feedback item as reviewed?")) break;
            req_init(&rq, OP_REVIEW_FEEDBACK);
            if ((r = read_int("Feedback #: ", &rq.id1)) != IN_OK) break;
            if ((r = read_line("Your note / action taken: ", rq.text, 128)) != IN_OK) break;
            r = do_request(&rq);
            break;
        }
        case 6:
            r = change_password_flow();
            break;
        case 7:
            return logout_flow();
        case 8:
            return exit_flow();
        }
        if (r != IN_OK) return menu_result_for(r);
        pause_enter();
    }
}
