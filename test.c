#include "client.h"
#include "config.h"

#include "words.c"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

  
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s <num_tests> seed?\n", argv[0]);
    return 1;
  }
  
  int num_tests = atoi(argv[1]);
  int default_seed = argc == 2 ? time(NULL) : atoi(argv[2]);
  
  // Citation: https://stackoverflow.com/a/822368
  srand(default_seed);

  FILE*log_file = fopen("test_log.txt", "w");

  // for storing random keys and vals
  char* key;
  char* value;
  char value_out[MAX_VALUE_LENGTH];

  while (num_tests--) {
    int op = rand() % 2; // DEL tests are disabled

    switch (op)
    {
    case 0:
      // SET
      key = WORDS[rand() % NUM_WORDS];
      value = WORDS[rand() % NUM_WORDS];

      dset(key, value);
      fprintf(log_file, "SET: %s -> %s\n", key, value);
      break;
    
    case 1:
      // GET
      key = WORDS[rand() % NUM_WORDS];
      
      dget(key, value_out);
      fprintf(log_file, "GET: %s -> %s\n", key, value_out);
      break;

    case 2:
      // DEL
      // this test is disabled since its not in scope
      // there is a memory issue in workers/storage.c for DEL
      key = WORDS[rand() % NUM_WORDS];

      ddel(key);
      fprintf(log_file, "DEL: %s\n", key);
      break;
    
    default:
      // unreachable
      break;
    }

    // Citation: https://www.geeksforgeeks.org/c/sleep-function-in-c/
    // without sleeping (at least 50ms), the server is overwhelmed
    usleep(50000);
  }

  fclose(log_file);
  return 0;
}
