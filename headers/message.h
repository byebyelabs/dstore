#pragma once

#include "config.h"

typedef struct {
    enum message_type type;
    char body[MAX_MESSAGE_LENGTH];
} Message;

// Send a across a socket with a header that includes the message length. Returns non-zero value if
// an error occurs.
int send_message(int fd, enum message_type type, char* message);

// Receive a message from a socket and return the message string (which must be freed later).
// Returns NULL when an error occurs.
Message* receive_message(int fd);
