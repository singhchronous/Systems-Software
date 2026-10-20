/*
 * employee_client.c - BANK EMPLOYEE module, client side (menu).
 */
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "util.h"

/* Shared with the admin menu: pick a field and new value for a customer/employee. */
int modify_details_flow(int op, const char *what)
{
    Request rq;
    req_init(&rq, op);
    int r;
    char prompt[64];
    snprintf(prompt, sizeof prompt, "%s ID: ", what);
    if ((r = read_int(prompt, &rq.id1)) != IN_OK) return r;
    printf("Which detail do you want to change?\n");
    printf("  1. First name\n  2. Last name\n  3. Phone\n  4. Email\n  5. Reset password\n");
    int f = read_choice(5);
    if (f < 0) return f;
    rq.id2 = f;
    if (f == 5) r = read_password("New password: ", rq.s1, sizeof rq.s1);
    else r = read_line("New value: ", rq.s1, sizeof rq.s1);
    if (r != IN_OK) return r;
    return do_request(&rq);
}

int employee_menu(const char *name, int uid)
{
    char who[128];
    snprintf(who, sizeof who, "Bank Employee: %s   |   Employee ID: %d", name, uid);

    for (;;) {
        print_header("BANK EMPLOYEE MENU", who);
        printf("  1. Add New Customer (open account)\n");
        printf("  2. Modify Customer Details\n");
        printf("  3. View Customer Details\n");
        printf("  4. List All Customers\n");
        printf("  5. Close Customer Account\n");
        printf("  6. View Assigned Loan Applications\n");
        printf("  7. Process Loan Application\n");
        printf("  8. Approve / Reject Loan\n");
        printf("  9. View Customer Transactions (Passbook)\n");
        printf(" 10. Change Password\n");
        printf(" 11. Logout\n");
        printf(" 12. Exit\n");

        int ch = read_choice(12);
        if (ch < 0) return menu_result_for(ch);

        Request rq;
        int r = IN_OK;
        switch (ch) {
        case 1: {
            req_init(&rq, OP_ADD_CUSTOMER);
            char again[TEXT_LEN];
            if ((r = read_line("First name: ", rq.s1, sizeof rq.s1)) != IN_OK) break;
            if ((r = read_line("Last name: ", rq.s2, sizeof rq.s2)) != IN_OK) break;
            if ((r = read_line("Phone: ", rq.s3, sizeof rq.s3)) != IN_OK) break;
            if ((r = read_line("Email: ", rq.s4, sizeof rq.s4)) != IN_OK) break;
            if ((r = read_password("Initial password for the customer: ", rq.text, FIELD_LEN)) != IN_OK) break;
            if ((r = read_password("Re-enter password: ", again, FIELD_LEN)) != IN_OK) break;
            if (strcmp(rq.text, again) != 0) { print_err("Passwords do not match."); break; }
            char buf[64];
            if ((r = read_line("Opening deposit in Rs. (0 for none): ", buf, sizeof buf)) != IN_OK) break;
            if (parse_money(buf, &rq.amount) == -1) { print_err("Invalid amount."); break; }
            r = do_request(&rq);
            break;
        }
        case 2:
            r = modify_details_flow(OP_MODIFY_CUSTOMER, "Customer account");
            break;
        case 3:
            req_init(&rq, OP_GET_CUSTOMER);
            if ((r = read_int("Customer account number: ", &rq.id1)) == IN_OK) r = do_request(&rq);
            break;
        case 4:
            req_init(&rq, OP_LIST_CUSTOMERS);
            r = do_request(&rq);
            break;
        case 5:
            req_init(&rq, OP_CLOSE_CUSTOMER);
            if ((r = read_int("Account number to CLOSE: ", &rq.id1)) != IN_OK) break;
            if (confirm("Closing is permanent. Continue?")) r = do_request(&rq);
            else printf("Cancelled.\n");
            break;
        case 6:
            req_init(&rq, OP_VIEW_ASSIGNED_LOANS);
            r = do_request(&rq);
            break;
        case 7:
            req_init(&rq, OP_PROCESS_LOAN);
            if ((r = read_int("Loan ID to process: ", &rq.id1)) == IN_OK) r = do_request(&rq);
            break;
        case 8:
            req_init(&rq, OP_DECIDE_LOAN);
            if ((r = read_int("Loan ID: ", &rq.id1)) != IN_OK) break;
            printf("  1. Approve (amount is credited to the customer)\n  2. Reject\n");
            rq.id2 = read_choice(2);
            if (rq.id2 < 0) { r = rq.id2; break; }
            if ((r = read_line(rq.id2 == 1 ? "Remark (optional): " : "Reason for rejection: ",
                               rq.text, REMARK_LEN)) != IN_OK) break;
            r = do_request(&rq);
            break;
        case 9:
            req_init(&rq, OP_VIEW_PASSBOOK);
            if ((r = read_int("Customer account number: ", &rq.id1)) == IN_OK) r = do_request(&rq);
            break;
        case 10:
            r = change_password_flow();
            break;
        case 11:
            return logout_flow();
        case 12:
            return exit_flow();
        }
        if (r != IN_OK) return menu_result_for(r);
        pause_enter();
    }
}
