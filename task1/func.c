#include "func.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

status str_length(const char *s, size_t *out)
{
    size_t n = 0;

    if (s == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    while (s[n] != '\0') {
        n++;
    }
    *out = n;
    return SUCCESS;
}

static status alloc_string(size_t len, char **out)
{
    char *buffer;

    if (out == NULL) {
        return ERROR_NULL_POINTER;
    }
    if (len == SIZE_MAX) {
        return ERROR_OVERFLOW;
    }
    buffer = (char *)malloc(len + 1);
    if (buffer == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    buffer[len] = '\0';
    *out = buffer;
    return SUCCESS;
}

status str_reverse(const char *s, char **out)
{
    size_t len = 0;
    size_t i;
    char *result = NULL;
    status code;

    if (s == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    code = str_length(s, &len);
    if (code != SUCCESS) {
        return code;
    }
    code = alloc_string(len, &result);
    if (code != SUCCESS) {
        return code;
    }
    for (i = 0; i < len; i++) {
        result[i] = s[len - 1 - i];
    }
    *out = result;
    return SUCCESS;
}

status str_upper_odd(const char *s, char **out)
{
    size_t len = 0;
    size_t i;
    char *result = NULL;
    status code;

    if (s == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    code = str_length(s, &len);
    if (code != SUCCESS) {
        return code;
    }
    code = alloc_string(len, &result);
    if (code != SUCCESS) {
        return code;
    }
    for (i = 0; i < len; i++) {
        if (i % 2 == 1) {
            result[i] = (char)toupper((unsigned char)s[i]);
        } else {
            result[i] = s[i];
        }
    }
    *out = result;
    return SUCCESS;
}

status str_group_chars(const char *s, char **out)
{
    size_t len = 0;
    size_t i;
    size_t pos = 0;
    char *result = NULL;
    status code;

    if (s == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    code = str_length(s, &len);
    if (code != SUCCESS) {
        return code;
    }
    code = alloc_string(len, &result);
    if (code != SUCCESS) {
        return code;
    }
    for (i = 0; i < len; i++) {
        if (isdigit((unsigned char)s[i])) {
            result[pos++] = s[i];
        }
    }
    for (i = 0; i < len; i++) {
        if (isalpha((unsigned char)s[i])) {
            result[pos++] = s[i];
        }
    }
    for (i = 0; i < len; i++) {
        if (!isdigit((unsigned char)s[i]) && !isalpha((unsigned char)s[i])) {
            result[pos++] = s[i];
        }
    }
    *out = result;
    return SUCCESS;
}

status str_concat_random(const char *first, const char *const *rest,
                                size_t rest_count, unsigned int seed,
                                char **out)
{
    const char **items;
    size_t count;
    size_t total = 0;
    size_t len = 0;
    size_t i;
    size_t j;
    size_t pos = 0;
    size_t k;
    char *result = NULL;
    status code;

    if (first == NULL || out == NULL || (rest == NULL && rest_count > 0)) {
        return ERROR_NULL_POINTER;
    }
    if (rest_count == SIZE_MAX) {
        return ERROR_OVERFLOW;
    }
    count = rest_count + 1;
    if (count > SIZE_MAX / sizeof(*items)) {
        return ERROR_OVERFLOW;
    }
    items = (const char **)malloc(count * sizeof(*items));
    if (items == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    items[0] = first;
    for (i = 0; i < rest_count; i++) {
        if (rest[i] == NULL) {
            free((void *)items);
            return ERROR_NULL_POINTER;
        }
        items[i + 1] = rest[i];
    }
    for (i = 0; i < count; i++) {
        code = str_length(items[i], &len);
        if (code != SUCCESS) {
            free((void *)items);
            return code;
        }
        if (len > SIZE_MAX - 1 - total) {
            free((void *)items);
            return ERROR_OVERFLOW;
        }
        total += len;
    }
    code = alloc_string(total, &result);
    if (code != SUCCESS) {
        free((void *)items);
        return code;
    }
    srand(seed);
    for (i = count - 1; i > 0; i--) {
        const char *tmp;
        j = (size_t)rand() % (i + 1);
        tmp = items[i];
        items[i] = items[j];
        items[j] = tmp;
    }
    for (i = 0; i < count; i++) {
        for (k = 0; items[i][k] != '\0'; k++) {
            result[pos++] = items[i][k];
        }
    }
    free((void *)items);
    *out = result;
    return SUCCESS;
}