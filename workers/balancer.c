#include "server.h"
#include "socket.h"
#include "message.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

storage_node_t *ring_head_storage_node = NULL; // head of sorted ring of storage nodes
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

// static helper for getting successor node pointer
static storage_node_t *_get_successor_ptr(const unsigned char *hash) {
  if (ring_head_storage_node == NULL) {
    return NULL;
  }

  storage_node_t *curr = ring_head_storage_node;

  do {
    if (hash_cmp(curr->hash, hash) >= 0) {
      // node hash >= key hash, so this node is responsible
      return curr;
    }
    curr = curr->successor;
  } while (curr != ring_head_storage_node);

  // hash is larger than all nodes
  return ring_head_storage_node;
}

// find storage node post hash (responsible for values on join)
int find_successor(unsigned char *hash, storage_node_t *result) {
  storage_node_t *successor_node_ptr = _get_successor_ptr(hash);

  if (successor_node_ptr == NULL) {
    fprintf(stderr, "%sring is empty!\n", SERVER_EVENTS_LOG_PREFIX);
    return -1;
  }

  *result = *successor_node_ptr;
  return 0;
}

// join the ring, transferring existing data from successor
int join_ring(storage_node_t *node) {
  if (node == NULL) {
    return -1;
  }

  pthread_mutex_lock(&mutex_lock);

  // Case 1: ring is completely empty
  if (ring_head_storage_node == NULL) {
    // node is its own successor and predecessor
    node->successor = node;
    node->predecessor = node;

    // set node to the ring head
    ring_head_storage_node = node;

    printf("%sFirst storage node joined the ring.\n", SERVER_EVENTS_LOG_PREFIX);
    pthread_mutex_unlock(&mutex_lock);
    return 0;
  }

  // Case 2: ring has nodes, find where this node belongs
  storage_node_t *successor_node = _get_successor_ptr(node->hash);
  storage_node_t *predecessor_node = successor_node->predecessor;

  // make both point to the node
  predecessor_node->successor = node;
  successor_node->predecessor = node;

  // make node point to correct successor
  node->successor = successor_node;
  
  // make node point to correct predecessor
  node->predecessor = predecessor_node;

  printf("%sStorage node joined ring successfully.\n", SERVER_EVENTS_LOG_PREFIX);
  
  // if new node's hash < head hash, it's the new ring head
  if (hash_cmp(node->hash, ring_head_storage_node->hash) < 0) {
    ring_head_storage_node = node;
  }

  pthread_mutex_unlock(&mutex_lock);
  return 0;
}

// helper for connecting to a storage node
int connect_to_storage_node(storage_node_t *node) {
  int fd = socket_connect(node->host, node->port);
  if (fd == -1) {
    fprintf(stderr, "%sFailed to connect to storage node at %s:%d.\n",
            SERVER_EVENTS_LOG_PREFIX, node->host, node->port);
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
  pthread_mutex_lock(&mutex_lock);
  int res = find_successor(key_hash, &target_node);
  pthread_mutex_unlock(&mutex_lock);

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
            SERVER_EVENTS_LOG_PREFIX, target_node.host, target_node.port);
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
  pthread_mutex_lock(&mutex_lock);
  int res = find_successor(key_hash, &target_node);
  pthread_mutex_unlock(&mutex_lock);

  if (res != 0) {
    fprintf(stderr,
            "%sGET failed: could not find node for key '%s'.\n",
            SERVER_EVENTS_LOG_PREFIX, key);
    return -1;
  }

  int fd = connect_to_storage_node(&target_node);
  if (fd == -1) {
    fprintf(stderr, "%sGET failed: could not connect to %s:%d.\n",
            SERVER_EVENTS_LOG_PREFIX, target_node.host, target_node.port);
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
  pthread_mutex_lock(&mutex_lock);
  int res = find_successor(key_hash, &target_node);
  pthread_mutex_unlock(&mutex_lock);

  if (res != 0) {
    fprintf(stderr,
            "%sDEL failed: could not find node for key '%s'.\n",
            SERVER_EVENTS_LOG_PREFIX, key);
    return -1;
  }
  
  int fd = connect_to_storage_node(&target_node);
  if (fd == -1) {
    fprintf(stderr, "%sDEL failed: could not connect to %s:%d.\n",
            SERVER_EVENTS_LOG_PREFIX, target_node.host, target_node.port);
    return -1;
  }

  int rc = send_message(fd, DEL, (char *)key);
  close(fd);
  return rc;
}