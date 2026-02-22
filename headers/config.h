#pragma once

// Log Prefixes
#define STORAGE_REGULAR_EVENT_LOG_PREFIX "dstore storage: "
#define STORAGE_TRANSFER_EVENT_LOG_PREFIX "dstore transfer: "

#define MESSAGE_PREFIX_LENGTH 3

// Message Types
enum message_type {
    SET,
    GET,
    DEL,
    TRANSFER_REQ_FROM_SERVER,
    TRANSFER_RESPONSE_FROM_CONTAINER
};

char* MESSAGE_PREFIXES[] = {
    // MUST BE MESSAGE_PREFIX_LENGTH CHARACTERS LONG
    // + IN THE SAME ORDER AS message_type_t
    "SET",
    "GET",
    "DEL",
    "TRF",
    "TRR"
};
