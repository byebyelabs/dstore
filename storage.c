#include "headers/storage.h"
#include <stdlib.h>
#include <string.h>

data_node_t* data_head = NULL;

void insert_data(data_node_t* data) {
  if (data_head == NULL) {
    data_head = data;
    return;
  }

  // otherwise insert in sorted order naively
  data_node_t* prev = NULL;
  data_node_t* curr = data_head;
  while (curr != NULL) {
    int cmp_result = memcmp(data->hash, curr->hash, HASH_LENGTH);
    // data->hash == curr->hash
    // replace key and free unnecessary new node
    if (cmp_result == 0) {
      free(curr->value);
      curr->value = data->value;
      free(data);
      return;
    }

    // data->hash > curr->hash
    // should insert here
    if (cmp_result > 0) {
      if (prev == NULL) data_head = data;
      else prev->next = data;
      data->next = curr;
      return;
    }

    // otherwise data->hash < curr->hash
    // can't insert here, so continue
    prev = curr;
    curr = curr->next;
  }

  // reached end of list, so insert at end
  // notice prev cannot be null since we checked the list is not length 0
  prev->next = data;
}

void set(const char* key, const char* value) {
  // copy value
  size_t value_len = strlen(value) + 1;
  char* copied_value = (char*) malloc(sizeof(char) * value_len);
  strncpy(copied_value, value, value_len);

  // make new data_node_t
  data_node_t* new_data = (data_node_t*) malloc(sizeof(data_node_t));

  // set value
  new_data->value = copied_value;

  // hash key
  hash_string(key, new_data->hash);

  // insert into list
  insert_data(new_data);
}

// // debug function for printing lists
// void print_list() {
//   data_node_t* curr = data_head;
//   while (curr != NULL) {
//     printf("key: <");
//     for (int i = 0; i < HASH_LENGTH; i++) {
//       printf("%02x", curr->hash[i]);
//     }
//     printf(">, value: < %s >, next: < %p >\n", curr->value, curr->next);
//     curr = curr->next;
//   }
// }

const char* get(const char* key) {
  // hash key
  unsigned char hashed_key[HASH_LENGTH + 1] = { 0 };
  hash_string(key, hashed_key); 

  // find data_node_t
  // naive linear search (for now)
  data_node_t* curr = data_head;
  while (curr != NULL) {
    if (memcmp(curr->hash, hashed_key, HASH_LENGTH) == 0) {
      return curr->value;
    }

    curr = curr->next;
  }

  // not found, return NULL
  return NULL;
}