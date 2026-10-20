/*
 * password.h - Salted SHA-256 password hashing.
 * Passwords are never stored in plain text: we store a random salt and
 * SHA-256(salt + password). The salt comes from /dev/urandom (open/read).
 */
#ifndef PASSWORD_H
#define PASSWORD_H

#include <stddef.h>

void sha256_hex(const void *data, size_t len, char out[65]);
void make_salt(char out[17]);
void hash_password(const char *salt, const char *password, char out[65]);
int  check_password(const char *salt, const char *stored_hash, const char *password);

#endif
