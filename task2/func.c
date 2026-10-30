#include "func.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

static int digit_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

bool is_blank(const char *s) {
    if (s == NULL) return true;
    for (; *s != '\0'; ++s) {
        if (!isspace((unsigned char)*s)) return false;
    }
    return true;
}

/* Достаёт следующий токен (последовательность непробельных символов).
   Нет токена -> ERROR_INVALID_DATA, не влезает в буфер -> ERROR_TOKEN_TOO_LONG. */
status next_token(const char **pos, char *out, size_t out_size) {
    if (pos == NULL || *pos == NULL || out == NULL || out_size < 2) {
        return ERROR_INVALID_ARGUMENTS;
    }

    const char *p = *pos;
    while (*p != '\0' && isspace((unsigned char)*p)) ++p;
    if (*p == '\0') return ERROR_INVALID_DATA;

    size_t len = 0;
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        if (len + 1 >= out_size) return ERROR_TOKEN_TOO_LONG;
        out[len++] = *p++;
    }
    out[len] = '\0';
    *pos = p;
    return SUCCESS;
}

status parse_ull_base(const char *s, int base, unsigned long long *out) {
    if (s == NULL || out == NULL || base < 2 || base > 36 || s[0] == '\0') {
        return ERROR_INVALID_ARGUMENTS;
    }

    unsigned long long value = 0;
    for (size_t i = 0; s[i] != '\0'; ++i) {
        int d = digit_value(s[i]);
        if (d < 0 || d >= base) return ERROR_INVALID_ARGUMENTS;
        if (value > (ULLONG_MAX - (unsigned long long)d) / (unsigned long long)base) {
            return ERROR_OVERFLOW;
        }
        value = value * (unsigned long long)base + (unsigned long long)d;
    }

    *out = value;
    return SUCCESS;
}

/* Формат: цифры, опционально одна точка. Без знака, экспоненты, nan/inf, hex. */
status parse_salary(const char *str, double *out) {
    if (str == NULL || out == NULL || *str == '\0') {
        return ERROR_INVALID_ARGUMENTS;
    }

    size_t digits = 0;
    bool dot = false;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            ++digits;
        } else if (str[i] == '.' && !dot) {
            dot = true;
        } else {
            return ERROR_INVALID_ARGUMENTS;
        }
    }
    if (digits == 0) return ERROR_INVALID_ARGUMENTS;

    char *endptr = NULL;
    double val = strtod(str, &endptr);
    if (endptr == str || *endptr != '\0') return ERROR_INVALID_ARGUMENTS;
    if (isinf(val)) return ERROR_OVERFLOW;

    *out = val;
    return SUCCESS;
}

status parse_name(const char *str) {
    if (str == NULL || *str == '\0') return ERROR_INVALID_ARGUMENTS;

    for (size_t i = 0; str[i] != '\0'; ++i) {
        char c = str[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return ERROR_INVALID_ARGUMENTS;
        }
    }
    return SUCCESS;
}

status parse_flag(const char *arg, char *out_flag) {
    if (arg == NULL || out_flag == NULL) return ERROR_INVALID_ARGUMENTS;
    if (arg[0] != '-' && arg[0] != '/') return ERROR_UNKNOWN_FLAG;
    if (arg[1] != 'a' && arg[1] != 'd') return ERROR_UNKNOWN_FLAG;
    if (arg[2] != '\0') return ERROR_UNKNOWN_FLAG;

    *out_flag = arg[1];
    return SUCCESS;
}