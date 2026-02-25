#include "storage.h"
#include "socket.h"
#include "message.h"
#include "util.h"

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

data_node_t* find_node_with_key(char* key) {
  // find data_node_t
  // naive linear search (for now)
  data_node_t* curr = data_head;
  while (curr != NULL) {
    if (memcmp(curr->hash, key, HASH_LENGTH) == 0) {
      return curr;
    }

    curr = curr->next;
  }

  // not found, return NULL
  return NULL;
}

// container functionalities for client commands
void set(char* props) {
  FILE* log_file = fopen("storage_node_set_log.txt", "a");
  fprintf(log_file, "%lu ", get_curr_time());

  // SET message format: <KEY_HASH><VAL_LEN>#<VAL>
  // key length is fixed at HASH_LENGTH
  char* key = props;
  char* value_len_str = props + HASH_LENGTH;
  
  size_t value_len = atoi(value_len_str);
  if (value_len > MAX_MESSAGE_LENGTH) {
    fprintf(stderr, "%sinvalid value length: %zu; aborting\n", STORAGE_EVENTS_LOG_PREFIX, value_len);
    fclose(log_file);
    return;
  }

  // find '#' separator
  char* value = value_len_str;
  while (*value != '#' && *value != '\0') value++;
  value++; // skip '#'

  // terminate key string
  key[HASH_LENGTH] = '\0';

  // copy value
  // another issue here: need to null terminate
  char* copied_value = (char*) malloc(sizeof(char) * (value_len + 1));
  strncpy(copied_value, value, value_len);
  copied_value[value_len] = '\0';

  // make new data_node_t
  data_node_t* new_data = (data_node_t*) malloc(sizeof(data_node_t));
  strncpy(new_data->hash, key, HASH_LENGTH);
  new_data->value = copied_value;
  new_data->value_len = value_len;
  // malloc does not guarantee zeroed out data...
  // so sometimes find_node_with_key would assume some node had a
  //  next pointer at some junk address when it didn't
  new_data->next = NULL;

  // insert into list
  insert_data(new_data);
  fprintf(log_file, "%lu\n", get_curr_time());
  fclose(log_file);
}

const char* get(char* props) {
  FILE* log_file = fopen("storage_node_get_log.txt", "a");
  fprintf(log_file, "%lu ", get_curr_time());

  // GET message format: <KEY_HASH>
  // sanity check
  props[HASH_LENGTH] = '\0';
  data_node_t* node_with_key = find_node_with_key(props);

  fprintf(log_file, "%lu\n", get_curr_time());
  fclose(log_file);

  // not found, return NULL
  return node_with_key != NULL ? node_with_key->value : NULL;
}

int del(char* props) {
  // DEL message format: <KEY_HASH>
  if (data_head == NULL) return -1;

  unsigned char hashed_key[HASH_LENGTH + 1] = { 0 };
  hash_string(props, hashed_key); 

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
