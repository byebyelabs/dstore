#include "headers/server.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

storage_node_t *ring_head = NULL; // head of sorted ring of storage nodes
pthread_mutex_t ring_lock = PTHREAD_MUTEX_INITIALIZER;

// print the ring, with optional hash values
void print_ring(bool print_hashes) {
  if (ring_head == NULL) {
    printf("| (empty ring) |\n");
    return;
  }

  printf("| ");
  storage_node_t *curr = ring_head;
  do {
    if (print_hashes) {
      printf("%s:%d (", curr->ip, curr->port);
      print_hash_hex(curr->hash);
      printf(") -> ");
    } else {
      printf("%s:%d -> ", curr->ip, curr->port);
    }
    curr = curr->successor;
  } while (curr != ring_head && curr != NULL);

  printf("(wraps to start) |\n");
}

// static helper for getting successor node pointer
static storage_node_t *_get_successor_ptr(const unsigned char *hash) {
  if (ring_head == NULL) {
    return NULL;
  }

  storage_node_t *curr = ring_head;

  do {
    if (hash_cmp(curr->hash, hash) >= 0) {
      // node hash >= key hash, so this node is responsible
      return curr;
    }
    curr = curr->successor;
  } while (curr != ring_head);

  // wrap-around case: hash is larger than all nodes
  return ring_head;
}

// find storage node post hash (responsible for values on join)
int find_successor(unsigned char *hash, storage_node_t *result) {
  storage_node_t *succ_ptr = _get_successor_ptr(hash);

  if (succ_ptr == NULL) {
    fprintf(stderr, SERVER_LOG_PREFIX "ring is empty!\n");
    return -1;
  }

  *result = *succ_ptr;
  return 0;
}

// find storage node pre hash (responsible for values on leave)
int find_predecessor(unsigned char *hash, storage_node_t *result) {
  storage_node_t successor_node;

  // predecessor of a hash is simply the predecessor of its successor!
  if (find_successor(hash, &successor_node) != 0) {
    return -1;
  }

  if (successor_node.predecessor != NULL) {
    // copy the data of the predecessor node
    *result = *(successor_node.predecessor);
    return 0;
  }

  fprintf(stderr, SERVER_LOG_PREFIX "predecessor not found!\n");
  return -1;
}

// join the ring, transferring existing data from successor
int join_ring(storage_node_t *node) {
  if (node == NULL) {
    return -1;
  }

  pthread_mutex_lock(&ring_lock);

  // Case 1: ring is completely empty
  if (ring_head == NULL) {
    node->successor = node;
    node->predecessor = node;
    ring_head = node;

    printf(SERVER_LOG_PREFIX "First node joined. Ring initialized.\n");
    pthread_mutex_unlock(&ring_lock);
    return 0;
  }

  // Case 2: ring has nodes, find where this node belongs
  storage_node_t *succ = _get_successor_ptr(node->hash);
  storage_node_t *pred = succ->predecessor;

  // update pointers for doubly linked list
  node->successor = succ;
  node->predecessor = pred;
  pred->successor = node;
  succ->predecessor = node;

  // if new node's hash < head hash, it's the new ring head
  if (hash_cmp(node->hash, ring_head->hash) < 0) {
    ring_head = node;
  }

  printf(SERVER_LOG_PREFIX "Node joined ring successfully.\n");

  // TODO: trigger data transfer from successor to node

  pthread_mutex_unlock(&ring_lock);
  return 0;
}

// leave the ring, transferring existing data to predecessor
int leave_ring(storage_node_t *node) {
  if (node == NULL) {
    return -1;
  }

  pthread_mutex_lock(&ring_lock);

  // Case 1: ring is empty
  if (ring_head == NULL) {
    pthread_mutex_unlock(&ring_lock);
    return -1;
  }

  // TODO: trigger data transfer to successor from node

  // Case 2: this was the last node left in the ring
  if (node->successor == node) {
    ring_head = NULL;
    printf(SERVER_LOG_PREFIX "Last node left. Ring is now empty.\n");
  }

  // Case 3: there are other nodes left in the ring
  else {
    storage_node_t *pred = node->predecessor;
    storage_node_t *succ = node->successor;
    pred->successor = succ;
    succ->predecessor = pred;

    // if this was the head node, move head to its successor
    if (ring_head == node) {
      ring_head = succ;
    }
  }

  // clean dangling pointers to existing nodes
  node->successor = NULL;
  node->predecessor = NULL;

  printf(SERVER_LOG_PREFIX "Node removed from topology.\n");

  pthread_mutex_unlock(&ring_lock);
  return 0;
}

// saves key-value pair to correct storage node
int dstore_set(const char *key, const char *value) {
  // hash key to find target node
  unsigned char key_hash[HASH_LENGTH];
  hash_string(key, key_hash);

  // find target node
  storage_node_t target_node;
  pthread_mutex_lock(&ring_lock);
  int res = find_successor(key_hash, &target_node);
  pthread_mutex_unlock(&ring_lock);

  if (res != 0) {
    fprintf(stderr,
            SERVER_LOG_PREFIX "SET failed: could not find node for key '%s'.\n",
            key);
    return -1;
  }

  // TODO: connect to storage node and set value

  return 0;
}

// gets key-value pair, saves value in value_buffer
int dstore_get(const char *key, char *value_buffer) {
  // hash key to find target node
  unsigned char key_hash[HASH_LENGTH];
  hash_string(key, key_hash);

  // find target node
  storage_node_t target_node;
  pthread_mutex_lock(&ring_lock);
  int res = find_successor(key_hash, &target_node);
  pthread_mutex_unlock(&ring_lock);

  if (res != 0) {
    fprintf(stderr,
            SERVER_LOG_PREFIX "GET failed: could not find node for key '%s'.\n",
            key);
    return -1;
  }

  // TODO: connect to storage node and get value
  // TODO: copy value to value_buffer

  return 0;
}

// deletes key-value pair from correct storage node
int dstore_del(const char *key) {
  // hash key to find target node
  unsigned char key_hash[HASH_LENGTH];
  hash_string(key, key_hash);

  // find target node
  storage_node_t target_node;
  pthread_mutex_lock(&ring_lock);
  int res = find_successor(key_hash, &target_node);
  pthread_mutex_unlock(&ring_lock);

  if (res != 0) {
    fprintf(stderr,
            SERVER_LOG_PREFIX "DEL failed: could not find node for key '%s'.\n",
            key);
    return -1;
  }

  // TODO: connect to storage node and delete key
  return 0;
}