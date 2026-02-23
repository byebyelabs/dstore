#pragma once

#include "util.h"
#include "config.h"

/**
 * Structs
 */

// underlying data structure to hold data
// linked list sorted by hash
typedef struct data_node {
  char hash[HASH_LENGTH + 1];
  size_t value_len;
  char* value;
  struct data_node* next;
} data_node_t;

// -----------------------------------------------

/**
 * Threads
 */

// thread that listens for commands (i.e. from the central server)
void* request_worker(void* input);

// -----------------------------------------------

/**
 * Helper functions
 */

// set key value pair in storage node
void set(char* props);

// get value from key
// returns copy of value at key, should be freed
const char* get(char* props);

// deletes key
// returns 0 on success, -1 on failure to find node with key
int del(char* props);

// insert data into list, modifying pointer
// data will be inserted in sorted order
void insert_data(data_node_t* data);

// finds data_node_t associated with key
data_node_t* find_node_with_key(const char* key);

// debug function for printing lists
void print_list();
