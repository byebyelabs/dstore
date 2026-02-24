#include "headers/util.h"
#include <openssl/evp.h>
#include <string.h>

void hash_string(const char *str, unsigned char *hash_out) {
  EVP_Q_digest(NULL, "MD5", NULL, str, strlen(str), hash_out, NULL);
}

int hash_cmp(const unsigned char *a, const unsigned char *b) {
  return memcmp(a, b, HASH_LENGTH);
}

void print_hash_hex(const unsigned char *hash) {
  for (int i = 0; i < HASH_LENGTH; i++) {
    printf("%02x", hash[i]);
  }
}