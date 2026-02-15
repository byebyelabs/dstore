#include <openssl/evp.h>
#include <string.h>

void hash_string(const char* str, unsigned char* hash_out) {
  EVP_Q_digest(NULL, "MD5", NULL, str, strlen(str), hash_out, NULL);
}