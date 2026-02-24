#include "server.h"
#include "socket.h"
#include "message.h"

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
    fprintf(stderr, "%sring is empty!\n", SERVER_EVENTS_LOG_PREFIX);
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

  fprintf(stderr, "%spredecessor not found!\n", SERVER_EVENTS_LOG_PREFIX);
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

    printf("%sFirst node joined. Ring initialized.\n", SERVER_EVENTS_LOG_PREFIX);
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

  printf("%sNode joined ring successfully.\n", SERVER_EVENTS_LOG_PREFIX);

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

  // Case 2: this was the last node left in the ring
  if (node->successor == node) {
    ring_head = NULL;
    printf("%sLast node left. Ring is now empty.\n", SERVER_EVENTS_LOG_PREFIX);
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

  printf("%sNode removed from topology.\n", SERVER_EVENTS_LOG_PREFIX);

  pthread_mutex_unlock(&ring_lock);
  return 0;
}

// helper for connecting to a storage node
int connect_to_storage_node(storage_node_t *node) {
  int fd = socket_connect(node->ip, node->port);
  if (fd == -1) {
    fprintf(stderr, "%sFailed to connect to storage node at %s:%d.\n",
            SERVER_EVENTS_LOG_PREFIX, node->ip, node->port);
  }
  return fd;
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
            "%sSET failed: could not find node for key '%s'.\n",
            SERVER_EVENTS_LOG_PREFIX, key);
    return -1;
  }

  char *payload = (char *)malloc(MAX_MESSAGE_LENGTH);
  if (payload == NULL) {
    fprintf(stderr, "%sSET failed: could not allocate memory for payload.\n",
            SERVER_EVENTS_LOG_PREFIX);
    return -1;
  }

  // SET format: "<KEY_HASH><VAL_LEN>#<VAL>
  unsigned char key_hash_str[HASH_LENGTH + 1];
  hash_string(key, key_hash_str);
  key_hash_str[HASH_LENGTH] = '\0';

  snprintf(payload, MAX_MESSAGE_LENGTH, "%s%zu#%s", key_hash_str, strlen(value), value);
  int fd = connect_to_storage_node(&target_node);
  if (fd == -1) {
    fprintf(stderr, "%sSET failed: could not connect to %s:%d.\n",
            SERVER_EVENTS_LOG_PREFIX, target_node.ip, target_node.port);
    free(payload);
    return -1;
  }

  int rc = send_message(fd, SET, payload);
  close(fd);
  free(payload);
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
            "%sGET failed: could not find node for key '%s'.\n",
            SERVER_EVENTS_LOG_PREFIX, key);
    return -1;
  }

  int fd = connect_to_storage_node(&target_node);
  if (fd == -1) {
    fprintf(stderr, "%sGET failed: could not connect to %s:%d.\n",
            SERVER_EVENTS_LOG_PREFIX, target_node.ip, target_node.port);
    return -1;
  }

  // THIS IS WHAT CAUSED SEGV BRUUUHH
  // key hash was not terminated bc its raw
  unsigned char key_hash_str[HASH_LENGTH + 1];
  memcpy(key_hash_str, key_hash, HASH_LENGTH);
  key_hash_str[HASH_LENGTH] = '\0';

  if (send_message(fd, GET, (char *)key_hash_str) != 0) {
    close(fd);
    return -1;
  }

  // storage node replies with a VAL message: "VAL:<value>"
  Message* response = receive_message(fd);
  close(fd);
  
  if (response == NULL) return -1;
  if (response->type != VAL) {
    fprintf(stderr, "%sGET failed: expected VAL response, got %.3s.\n",
      SERVER_EVENTS_LOG_PREFIX, response->body);
      free(response);
      return -1;
    }
    
  strcpy(value_buffer, response->body);
  free(response);
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
            "%sDEL failed: could not find node for key '%s'.\n",
            SERVER_EVENTS_LOG_PREFIX, key);
    return -1;
  }
  
  int fd = connect_to_storage_node(&target_node);
  if (fd == -1) {
    fprintf(stderr, "%sDEL failed: could not connect to %s:%d.\n",
            SERVER_EVENTS_LOG_PREFIX, target_node.ip, target_node.port);
    return -1;
  }

  int rc = send_message(fd, DEL, (char *)key);
  close(fd);
  return rc;
}