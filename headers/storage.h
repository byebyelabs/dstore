#pragma once

#include "util.h"
#include "config.h"

#define DEFAULT_STORAGE_PORT 42069

/**
 * Structs
 */

// underlying data structure to hold data
// linked list sorted by hash
typedef struct data_node {
  unsigned char hash[HASH_LENGTH + 1];
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
void set(const char* key, const char* value);

// get value from key
// returns copy of value at key, should be freed
const char* get(const char* key);

// deletes key
// returns 0 on success, -1 on failure to find node with key
int del(const char* key);

// insert data into list, modifying pointer
// data will be inserted in sorted order
void insert_data(data_node_t* data);

// finds data_node_t associated with key
data_node_t* find_node_with_key(const char* key);

// debug function for printing lists
void print_list();

// transfer all keys >= hash to target IP address + port
// returns 0 on success, -1 on failure
int transfer_keys(const char* ip_address, unsigned short port, unsigned char* hash);

// handle incoming transfer message and store data
// message format: "TRANSFER:<hash_hex>:<val_length><value>:<hash_hex>:<val_length><value>:..."
void handle_transfer(const char* message);
