#pragma once
#include <openssl/evp.h>

#define HASH_LENGTH 32
#define HASH_ARR_LEN HASH_LENGTH + 1
#define RAW_HASH_LENGTH 16

/**
 * hashes str with MD5 and places in hash_out
 */
void hash_string(const char *str, char *hash_out);

/**
 * compares two hashes
 * returns <0, 0, >0 like memcmp
 */
int hash_cmp(const char *a, const char *b);
