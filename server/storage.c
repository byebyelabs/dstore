#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "config.h"
#include "workers/storage.c"

int main() {
  // Set up a server socket to accept incoming connections
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if (server_socket_fd == -1) {
    perror("failed to open port");
    exit(EXIT_FAILURE);
  }
  if (listen(server_socket_fd, 1) == -1) {
    perror("failed to listen on server socket");
    exit(EXIT_FAILURE);
  }

  printf("%slistening on port %d\n", STORAGE_EVENTS_LOG_PREFIX, port);

  // spawn listener thread
  pthread_t request_handler_thread;
  pthread_create(&request_handler_thread, NULL, request_worker, (void*) (long) server_socket_fd);

  // wait for listener thread
  pthread_join(request_handler_thread, NULL);

  return 0;
}

void* request_worker(void* input) {
  printf("%swaiting for requests...\n", STORAGE_EVENTS_LOG_PREFIX); 
  // cast input back to server_socket_fd
  int server_socket_fd = (int) (long) input;

  // repeatedly accept connections on server socket
  int client_socket_fd;
  while ((client_socket_fd = server_socket_accept(server_socket_fd)) != -1) {
    printf("%saccepted connection from client\n", STORAGE_EVENTS_LOG_PREFIX);

    Message* request = receive_message(client_socket_fd);
    if (request == NULL) {
      printf("%sfailed to receive message from client\n", STORAGE_EVENTS_LOG_PREFIX);
      close(client_socket_fd);
      continue;
    }

    if (request->type == SET) {
      set(request->body);
    } else if (request->type == DEL) {
      del(request->body);
    } else if (request->type == GET) {
      // only request that requires a response
      const char* value = get(request->body);
      send_message(client_socket_fd, VAL, (value != NULL ? (char*) value : ""));
    } else {
      printf("%sshould be unreachable: #1\n", STORAGE_EVENTS_LOG_PREFIX);
    }

    free(request);
  }
  
  return NULL;
}
