#pragma once

// Ports
#define DEFAULT_BALANCER_PORT 42069

// Log Prefixes
#define STORAGE_REGULAR_EVENT_LOG_PREFIX "dstore storage: "
#define STORAGE_TRANSFER_EVENT_LOG_PREFIX "dstore transfer: "
#define SERVER_EVENTS_LOG_PREFIX "dstore server: "

#define MESSAGE_PREFIX_LENGTH 3
#define MAX_MESSAGE_LENGTH 1024

// Message Types
enum message_type {
    SET,  // set key value pair
    GET,  // get value for key
    DEL,  // delete key
    JOIN, // storage node joins the balancer
    VAL,  // storage node returns a value
};

// In order of type, must be MESSAGE_PREFIX_LENGTH chars each
#define MESSAGE_PREFIXES { "SET", "GET", "DEL", "JNS", "VAL" };
