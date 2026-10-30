#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "big_int.h"

static int char_to_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

static char digit_to_char(int d) {
    if (d >= 0 && d <= 9) return (char)('0' + d);
    if (d >= 10 && d <= 35) return (char)('A' + (d - 10));
    return '0';
}

status validate_string(const char *str, int base) {
    if (str == NULL || *str == '\0') {
        return ERROR_INVALID_ARGUMENTS;
    }
    if (base < 2 || base > 36) {
        return ERROR_INVALID_BASE;
    }

    for (size_t i = 0; str[i] != '\0'; i++) {
        int digit = char_to_digit(str[i]);
        if (digit < 0 || digit >= base) {
            return ERROR_INVALID_DIGIT;
        }
    }
    return SUCCESS;
}

status add_large_numbers(const char *num1, const char *num2, int base, char **result) {
    if (num1 == NULL || num2 == NULL || result == NULL) {
        return ERROR_INVALID_ARGUMENTS;
    }
    *result = NULL;

    if (base < 2 || base > 36) {
        return ERROR_INVALID_BASE;
    }

    status st = validate_string(num1, base);
    if (st != SUCCESS) {
        return st;
    }
    st = validate_string(num2, base);
    if (st != SUCCESS) {
        return st;
    }

    size_t len1 = strlen(num1);
    size_t len2 = strlen(num2);
    size_t max_len = len1 > len2 ? len1 : len2;

    if (max_len > SIZE_MAX - 2) {
        return ERROR_OVERFLOW;
    }

    char *buffer = malloc(max_len + 2);
    if (buffer == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    size_t i = len1;
    size_t j = len2;
    size_t k = 0;
    int carry = 0;

    while (i > 0 || j > 0 || carry > 0) {
        int d1 = 0;
        int d2 = 0;
        if (i > 0) {
            i--;
            d1 = char_to_digit(num1[i]);
        }
        if (j > 0) {
            j--;
            d2 = char_to_digit(num2[j]);
        }

        int sum = d1 + d2 + carry;
        carry = sum / base;
        buffer[k++] = digit_to_char(sum % base);
    }
    buffer[k] = '\0';

    for (size_t l = 0; l < k / 2; l++) {
        char temp = buffer[l];
        buffer[l] = buffer[k - 1 - l];
        buffer[k - 1 - l] = temp;
    }

    size_t start = 0;
    while (buffer[start] == '0' && buffer[start + 1] != '\0') {
        start++;
    }
    if (start > 0) {
        memmove(buffer, buffer + start, k - start + 1);
    }

    *result = buffer;
    return SUCCESS;
}

status sum_multiple_numbers(int base, int count, char **result, ...) {
    if (result == NULL || count <= 0) {
        return ERROR_INVALID_ARGUMENTS;
    }
    *result = NULL;

    if (base < 2 || base > 36) {
        return ERROR_INVALID_BASE;
    }

    char *current_sum = malloc(2);
    if (current_sum == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    current_sum[0] = '0';
    current_sum[1] = '\0';

    va_list args;
    va_start(args, result);

    for (int i = 0; i < count; i++) {
        const char *next_num = va_arg(args, const char *);

        char *temp_result = NULL;
        status add_status = add_large_numbers(current_sum, next_num, base, &temp_result);

        free(current_sum);
        if (add_status != SUCCESS) {
            va_end(args);
            return add_status;
        }
        current_sum = temp_result;
    }

    va_end(args);
    *result = current_sum;
    return SUCCESS;
}