#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "message.h"

char* MESSAGE_PREFIXES_ARR[] = MESSAGE_PREFIXES;

// Send a across a socket with a header that includes the message length.
int send_message(int fd, enum message_type type, char* message) {
  // If the message is NULL, set errno to EINVAL and return an error
  if (message == NULL) {
    errno = EINVAL;
    return -1;
  }
  
  size_t len = strlen(message) + MESSAGE_PREFIX_LENGTH + 1; // +1 for the colon separator
  char full_message[MAX_MESSAGE_LENGTH];

  snprintf(
    full_message,
    len + 1,
    "%s:%s",
    MESSAGE_PREFIXES_ARR[type],
    message
  );

  // First, send the length of the message in a size_t
  if (write(fd, &len, sizeof(size_t)) != sizeof(size_t)) {
    // Writing failed, so return an error
    return -1;
  }

  // Now we can send the message. Loop until the entire message has been written.
  size_t bytes_written = 0;
  while (bytes_written < len) {
    // Try to write the entire remaining message
    ssize_t rc = write(fd, full_message + bytes_written, len - bytes_written);

    // Did the write fail? If so, return an error
    if (rc <= 0) return -1;

    // If there was no error, write returned the number of bytes written
    bytes_written += rc;
  }

  return 0;
}

// Receive a message from a socket and return the message (which must be freed later)
Message* receive_message(int fd) {
  // First try to read in the message length
  size_t len;
  if (read(fd, &len, sizeof(size_t)) != sizeof(size_t)) {
    // Reading failed. Return an error
    return NULL;
  }

  // Now make sure the message length is reasonable
  if (len > MAX_MESSAGE_LENGTH) {
    errno = EINVAL;
    return NULL;
  }

  // Allocate space for the message and a null terminator
  Message* result = malloc(sizeof(Message));

  // read the first MESSAGE_PREFIX_LENGTH chars to determine message type, then read the rest of the message body
  char prefix[MESSAGE_PREFIX_LENGTH + 1];
  ssize_t rc = read(fd, prefix, MESSAGE_PREFIX_LENGTH);
  if (rc != MESSAGE_PREFIX_LENGTH) {
    free(result);
    return NULL;
  }
  prefix[MESSAGE_PREFIX_LENGTH] = '\0';

  // Find the message type based on the prefix
  result->type = -1;
  for (int i = 0; i < 5; i++) {
    if (strcmp(prefix, MESSAGE_PREFIXES_ARR[i]) == 0) {
      result->type = i;
      break;
    }
  }

  if (result->type == -1) {
    // could not find a valid message type
    free(result);
    errno = EINVAL;
    return NULL;
  }

  // len is now the actual body length
  len -= MESSAGE_PREFIX_LENGTH + 1; 
  size_t bytes_read = 0;

  // discard ":"
  char discard;
  if (read(fd, &discard, 1) != 1) {
    free(result);
    return NULL;
  }

  while (bytes_read < len) {
    // Try to read the entire remaining message
    ssize_t rc = read(fd, result->body + bytes_read, len - bytes_read);

    // Did the read fail? If so, return an error
    if (rc <= 0) {
      free(result);
      return NULL;
    }

    // Update the number of bytes read
    bytes_read += rc;
  }

  // Add a null terminator to the message
  result->body[len] = '\0';
  return result;
}
