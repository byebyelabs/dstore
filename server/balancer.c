#include "headers/server.h"
#include "headers/socket.h"
#include "headers/message.h"
#include "headers/config.h"


void* handle_client(int fd) {
  Message *msg = receive_message(fd);
  if (msg == NULL) {
    close(fd);
    return NULL;
  }

  char *body = msg->body;
  if (msg->type == SET) {
    // user will separate key and value with ' '
    char *key = body;
    int space = 0;

    while (body[space] != ' ' && body[space] != '\0') space++;
    if (body[space] == '\0') {
      fprintf(stderr, "%sinvalid SET message body: expected 'key value', got '%s'\n",
              SERVER_EVENTS_LOG_PREFIX, body);
      free(msg);
      close(fd);
      return NULL;
    }

    key[space] = '\0';
    char *value = body + space + 1;

    dstore_set(key, value);
  } else if (msg->type == GET) {
    char* value_buffer = malloc(sizeof(char) * MAX_VALUE_LENGTH);
    if (dstore_get(body, value_buffer) == 0) {
      send_message(fd, VAL, value_buffer);
    }
    free(value_buffer);
  } else if (msg->type == DEL) {
    dstore_del(body);
  }

  free(msg);
  close(fd);
  return NULL;
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "  Usage: %s [host1:port1 ...] (one required)\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  unsigned short port = DEFAULT_BALANCER_PORT;
  int server_fd = server_socket_open(&port);
  if (server_fd == -1) {
    perror("server_socket_open");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 128) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("%sbalancer listening on port %d\n", SERVER_EVENTS_LOG_PREFIX, port);

  // connect to each storage node and register it in the ring
  for (int i = 1; i < argc; i++) {
    // parse "host:port" argument
    char* host = argv[i];
    int colon = 0;
    while (host[colon] != ':' && host[colon] != '\0') colon++;

    if (host[colon] == '\0') {
      fprintf(stderr, "invalid storage node address: %s\n", host);
      continue;
    }

    host[colon] = '\0';
    int port = atoi(host + colon + 1);

    // verify we can reach the storage node
    int fd = socket_connect(host, port);
    if (fd == -1) {
      fprintf(stderr, "%sfailed to connect to storage node at %s:%d\n",
              SERVER_EVENTS_LOG_PREFIX, host, port);
      continue;
    }
    close(fd);

    // build storage_node_t and hash it by "host:port"
    storage_node_t *node = malloc(sizeof(storage_node_t));
    strncpy(node->ip, host, MAX_IP_STR_LEN - 1);
    node->ip[MAX_IP_STR_LEN - 1] = '\0';
    node->port = port;

    char node_id[MAX_IP_STR_LEN + 8];
    snprintf(node_id, sizeof(node_id), "%s:%d", node->ip, node->port);
    hash_string(node_id, node->hash);

    join_ring(node);
    printf("%sregistered storage node at %s:%d\n",
           SERVER_EVENTS_LOG_PREFIX, node->ip, port);
  }

  // nodes are ready to store, now listen for client requests
  int client_fd;
  while ((client_fd = server_socket_accept(server_fd)) != -1) {
    // handle client requests synchronously since they are short lived
    handle_client(client_fd);
  }

  close(server_fd);
  return 0;
}
