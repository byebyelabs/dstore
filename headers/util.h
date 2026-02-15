#pragma once
#include <openssl/evp.h>

#define HASH_LENGTH 16

/**
 * hashes str with MD5 and places in hash_out
 */
void hash_string(const char* str, unsigned char* hash_out);