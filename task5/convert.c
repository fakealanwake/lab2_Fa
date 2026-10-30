#include "convert.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static status duplicate_text(const char *text, char **out)
{
    size_t len;
    char *copy;

    len = strlen(text);
    copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    memcpy(copy, text, len + 1);
    *out = copy;
    return SUCCESS;
}

status format_roman(int value, char **out)
{
    static const int values[] = {1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1};
    static const char *symbols[] = {
        "M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"
    };
    unsigned int number;
    int negative = 0;
    size_t cap;
    size_t pos = 0;
    char *buf;
    size_t i;

    if (out == NULL) {
        return ERROR_NULL_POINTER;
    }
    *out = NULL;
    if (value == 0) {
        return duplicate_text("0", out);
    }
    if (value < 0) {
        negative = 1;
        number = (value == INT_MIN) ? (unsigned int)INT_MAX + 1u : (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }

    cap = (size_t)(number / 1000u) + 16u;
    if (negative) {
        cap++;
    }
    buf = (char *)malloc(cap + 1);
    if (buf == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    if (negative) {
        buf[pos++] = '-';
    }
    for (i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        while (number >= (unsigned int)values[i]) {
            size_t slen = strlen(symbols[i]);
            if (pos + slen > cap) {
                free(buf);
                return ERROR_OVERFLOW;
            }
            memcpy(buf + pos, symbols[i], slen);
            pos += slen;
            number -= (unsigned int)values[i];
        }
    }
    buf[pos] = '\0';
    *out = buf;
    return SUCCESS;
}

status format_zeckendorf(unsigned int value, char **out)
{
    unsigned int fibs[64];
    char bits[64];
    int count = 0;
    int highest = -1;
    int i;
    unsigned int left;
    char *buf;
    size_t pos = 0;

    if (out == NULL) {
        return ERROR_NULL_POINTER;
    }
    *out = NULL;
    if (value == 0) {
        return duplicate_text("1", out);
    }

    fibs[count++] = 1;
    fibs[count++] = 2;
    while (count < 64) {
        unsigned int next = fibs[count - 1] + fibs[count - 2];
        if (next < fibs[count - 1] || next > value) {
            break;
        }
        fibs[count++] = next;
        if (next == value) {
            break;
        }
    }

    left = value;
    for (i = count - 1; i >= 0; i--) {
        if (fibs[i] <= left) {
            bits[i] = '1';
            left -= fibs[i];
            if (highest < 0) {
                highest = i;
            }
        } else {
            bits[i] = '0';
        }
    }
    if (highest < 0 || left != 0) {
        return ERROR_INVALID_ARGUMENT;
    }

    buf = (char *)malloc((size_t)highest + 3);
    if (buf == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    for (i = 0; i <= highest; i++) {
        buf[pos++] = bits[i];
    }
    buf[pos++] = '1';
    buf[pos] = '\0';
    *out = buf;
    return SUCCESS;
}

static int digit_char(int digit, int uppercase)
{
    if (digit < 10) {
        return '0' + digit;
    }
    return (uppercase ? 'A' : 'a') + (digit - 10);
}

status format_int_base(int value, int base, int uppercase, char **out)
{
    unsigned int number;
    int negative = 0;
    char rev[sizeof(int) * 8 + 2];
    size_t n = 0;
    size_t i;
    char *buf;
    size_t pos = 0;

    if (out == NULL) {
        return ERROR_NULL_POINTER;
    }
    *out = NULL;
    if (base < 2 || base > 36) {
        base = 10;
    }
    if (value < 0) {
        negative = 1;
        number = (value == INT_MIN) ? (unsigned int)INT_MAX + 1u : (unsigned int)(-value);
    } else {
        number = (unsigned int)value;
    }

    if (number == 0) {
        rev[n++] = '0';
    } else {
        while (number > 0) {
            rev[n++] = (char)digit_char((int)(number % (unsigned int)base), uppercase);
            number /= (unsigned int)base;
        }
    }

    buf = (char *)malloc(n + (negative ? 1 : 0) + 1);
    if (buf == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    if (negative) {
        buf[pos++] = '-';
    }
    for (i = 0; i < n; i++) {
        buf[pos++] = rev[n - 1 - i];
    }
    buf[pos] = '\0';
    *out = buf;
    return SUCCESS;
}

static int char_digit(char ch, int uppercase, int *digit)
{
    if (ch >= '0' && ch <= '9') {
        *digit = ch - '0';
        return 1;
    }
    if (!uppercase && ch >= 'a' && ch <= 'z') {
        *digit = ch - 'a' + 10;
        return 1;
    }
    if (uppercase && ch >= 'A' && ch <= 'Z') {
        *digit = ch - 'A' + 10;
        return 1;
    }
    return 0;
}

static status dec_push(unsigned char **digits, size_t *len, size_t *cap, unsigned char digit)
{
    if (*len == *cap) {
        size_t next = (*cap == 0) ? 16 : *cap * 2;
        unsigned char *grown;
        if (next < *cap) {
            return ERROR_OVERFLOW;
        }
        grown = (unsigned char *)realloc(*digits, next);
        if (grown == NULL) {
            return ERROR_MEMORY_ALLOCATION;
        }
        *digits = grown;
        *cap = next;
    }
    (*digits)[(*len)++] = digit;
    return SUCCESS;
}

static status dec_mul_add(unsigned char **digits, size_t *len, size_t *cap, int base, int add)
{
    size_t i;
    int carry = add;

    for (i = 0; i < *len; i++) {
        int value = (int)(*digits)[i] * base + carry;
        (*digits)[i] = (unsigned char)(value % 10);
        carry = value / 10;
    }
    while (carry > 0) {
        status st = dec_push(digits, len, cap, (unsigned char)(carry % 10));
        if (st != SUCCESS) {
            return st;
        }
        carry /= 10;
    }
    if (*len == 0) {
        return dec_push(digits, len, cap, 0);
    }
    return SUCCESS;
}

status format_to_decimal(const char *number, int base, int uppercase, char **out)
{
    unsigned char *digits = NULL;
    size_t len = 0;
    size_t cap = 0;
    int negative = 0;
    size_t i;
    size_t high;
    char *buf;
    size_t pos = 0;
    status st = SUCCESS;

    if (number == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    *out = NULL;
    if (base < 2 || base > 36) {
        base = 10;
    }
    if (number[0] == '+' || number[0] == '-') {
        negative = number[0] == '-';
        number++;
    }
    if (number[0] == '\0') {
        return ERROR_INVALID_ARGUMENT;
    }

    for (i = 0; number[i] != '\0'; i++) {
        int digit = 0;
        if (!char_digit(number[i], uppercase, &digit) || digit >= base) {
            free(digits);
            return ERROR_INVALID_ARGUMENT;
        }
        st = dec_mul_add(&digits, &len, &cap, base, digit);
        if (st != SUCCESS) {
            free(digits);
            return st;
        }
    }

    high = len;
    while (high > 1 && digits[high - 1] == 0) {
        high--;
    }
    if (high == 1 && digits[0] == 0) {
        negative = 0;
    }
    buf = (char *)malloc(high + (negative ? 1 : 0) + 1);
    if (buf == NULL) {
        free(digits);
        return ERROR_MEMORY_ALLOCATION;
    }
    if (negative) {
        buf[pos++] = '-';
    }
    for (i = 0; i < high; i++) {
        buf[pos++] = (char)('0' + digits[high - 1 - i]);
    }
    buf[pos] = '\0';
    free(digits);
    *out = buf;
    return SUCCESS;
}

status format_memory(const void *data, size_t nbytes, char **out)
{
    const unsigned char *bytes;
    size_t len;
    char *buf;
    size_t pos = 0;
    size_t i;

    if (data == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    *out = NULL;
    if (nbytes == 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    if (nbytes > (SIZE_MAX - 1) / 9) {
        return ERROR_OVERFLOW;
    }
    len = nbytes * 8 + (nbytes - 1);
    buf = (char *)malloc(len + 1);
    if (buf == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    bytes = (const unsigned char *)data;
    for (i = 0; i < nbytes; i++) {
        int bit;
        if (i > 0) {
            buf[pos++] = ' ';
        }
        for (bit = 7; bit >= 0; bit--) {
            buf[pos++] = (bytes[i] & (1u << bit)) ? '1' : '0';
        }
    }
    buf[pos] = '\0';
    *out = buf;
    return SUCCESS;
}
