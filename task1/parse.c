#include "parse.h"

#include <ctype.h>
#include <limits.h>
#include <stddef.h>

status parse_flag(const char *text, char *out)
{
    if (text == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    if (text[0] != '-' || text[1] == '\0' || text[2] != '\0') {
        return ERROR_UNKNOWN_FLAG;
    }
    switch (text[1]) {
    case 'l':
    case 'r':
    case 'u':
    case 'n':
    case 'c':
        *out = text[1];
        return SUCCESS;
    default:
        return ERROR_UNKNOWN_FLAG;
    }
}

status parse_seed(const char *text, unsigned int *out)
{
    unsigned int value = 0;
    size_t i;
    if (text[0] == '+') {
        i = 1;
    }

    if (text == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    if (text[0] == '\0') {
        return ERROR_INVALID_NUMBER;
    }
    for (i = 0; text[i] != '\0'; i++) {
        unsigned int digit;
        if (!isdigit((unsigned char)text[i])) {
            return ERROR_INVALID_NUMBER;
        }
        digit = (unsigned int)(text[i] - '0');
        if (value > (UINT_MAX - digit) / 10) {
            return ERROR_INVALID_NUMBER;
        }
        value = value * 10 + digit;
    }
    *out = value;
    return SUCCESS;
}