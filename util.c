#include "headers/util.h"
#include <openssl/evp.h>
#include <string.h>
#include <sys/time.h>

void hash_string(const char *str, char *hash_out) {
  unsigned char raw_hash[RAW_HASH_LENGTH];
  size_t len;
  EVP_Q_digest(NULL, "MD5", NULL, str, strlen(str), raw_hash, &len);

  if (len != RAW_HASH_LENGTH) {
    fprintf(stderr, "!!!\nhash output was too long?\n!!!\n");
  }

  // convert raw hash to full hash
  for (int i = 0; i < RAW_HASH_LENGTH; i++) {
    snprintf(&hash_out[i * 2], sizeof(char) * (2 + 1), "%02x", raw_hash[i]);
  }
  hash_out[HASH_LENGTH] = '\0';
}

int hash_cmp(const char *a, const char *b) {
  return strncmp(a, b, HASH_LENGTH);
}

unsigned long get_curr_time() {
  // Citation: https://stackoverflow.com/questions/5833094/get-a-timestamp-in-c-in-microseconds
  struct timeval tv;
  gettimeofday(&tv,NULL);
  unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
  return time_in_micros;
}
