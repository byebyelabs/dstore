#include "headers/util.h"
#include <openssl/evp.h>
#include <string.h>

void hash_string(const char *str, unsigned char *hash_out) {
  // Getting rid of MD5 hashing because it is too buggy for 
  // our use case. our hash = truncate key at 16 chars and
  // pad with ' ' to get 16 byte hash.
  memset(hash_out, ' ', HASH_LENGTH);
  int len = strlen(str);
  strncpy((char *)hash_out, str, len > HASH_LENGTH ? HASH_LENGTH : len);
}

int hash_cmp(const unsigned char *a, const unsigned char *b) {
  return memcmp(a, b, HASH_LENGTH);
}
