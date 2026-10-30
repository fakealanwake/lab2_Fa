#include "input.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 4096
#define NAME_SIZE 1024
#define MAX_FILES 100000

void free_file_list(file_info *files, size_t count)
{
    size_t i;

    if (files == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        free(files[i].name);
        files[i].name = NULL;
    }
    free(files);
}

static int is_blank(const char *s)
{
    if (s == NULL) {
        return 1;
    }
    for (; *s != '\0'; s++) {
        if (!isspace((unsigned char)*s)) {
            return 0;
        }
    }
    return 1;
}

static status next_token(const char **pos, char *out, size_t out_size)
{
    const char *p;
    size_t len = 0;

    if (pos == NULL || *pos == NULL || out == NULL || out_size < 2) {
        return ERROR_INVALID_DATA;
    }
    p = *pos;
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p == '\0') {
        return ERROR_INVALID_DATA;
    }
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        if (len + 1 >= out_size) {
            return ERROR_LIMIT;
        }
        out[len++] = *p++;
    }
    out[len] = '\0';
    *pos = p;
    return SUCCESS;
}

static status parse_ull(const char *text, unsigned long long *out)
{
    unsigned long long value = 0;
    size_t i;

    if (text == NULL || out == NULL || text[0] == '\0') {
        return ERROR_INVALID_DATA;
    }
    for (i = 0; text[i] != '\0'; i++) {
        unsigned int digit;
        if (!isdigit((unsigned char)text[i])) {
            return ERROR_INVALID_DATA;
        }
        digit = (unsigned int)(text[i] - '0');
        if (value > (ULLONG_MAX - digit) / 10) {
            return ERROR_OVERFLOW;
        }
        value = value * 10 + digit;
    }
    *out = value;
    return SUCCESS;
}

static status read_line(FILE *in, char *buf, size_t buf_size, int *got_line)
{
    size_t len;

    if (fgets(buf, (int)buf_size, in) == NULL) {
        if (ferror(in)) {
            return ERROR_IO;
        }
        *got_line = 0;
        return SUCCESS;
    }
    len = strlen(buf);
    if (len == 0 || (buf[len - 1] != '\n' && !feof(in))) {
        return ERROR_LIMIT;
    }
    *got_line = 1;
    return SUCCESS;
}

static status names_unique(const file_info *files, size_t count)
{
    size_t i;
    size_t j;

    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (strcmp(files[i].name, files[j].name) == 0) {
                return ERROR_INVALID_DATA;
            }
        }
    }
    return SUCCESS;
}

status read_backup_problem(FILE *in, file_info **files, size_t *count, unsigned long long *capacity)
{
    char line[LINE_SIZE];
    char token[NAME_SIZE];
    const char *p;
    file_info *arr = NULL;
    unsigned long long n_value = 0;
    unsigned long long cap = 0;
    size_t n = 0;
    size_t i;
    int got = 0;
    status st;

    if (in == NULL || files == NULL || count == NULL || capacity == NULL) {
        return ERROR_NULL_POINTER;
    }
    *files = NULL;
    *count = 0;
    *capacity = 0;

    st = read_line(in, line, sizeof(line), &got);
    if (st != SUCCESS) {
        return st;
    }
    if (!got || is_blank(line)) {
        return ERROR_INVALID_DATA;
    }
    p = line;
    st = next_token(&p, token, sizeof(token));
    if (st != SUCCESS) {
        return st;
    }
    st = parse_ull(token, &n_value);
    if (st != SUCCESS) {
        return st;
    }
    st = next_token(&p, token, sizeof(token));
    if (st != SUCCESS) {
        return st;
    }
    st = parse_ull(token, &cap);
    if (st != SUCCESS) {
        return st;
    }
    if (!is_blank(p) || n_value == 0 || n_value > MAX_FILES) {
        return ERROR_INVALID_DATA;
    }
    n = (size_t)n_value;

    arr = (file_info *)calloc(n, sizeof(*arr));
    if (arr == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    for (i = 0; i < n; i++) {
        unsigned long long size = 0;
        unsigned long long importance = 0;
        char *name;

        st = read_line(in, line, sizeof(line), &got);
        if (st != SUCCESS) {
            free_file_list(arr, i);
            return st;
        }
        if (!got || is_blank(line)) {
            free_file_list(arr, i);
            return ERROR_INVALID_DATA;
        }
        p = line;
        st = next_token(&p, token, sizeof(token));
        if (st != SUCCESS) {
            free_file_list(arr, i);
            return st;
        }
        name = (char *)malloc(strlen(token) + 1);
        if (name == NULL) {
            free_file_list(arr, i);
            return ERROR_MEMORY_ALLOCATION;
        }
        memcpy(name, token, strlen(token) + 1);
        arr[i].name = name;

        st = next_token(&p, token, sizeof(token));
        if (st != SUCCESS || (st = parse_ull(token, &size)) != SUCCESS || size == 0) {
            free_file_list(arr, i + 1);
            return st == SUCCESS ? ERROR_INVALID_DATA : st;
        }
        st = next_token(&p, token, sizeof(token));
        if (st != SUCCESS || (st = parse_ull(token, &importance)) != SUCCESS) {
            free_file_list(arr, i + 1);
            return st;
        }
        if (!is_blank(p)) {
            free_file_list(arr, i + 1);
            return ERROR_INVALID_DATA;
        }
        arr[i].size = size;
        arr[i].importance = importance;
    }

    st = read_line(in, line, sizeof(line), &got);
    while (st == SUCCESS && got && is_blank(line)) {
        st = read_line(in, line, sizeof(line), &got);
    }
    if (st != SUCCESS) {
        free_file_list(arr, n);
        return st;
    }
    if (got) {
        free_file_list(arr, n);
        return ERROR_INVALID_DATA;
    }

    st = names_unique(arr, n);
    if (st != SUCCESS) {
        free_file_list(arr, n);
        return st;
    }

    *files = arr;
    *count = n;
    *capacity = cap;
    return SUCCESS;
}
