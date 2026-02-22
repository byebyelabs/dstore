#include "headers/storage.h"
#include "headers/socket.h"
#include "headers/message.h"
#include "headers/util.h"

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

    // data->hash < curr->hash
    // should insert here
    if (cmp_result < 0) {
      if (prev == NULL) data_head = data;
      else prev->next = data;
      data->next = curr;
      return;
    }

    // otherwise data->hash > curr->hash
    // can't insert here, so continue
    prev = curr;
    curr = curr->next;
  }

  // reached end of list, so insert at end
  // notice prev cannot be null since we checked the list is not length 0
  prev->next = data;
}

data_node_t* find_node_with_key(const char* key) {
  // hash key
  unsigned char hashed_key[HASH_LENGTH + 1] = { 0 };
  hash_string(key, hashed_key); 

  // find data_node_t
  // naive linear search (for now)
  data_node_t* curr = data_head;
  while (curr != NULL) {
    if (memcmp(curr->hash, hashed_key, HASH_LENGTH) == 0) {
      return curr;
    }

    curr = curr->next;
  }

  // not found, return NULL
  return NULL;
}

// debug function for printing lists
void print_list() {
  data_node_t* curr = data_head;
  while (curr != NULL) {
    printf("key: <");
    for (int i = 0; i < HASH_LENGTH; i++) {
      printf("%02x", curr->hash[i]);
    }
    printf(">, value: < %s >, next: < %p >\n", curr->value, curr->next);
    curr = curr->next;
  }
}

// container functionalities for client commands
void set(char* props) {
  // SET message format: <KEY_HASH><VAL_LEN>#<VAL>
  // key length is fixed at HASH_LENGTH
  char* key = props;
  char* value_len_str = props + HASH_LENGTH;
  
  size_t value_len = atoi(value_len_str);
  char* value = value_len_str + strlen(value_len_str) + 1;

  // terminate key string
  key[HASH_LENGTH] = '\0';

  // copy value
  char* copied_value = (char*) malloc(sizeof(char) * value_len);
  strncpy(copied_value, value, value_len);

  // make new data_node_t
  data_node_t* new_data = (data_node_t*) malloc(sizeof(data_node_t));
  strncpy(new_data->hash, key, HASH_LENGTH);
  new_data->value = copied_value;
  new_data->value_len = value_len;

  // insert into list
  insert_data(new_data);
}

const char* get(const char* key) {
  // print_list();
  data_node_t* node_with_key = find_node_with_key(key);

  // not found, return NULL
  return node_with_key != NULL ? node_with_key->value : NULL;
}

int del(const char* key) {
  if (data_head == NULL) return -1;

  unsigned char hashed_key[HASH_LENGTH + 1] = { 0 };
  hash_string(key, hashed_key); 

  data_node_t* prev = NULL;
  data_node_t* curr = data_head;
  while (curr != NULL) {
    if (memcmp(hashed_key, curr->hash, HASH_LENGTH) == 0) {
      // delete key
      if (prev == NULL) data_head = curr->next;
      else prev->next = curr->next;
      free(curr->value);
      free(curr);
      return 0;
    }
    
    prev = curr;
    curr = curr->next;
  }

  return -1;
}

// container functionalities for C2C transfers
void handle_incoming_transfer(const char* message) {
  // TODO
}

void handle_transfer_request(const char* message) {
  // TODO
}