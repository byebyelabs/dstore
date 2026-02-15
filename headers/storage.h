#pragma once

#include "util.h"

/**
 * underlying data structure to hold data
 * linked list sorted by hash
 */
typedef struct data_node {
  unsigned char hash[HASH_LENGTH + 1];
  char* value;
  struct data_node* next;
} data_node_t;

/**
 * set key value pair in storage node
 */
void set(const char* key, const char* value);

/**
 * get value from key
 * returns copy of value at key, should be freed
 */
const char* get(const char* key);

/**
 * deletes key
 */
void del(const char* key);

/**
 * insert data into list, modifying pointer
 * data will be inserted in sorted order
 */
void insert_data(data_node_t* data);