#include "headers/storage.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main() {
  set("key", "value");
  set("key2", "another value");
  set("key", "not the original value");
  set("key3", "even more values?");
  set("key4", "does it keep going down?");
  set("key5", "i think it does");

  printf("testing key2\n");
  const char* key2_value = get("key2");
  assert(key2_value != NULL && strcmp(key2_value, "another value") == 0);
  printf("testing key again\n");
  const char* key_value = get("key");
  assert(key_value != NULL && strcmp(key_value, "not the original value") == 0);
  printf("done\n");
  
  return 0;
}