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
    char *value = strtok(body, " ");

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

  int num_storage_nodes = 2;
  if (argc > 2) {
    fprintf(stderr, "Usage: %s ?num_storage_nodes[default:2]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (argc == 2) {
    num_storage_nodes = atoi(argv[1]);
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

  // accept storage node registrations
  while (num_storage_nodes--) {
    // use server_socket_accept
    int storage_node_socket_fd = server_socket_accept(server_fd);
    if (storage_node_socket_fd == -1) {
      perror("something went wrong with accept");
      continue;
    }

    Message *msg = receive_message(storage_node_socket_fd);
    if (msg == NULL) {
      perror("something went wrong with receive_message");
      close(storage_node_socket_fd);
      continue;
    }

    if (msg->type != JOIN) {
      fprintf(stderr, "%sunexpected message type during join phase: %.3s\n",
              SERVER_EVENTS_LOG_PREFIX, msg->body);
      free(msg);
      close(storage_node_socket_fd);
      continue;
    }

    // Message body format: "<IP>:<PORT>"
    char *ip = msg->body;
    char *port = strtok(msg->body, ":"); // trust this will come thru
    
    size_t ip_len = strlen(ip);
    ip[ip_len] = '\0';

    // add node to ring
    storage_node_t *node = malloc(sizeof(storage_node_t));
    strncpy(node->ip, ip, MAX_IP_STR_LEN);
    node->port = (uint16_t)atoi(port);
    
    // successor and predecessor will be set in join_ring
    join_ring(node);
    free(msg);
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
