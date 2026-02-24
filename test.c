#include "client.h"
#include "config.h"

#include <stdio.h>

int main() {
  dset("hello", "julian");
  printf("set {hello: julian}\n");

  char value1[MAX_VALUE_LENGTH];
  dget("hello", value1);
  printf("got value: %s\n", value1);

  dset("hello", "devanshu");
  printf("set {hello: devanshu}\n");

  char value2[MAX_VALUE_LENGTH];
  dget("hello", value2);
  printf("got value: %s\n", value2);
}
