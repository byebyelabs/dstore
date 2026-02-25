#pragma once
#include <openssl/evp.h>

#define HASH_LENGTH 16

/**
 * hashes str with MD5 and places in hash_out
 */
void hash_string(const char *str, unsigned char *hash_out);

/**
 * compares two hashes
 * returns <0, 0, >0 like memcmp
 */
int hash_cmp(const unsigned char *a, const unsigned char *b);
