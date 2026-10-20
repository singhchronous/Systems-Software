/*
 * util.c - String, money, time and validation helpers.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "util.h"

void str_copy(char *dst, const char *src, size_t cap)
{
    if (cap == 0) return;
    size_t i = 0;
    if (src) {
        for (; i + 1 < cap && src[i]; i++) dst[i] = src[i];
    }
    dst[i] = '\0';
}

void str_trim(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    size_t start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start) memmove(s, s + start, len - start + 1);
}

void appendf(char *buf, size_t cap, const char *fmt, ...)
{
    size_t used = strlen(buf);
    if (used + 1 >= cap) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + used, cap - used, fmt, ap);
    va_end(ap);
}

int parse_money(const char *s, int64_t *paise)
{
    if (!s) return -1;
    while (*s == ' ') s++;
    if (!*s) return -1;
    int64_t rupees = 0, frac = 0;
    int digits = 0, frac_digits = 0, seen_dot = 0;
    for (const char *p = s; *p; p++) {
        if (*p == '.') {
            if (seen_dot) return -1;
            seen_dot = 1;
        } else if (isdigit((unsigned char)*p)) {
            if (seen_dot) {
                if (++frac_digits > 2) return -1;
                frac = frac * 10 + (*p - '0');
            } else {
                if (++digits > 12) return -1;          /* avoid overflow */
                rupees = rupees * 10 + (*p - '0');
            }
        } else if (*p == ' ' || *p == '\n') {
            break;
        } else {
            return -1;
        }
    }
    if (digits == 0 && frac_digits == 0) return -1;
    if (frac_digits == 1) frac *= 10;
    *paise = rupees * 100 + frac;
    return 0;
}

void format_money(int64_t paise, char *buf, size_t cap)
{
    int neg = paise < 0;
    uint64_t v = neg ? (uint64_t)(-paise) : (uint64_t)paise;
    uint64_t rupees = v / 100, frac = v % 100;

    char digits[32];
    snprintf(digits, sizeof digits, "%llu", (unsigned long long)rupees);
    size_t n = strlen(digits);

    /* Indian grouping: last 3 digits, then groups of 2 (1,23,45,678) */
    char grouped[48];
    size_t gi = 0;
    for (size_t i = 0; i < n; i++) {
        size_t remaining = n - i;
        grouped[gi++] = digits[i];
        if (remaining > 3 && (remaining - 3) % 2 == 1) grouped[gi++] = ',';
    }
    grouped[gi] = '\0';
    snprintf(buf, cap, "%sRs. %s.%02llu", neg ? "-" : "", grouped, (unsigned long long)frac);
}

void format_time(int64_t t, char *buf, size_t cap)
{
    time_t tt = (time_t)t;
    struct tm tmv;
    localtime_r(&tt, &tmv);
    strftime(buf, cap, "%Y-%m-%d %H:%M:%S", &tmv);
}

int parse_int(const char *s, int *out)
{
    if (!s) return -1;
    while (*s == ' ') s++;
    if (!*s) return -1;
    long v = 0;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    if (!*s) return -1;
    for (; *s; s++) {
        if (*s == '\n' || *s == ' ') break;
        if (!isdigit((unsigned char)*s)) return -1;
        v = v * 10 + (*s - '0');
        if (v > 2000000000L) return -1;
    }
    *out = (int)(neg ? -v : v);
    return 0;
}

const char *role_name(int role)
{
    switch (role) {
    case ROLE_CUSTOMER: return "Customer";
    case ROLE_EMPLOYEE: return "Employee";
    case ROLE_MANAGER:  return "Manager";
    case ROLE_ADMIN:    return "Administrator";
    default:            return "None";
    }
}

const char *status_name(int status)
{
    switch (status) {
    case STATUS_ACTIVE:   return "ACTIVE";
    case STATUS_INACTIVE: return "INACTIVE";
    case STATUS_CLOSED:   return "CLOSED";
    default:              return "UNKNOWN";
    }
}

const char *loan_status_name(int status)
{
    switch (status) {
    case LOAN_PENDING:      return "PENDING";
    case LOAN_ASSIGNED:     return "ASSIGNED";
    case LOAN_UNDER_REVIEW: return "UNDER_REVIEW";
    case LOAN_APPROVED:     return "APPROVED";
    case LOAN_REJECTED:     return "REJECTED";
    default:                return "UNKNOWN";
    }
}

const char *txn_type_name(int type)
{
    switch (type) {
    case TXN_DEPOSIT:      return "DEPOSIT";
    case TXN_WITHDRAW:     return "WITHDRAW";
    case TXN_TRANSFER_OUT: return "TRANSFER_OUT";
    case TXN_TRANSFER_IN:  return "TRANSFER_IN";
    case TXN_LOAN_CREDIT:  return "LOAN_CREDIT";
    case TXN_OPENING:      return "OPENING";
    default:               return "UNKNOWN";
    }
}

const char *feedback_status_name(int status)
{
    return status == FEEDBACK_REVIEWED ? "REVIEWED" : "NEW";
}

int valid_name(const char *s)
{
    size_t n = strlen(s);
    if (n == 0 || n >= NAME_LEN) return 0;
    for (; *s; s++)
        if (!isalpha((unsigned char)*s) && *s != ' ' && *s != '.' && *s != '-' && *s != '\'')
            return 0;
    return 1;
}

int valid_phone(const char *s)
{
    size_t n = strlen(s);
    if (n < 7 || n >= PHONE_LEN) return 0;
    for (size_t i = 0; i < n; i++)
        if (!isdigit((unsigned char)s[i]) && !(i == 0 && s[i] == '+')) return 0;
    return 1;
}

int valid_email(const char *s)
{
    size_t n = strlen(s);
    if (n < 5 || n >= EMAIL_LEN) return 0;
    const char *at = strchr(s, '@');
    if (!at || at == s || strchr(at + 1, '@')) return 0;
    const char *dot = strrchr(at, '.');
    if (!dot || dot == at + 1 || dot[1] == '\0') return 0;
    for (; *s; s++) if (isspace((unsigned char)*s)) return 0;
    return 1;
}

int valid_password(const char *s)
{
    size_t n = strlen(s);
    if (n < 4 || n > 32) return 0;
    for (; *s; s++) if (!isgraph((unsigned char)*s)) return 0;
    return 1;
}
