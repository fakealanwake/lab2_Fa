#ifndef STATUS_H
#define STATUS_H

typedef enum status {
    SUCCESS = 0,
    ERROR_NULL_POINTER,
    ERROR_INVALID_ARGUMENT,
    ERROR_FILE_OPEN,
    ERROR_FILE_READ,
    ERROR_MEMORY_ALLOCATION
} status;

#endif