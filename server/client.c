#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "config.h"
#include "message.h"
#include "socket.h"


// only testing on a single machine
const char *host = "localhost";
uint16_t port = DEFAULT_BALANCER_PORT;
int BALANCER_FD = -1;

void connect_to_balancer() {;
  if ((BALANCER_FD = socket_connect((char *)host, port)) == -1) {
    perror("dstore: connect");
    exit(EXIT_FAILURE);
  }
}

void dset(char *key, char *value) {
  connect_to_balancer();

  // body is "<key> <value>"
  char body[MAX_MESSAGE_LENGTH];
  snprintf(body, sizeof(body), "%s %s", key, value);
  send_message(BALANCER_FD, SET, body);

  close(BALANCER_FD);
}

void dget(char *key, char *value_out) {
  connect_to_balancer();

  if (send_message(BALANCER_FD, GET, (char *)key) != 0) {
    close(BALANCER_FD);
    return;
  }

  Message *resp = receive_message(BALANCER_FD);
  close(BALANCER_FD);

  if (resp == NULL) {
    perror("dstore_get: receive_message got NULL");
    return;
  }

  if (resp->type != VAL) {
    perror("dstore_get: expected VAL response");
    free(resp);
    return;
  }

  strcpy(value_out, resp->body);
  free(resp);
}

void ddel(char *key) {
  connect_to_balancer();
  send_message(BALANCER_FD, DEL, key);
  close(BALANCER_FD);
}
