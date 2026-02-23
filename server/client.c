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

void connect() {;
  if ((BALANCER_FD = socket_connect((char *)host, port)) == -1) {
    perror("dstore: connect");
    exit(EXIT_FAILURE);
  }
}

void dset(char *key, char *value) {
  if (BALANCER_FD == -1) connect();

  // body is "<key> <value>"
  char body[MAX_MESSAGE_LENGTH];
  snprintf(body, sizeof(body), "%s %s", key, value);
  send_message(BALANCER_FD, SET, body);
}

void dget(char *key, char *value_out) {
  if (BALANCER_FD == -1) connect();

  if (send_message(BALANCER_FD, GET, (char *)key) != 0) {
    return NULL;
  }

  Message *resp = receive_message(BALANCER_FD);
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
  return value_out;
}

void ddel(char *key) {
  if (BALANCER_FD == -1) connect();
  send_message(BALANCER_FD, DEL, key);
}
