/*
 * manager_server.c - MANAGER module, server side (UML class "Manager").
 *
 *   Activate/Deactivate Customer Accounts   set_customer_status()
 *   Assign Loan Application Processes       assign_loan_to_employee()
 *   Review Customer Feedback                get_all_feedback() + review_feedback()
 *   Change Password / Logout / Exit         auth.c / server.c
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "db.h"
#include "server.h"
#include "session.h"
#include "util.h"

/* ------------------------------------------------- activate / deactivate */

typedef struct {
    int new_status;
    int old_status;
} StatusCtx;

static int cust_status_cb(void *rec, void *ctx, char *err, size_t cap)
{
    CustomerRecord *r = rec;
    StatusCtx *s = ctx;
    s->old_status = r->status;
    if (r->status == STATUS_CLOSED) { str_copy(err, "This account is CLOSED and cannot be re-activated.", cap); return -1; }
    if (r->status == s->new_status) {
        snprintf(err, cap, "Account %d is already %s.", r->id, status_name(r->status));
        return -1;
    }
    r->status = s->new_status;
    r->updated_at = (int64_t)time(NULL);
    return 0;
}

static void set_customer_status(ClientCtx *c, const Request *rq, Response *rs)
{
    if (rq->id2 != 1 && rq->id2 != 2) { resp_err(rs, "Choose 1 (activate) or 2 (deactivate)."); return; }
    StatusCtx s = {rq->id2 == 1 ? STATUS_ACTIVE : STATUS_INACTIVE, 0};
    char err[160] = "";
    int rc = db_update(T_CUSTOMERS, rq->id1, cust_status_cb, &s, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Customer %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }

    int kicked = 0;
    if (s.new_status == STATUS_INACTIVE) kicked = force_logout_user(rq->id1);   /* signal SIGUSR1 */
    resp_ok(rs, "Customer account %d is now %s.%s", rq->id1, status_name(s.new_status),
            kicked ? "\nThe customer was online and has been logged out immediately." : "");
    log_transaction(c, "SET_CUSTOMER_STATUS %d -> %s", rq->id1, status_name(s.new_status));
}

/* -------------------------------------------------------- list loans */

typedef struct {
    int only_pending;
    Response *rs;
    int n;
} LoanListCtx;

static int loans_cb(const void *rec, void *ctx)
{
    const LoanRecord *l = rec;
    LoanListCtx *lc = ctx;
    if (l->status == LOAN_APPROVED || l->status == LOAN_REJECTED) return 0;
    if (lc->only_pending && l->status != LOAN_PENDING) return 0;
    char amt[40], when[32], who[16] = "-";
    format_money(l->amount, amt, sizeof amt);
    format_time(l->applied_at, when, sizeof when);
    if (l->assigned_to) snprintf(who, sizeof who, "%d", l->assigned_to);
    resp_append(lc->rs, "%-6d %-6d %18s %6d  %-13s %-9s %-19s %s\n", l->id, l->customer_id, amt,
                l->tenure_months, loan_status_name(l->status), who, when, l->purpose);
    lc->n++;
    return 0;
}

static void list_loans(const Request *rq, Response *rs)
{
    resp_ok(rs, "%s loan applications:\n", rq->id1 ? "Unassigned (PENDING)" : "All open");
    resp_append(rs, "%-6s %-6s %18s %6s  %-13s %-9s %-19s %s\n", "Loan", "A/c", "Amount", "Months",
                "Status", "Assigned", "Applied on", "Purpose");
    resp_append(rs, "------------------------------------------------------------------------------------------------\n");
    LoanListCtx lc = {rq->id1, rs, 0};
    db_scan(T_LOANS, loans_cb, &lc);
    if (lc.n == 0) resp_append(rs, "(none)\n");
}

/* ----------------------------------------------------------- assign loan */

typedef struct {
    int32_t emp, mgr;
    int32_t prev;
} AssignCtx;

static int assign_cb(void *rec, void *ctx, char *err, size_t cap)
{
    LoanRecord *l = rec;
    AssignCtx *a = ctx;
    if (l->status != LOAN_PENDING && l->status != LOAN_ASSIGNED) {
        snprintf(err, cap, "Loan %d is %s and can no longer be (re)assigned.", l->id, loan_status_name(l->status));
        return -1;
    }
    if (l->assigned_to == a->emp) {
        snprintf(err, cap, "Loan %d is already assigned to employee %d.", l->id, a->emp);
        return -1;
    }
    a->prev = l->assigned_to;
    l->assigned_to = a->emp;
    l->assigned_by = a->mgr;
    l->status = LOAN_ASSIGNED;
    l->updated_at = (int64_t)time(NULL);
    return 0;
}

/* UML: assign_loan_to_employee */
static void assign_loan_to_employee(ClientCtx *c, const Request *rq, Response *rs)
{
    EmployeeRecord e;
    if (db_get(T_EMPLOYEES, rq->id2, &e) == -1) { resp_err(rs, "Employee %d not found.", rq->id2); return; }
    if (e.role != ROLE_EMPLOYEE) { resp_err(rs, "%d is a %s, not a bank employee.", e.id, role_name(e.role)); return; }
    if (e.status != STATUS_ACTIVE) { resp_err(rs, "Employee %d is not active.", e.id); return; }

    AssignCtx a = {rq->id2, c->uid, 0};
    char err[160] = "";
    int rc = db_update(T_LOANS, rq->id1, assign_cb, &a, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Loan %d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }
    if (a.prev)
        resp_ok(rs, "Loan %d re-assigned from employee %d to %d (%s %s).", rq->id1, a.prev, e.id, e.first_name, e.last_name);
    else
        resp_ok(rs, "Loan %d assigned to employee %d (%s %s).", rq->id1, e.id, e.first_name, e.last_name);
    log_transaction(c, "ASSIGN_LOAN %d -> employee %d", rq->id1, e.id);
}

/* --------------------------------------------------------------- feedback */

typedef struct {
    int only_new;
    Response *rs;
    int n, unread;
} FbCtx;

static int fb_cb(const void *rec, void *ctx)
{
    const FeedbackRecord *f = rec;
    FbCtx *fc = ctx;
    if (f->status == FEEDBACK_NEW) fc->unread++;
    if (fc->only_new && f->status != FEEDBACK_NEW) return 0;
    char when[32];
    format_time(f->created_at, when, sizeof when);
    resp_append(fc->rs, "#%-4d A/c %-5d %s  [%s]\n      \"%s\"\n", f->id, f->customer_id, when,
                feedback_status_name(f->status), f->message);
    if (f->status == FEEDBACK_REVIEWED)
        resp_append(fc->rs, "      -> reviewed by %d: %s\n", f->reviewed_by, f->manager_note);
    fc->n++;
    return 0;
}

/* UML: get_all_feedback */
static void get_all_feedback(const Request *rq, Response *rs)
{
    resp_ok(rs, "%s customer feedback:\n\n", rq->id1 ? "Unreviewed" : "All");
    FbCtx fc = {rq->id1, rs, 0, 0};
    db_scan(T_FEEDBACK, fb_cb, &fc);
    if (fc.n == 0) resp_append(rs, "(none)\n");
    resp_append(rs, "\n%d feedback item(s) waiting for review.\n", fc.unread);
}

typedef struct {
    int32_t mgr;
    const char *note;
} ReviewCtx;

static int review_cb(void *rec, void *ctx, char *err, size_t cap)
{
    FeedbackRecord *f = rec;
    ReviewCtx *r = ctx;
    if (f->status == FEEDBACK_REVIEWED) {
        snprintf(err, cap, "Feedback #%d was already reviewed by %d.", f->id, f->reviewed_by);
        return -1;
    }
    f->status = FEEDBACK_REVIEWED;
    f->reviewed_by = r->mgr;
    str_copy(f->manager_note, r->note, sizeof f->manager_note);
    f->reviewed_at = (int64_t)time(NULL);
    return 0;
}

/* UML: review_feedback(feedback_id) */
static void review_feedback(ClientCtx *c, const Request *rq, Response *rs)
{
    char note[FEEDBACK_NOTE_LEN];
    str_copy(note, rq->text, sizeof note);
    str_trim(note);
    if (!note[0]) str_copy(note, "Noted", sizeof note);
    ReviewCtx r = {c->uid, note};
    char err[160] = "";
    int rc = db_update(T_FEEDBACK, rq->id1, review_cb, &r, err, sizeof err);
    if (rc == -1) { resp_err(rs, "Feedback #%d not found.", rq->id1); return; }
    if (rc != 0) { resp_err(rs, "%s", err); return; }
    resp_ok(rs, "Feedback #%d marked as REVIEWED.", rq->id1);
    log_transaction(c, "REVIEW_FEEDBACK #%d", rq->id1);
}

/* UML: Bank::handle_manager_requests */
int handle_manager_requests(ClientCtx *c, const Request *rq, Response *rs)
{
    switch (rq->op) {
    case OP_SET_CUSTOMER_STATUS: set_customer_status(c, rq, rs); return 1;
    case OP_LIST_LOANS:          list_loans(rq, rs); return 1;
    case OP_LIST_EMPLOYEES:      op_list_employees(rs, 1); return 1;
    case OP_ASSIGN_LOAN:         assign_loan_to_employee(c, rq, rs); return 1;
    case OP_LIST_FEEDBACK:       get_all_feedback(rq, rs); return 1;
    case OP_REVIEW_FEEDBACK:     review_feedback(c, rq, rs); return 1;
    case OP_LIST_CUSTOMERS:      op_list_customers(rs); return 1;
    case OP_GET_CUSTOMER:        op_customer_details(rq->id1, rs); return 1;
    default:                     return 0;
    }
}
