#pragma once

#include "storage.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_SERVER_PORT DEFAULT_STORAGE_PORT
#define MAX_HOST_STR_LEN 16

// Storage Node in the ring
typedef struct storage_node {
  unsigned char hash[HASH_LENGTH];
  char host[MAX_HOST_STR_LEN];
  uint16_t port;
  struct storage_node *successor;
  struct storage_node *predecessor;
} storage_node_t;

/**
Finds the storage node responsible for a given hash.
This is the first node with hash >= key hash.
If no such node exists, it wraps around to the first node in the ring.

Args:
  hash: pointer to the hash of the key to find
  result: pointer to the storage node to store the result

Returns:
  0 on success
  -1 on failure
*/
int find_successor(unsigned char *hash, storage_node_t *result);

/*
  Storage-facing API
*/

/**
Joins given node to the ring.

Args:
  node: pointer to the storage node joining ring.

Returns:
  0 on success
  -1 on failure
*/
int join_ring(storage_node_t *node);

/*
  Client-facing API
*/

// saves key-value pair to correct storage node
int dstore_set(const char *key, const char *value);

// gets key-value pair, saves value in value_buffer
int dstore_get(const char *key, char *value_buffer);

// deletes key-value pair from correct storage node
int dstore_del(const char *key);