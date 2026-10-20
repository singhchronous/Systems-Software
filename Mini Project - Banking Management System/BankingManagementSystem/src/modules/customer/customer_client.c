/*
 * customer_client.c - CUSTOMER module, client side (menu).
 */
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "util.h"

int customer_menu(const char *name, int uid)
{
    char who[128];
    snprintf(who, sizeof who, "Customer: %s   |   Account No: %d", name, uid);

    for (;;) {
        print_header("CUSTOMER MENU", who);
        printf("  1. View Account Balance\n");
        printf("  2. Deposit Money\n");
        printf("  3. Withdraw Money\n");
        printf("  4. Transfer Funds\n");
        printf("  5. Apply for a Loan\n");
        printf("  6. View My Loan Applications\n");
        printf("  7. Change Password\n");
        printf("  8. Add Feedback\n");
        printf("  9. View Transaction History\n");
        printf(" 10. Logout\n");
        printf(" 11. Exit\n");

        int ch = read_choice(11);
        if (ch < 0) return menu_result_for(ch);

        Request rq;
        int r = IN_OK;
        switch (ch) {
        case 1:
            req_init(&rq, OP_VIEW_BALANCE);
            r = do_request(&rq);
            break;
        case 2:
            req_init(&rq, OP_DEPOSIT);
            if ((r = read_money("Amount to deposit (Rs.): ", &rq.amount)) == IN_OK) r = do_request(&rq);
            break;
        case 3:
            req_init(&rq, OP_WITHDRAW);
            if ((r = read_money("Amount to withdraw (Rs.): ", &rq.amount)) == IN_OK) r = do_request(&rq);
            break;
        case 4: {
            req_init(&rq, OP_TRANSFER);
            if ((r = read_int("Transfer to account number: ", &rq.id1)) != IN_OK) break;
            if ((r = read_money("Amount to transfer (Rs.): ", &rq.amount)) != IN_OK) break;
            char amt[40], q[96];
            format_money(rq.amount, amt, sizeof amt);
            snprintf(q, sizeof q, "Transfer %s to account %d?", amt, rq.id1);
            if (confirm(q)) r = do_request(&rq);
            else printf("Transfer cancelled.\n");
            break;
        }
        case 5:
            req_init(&rq, OP_APPLY_LOAN);
            if ((r = read_money("Loan amount (Rs.): ", &rq.amount)) != IN_OK) break;
            if ((r = read_int("Tenure in months (1-360): ", &rq.id1)) != IN_OK) break;
            if ((r = read_line("Purpose of the loan: ", rq.s1, sizeof rq.s1)) != IN_OK) break;
            r = do_request(&rq);
            break;
        case 6:
            req_init(&rq, OP_VIEW_MY_LOANS);
            r = do_request(&rq);
            break;
        case 7:
            r = change_password_flow();
            break;
        case 8:
            req_init(&rq, OP_ADD_FEEDBACK);
            if ((r = read_line("Your feedback (max 255 characters):\n> ", rq.text, sizeof rq.text)) == IN_OK)
                r = do_request(&rq);
            break;
        case 9:
            req_init(&rq, OP_VIEW_HISTORY);
            r = do_request(&rq);
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
