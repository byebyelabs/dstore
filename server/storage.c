#include "container.c"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int main() {
  // Set up a server socket to accept incoming connections
  // Try to assign it to DEFAULT_STORAGE_PORT
  unsigned short port = DEFAULT_STORAGE_PORT;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("failed to open port");
    exit(EXIT_FAILURE);
  }
  if (listen(server_socket_fd, 1) == -1) {
    perror("failed to listen on server socket");
    exit(EXIT_FAILURE);
  }

  printf("%slistening on port %d!\n", STORAGE_REGULAR_EVENT_LOG_PREFIX, port);

  // spawn listener thread
  pthread_t request_handler_thread;
  pthread_create(&request_handler_thread, NULL, request_worker, (void*) (long) server_socket_fd);

  // wait for listener thread
  pthread_join(request_handler_thread, NULL);

  return 0;
}

void* request_worker(void* input) {
  printf("%swaiting for requests...\n", STORAGE_REGULAR_EVENT_LOG_PREFIX); 
  // cast input back to server_socket_fd
  int server_socket_fd = (int) (long) input;

  // repeatedly accept connections on server socket
  int client_socket_fd;
  while ((client_socket_fd = server_socket_accept(server_socket_fd)) != -1) {
    char* request = receive_message(client_socket_fd);

    // Check MESSAGE_PREFIX_LENGTH characters to determine message type
    char message_prefix[MESSAGE_PREFIX_LENGTH + 1];
    strncpy(message_prefix, request, MESSAGE_PREFIX_LENGTH);
    message_prefix[MESSAGE_PREFIX_LENGTH] = '\0';

    char* request_props = request + MESSAGE_PREFIX_LENGTH;
    if (strcmp(message_prefix, MESSAGE_PREFIXES[SET]) == 0) {
      set(request_props);
    } else if (strcmp(message_prefix, MESSAGE_PREFIXES[GET]) == 0) {
      get(request_props);
    } else if (strcmp(message_prefix, MESSAGE_PREFIXES[DEL]) == 0) {
      del(request_props);
    } else if (strcmp(message_prefix, MESSAGE_PREFIXES[TRANSFER_REQ_FROM_SERVER]) == 0) {
      handle_transfer_request(request_props);
    } else if (strcmp(message_prefix, MESSAGE_PREFIXES[TRANSFER_RESPONSE_FROM_CONTAINER]) == 0) {
      handle_incoming_transfer(request_props);
    }

    free(request);
  }
  
  return NULL;
}
