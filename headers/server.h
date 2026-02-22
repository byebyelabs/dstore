#pragma once

#include "storage.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_SERVER_PORT DEFAULT_STORAGE_PORT
#define SERVER_LOG_PREFIX "dstore server: "

// Storage Node in the ring
typedef struct storage_node {
  unsigned char hash[HASH_LENGTH];
  char ip[16];                    // e.g. "10.0.0.1"
  uint16_t port;                  // e.g. 8001
  struct storage_node *successor; // next node in ring (sorted by hash)
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

/**
Finds the storage node just before a given hash in the ring.
This is the node previous to the first node with hash >= key hash.
If no such node exists, it wraps around to the last node in the ring.

Args:
  hash: pointer to the hash of the key to find
  result: pointer to the storage node info to store the result

Returns:
  0 on success
  -1 on failure
*/
int find_predecessor(unsigned char *hash, storage_node_t *result);

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

/**
Leaves the ring, transferring existing data to predecessor.

Args:
  node: pointer to the storage node to leave the ring at

Returns:
  0 on success
  -1 on failure
*/
int leave_ring(storage_node_t *node);

/*
  Client-facing API
*/

// saves key-value pair to correct storage node
int dstore_set(const char *key, const char *value);

// gets key-value pair, saves value in value_buffer
int dstore_get(const char *key, char *value_buffer);

// deletes key-value pair from correct storage node
int dstore_del(const char *key);

// debug: print all nodes in the ring
void print_ring(bool print_hashes);