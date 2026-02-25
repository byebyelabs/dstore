#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "config.h"
#include "message.h"
#include "socket.h"
#include "util.h"

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
  FILE *log_file = fopen("set_log.txt", "a");
  fprintf(log_file, "%lu ", get_curr_time());
  connect_to_balancer();

  // body is "<key> <value>"
  char body[MAX_MESSAGE_LENGTH];
  snprintf(body, sizeof(body), "%s %s", key, value);
  send_message(BALANCER_FD, SET, body);

  close(BALANCER_FD);
  fprintf(log_file, "%lu\n", get_curr_time());
  fclose(log_file);
}

void dget(char *key, char *value_out) {
  FILE *log_file = fopen("get_log.txt", "a");
  fprintf(log_file, "%lu ", get_curr_time());

  connect_to_balancer();

  if (send_message(BALANCER_FD, GET, (char *)key) != 0) {
    close(BALANCER_FD);
    return;
  }

  Message *resp = receive_message(BALANCER_FD);
  close(BALANCER_FD);

  if (resp == NULL) {
    value_out[0] = '\0'; // set to empty string on failure
    return;
  }

  if (resp->type != VAL) {
    perror("dstore_get: expected VAL response");
    free(resp);
    return;
  }

  strcpy(value_out, resp->body);
  free(resp);
  fprintf(log_file, "%lu\n", get_curr_time());
  fclose(log_file);
}

void ddel(char *key) {
  connect_to_balancer();
  send_message(BALANCER_FD, DEL, key);
  close(BALANCER_FD);
}
