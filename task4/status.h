#ifndef STATUS_H
#define STATUS_H

typedef enum {
    SUCCESS = 0,
    ERROR_NULL_POINTER,
    ERROR_INVALID_DATA,
    ERROR_MEMORY_ALLOCATION,
    ERROR_OVERFLOW,
    ERROR_LIMIT,
    ERROR_IO
} status;

#endif
