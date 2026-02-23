#pragma once

// Ports
#define DEFAULT_BALANCER_PORT 42069

// Log Prefixes
#define SERVER_EVENTS_LOG_PREFIX "dstore server: "
#define STORAGE_EVENTS_LOG_PREFIX "dstore storage: "

#define MESSAGE_PREFIX_LENGTH 3
#define MAX_MESSAGE_LENGTH 1024
#define MAX_VALUE_LENGTH 900 // conservative bound to fit in length

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
