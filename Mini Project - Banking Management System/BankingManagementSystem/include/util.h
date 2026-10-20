/*
 * util.h - Small helpers shared by server, client and tools.
 */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>
#include "records.h"

#define MAX_TXN_AMOUNT  (100000000LL * 100)   /* Rs. 10 crore per operation, in paise */
#define MAX_LOAN_AMOUNT (10000000LL * 100)    /* Rs. 1 crore per loan                 */

/* Safe string copy that always NUL-terminates (unlike strncpy). */
void str_copy(char *dst, const char *src, size_t cap);

/* Remove leading/trailing whitespace in place. */
void str_trim(char *s);

/* Append formatted text to a buffer of total capacity cap (never overflows). */
void appendf(char *buf, size_t cap, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* "1234.5" -> 123450 paise. Returns 0 on success, -1 if not a valid
 * positive amount with at most 2 decimals. */
int parse_money(const char *s, int64_t *paise);

/* 12345678 paise -> "Rs. 1,23,456.78" (Indian digit grouping). */
void format_money(int64_t paise, char *buf, size_t cap);

/* epoch seconds -> "2026-10-20 14:05:09" (local time). */
void format_time(int64_t t, char *buf, size_t cap);

/* Parse a whole decimal integer. Returns 0 on success. */
int parse_int(const char *s, int *out);

const char *role_name(int role);
const char *status_name(int status);
const char *loan_status_name(int status);
const char *txn_type_name(int type);
const char *feedback_status_name(int status);

/* Input validators: return 1 when valid. */
int valid_name(const char *s);
int valid_phone(const char *s);
int valid_email(const char *s);
int valid_password(const char *s);   /* 4..32 printable chars, no spaces */

#endif
