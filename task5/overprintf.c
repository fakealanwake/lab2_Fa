#include "overprintf.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "convert.h"
#include "status.h"

#define FORMAT_CAP 1000000

typedef struct buf {
    char *data;
    size_t len;
    size_t cap;
} buf;

typedef struct spec {
    char text[128];
    int custom;
    char c1;
    char c2;
    int width_star;
    int prec_star;
    int length;
    char conv;
} spec;

static status buf_init(buf *b)
{
    b->data = (char *)malloc(64);
    if (b->data == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    b->data[0] = '\0';
    b->len = 0;
    b->cap = 64;
    return SUCCESS;
}

static void buf_free(buf *b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static status buf_add(buf *b, const char *text, size_t n)
{
    if (n > SIZE_MAX - 1 - b->len) {
        return ERROR_OVERFLOW;
    }
    if (b->len + n + 1 > b->cap) {
        size_t next = b->cap;
        char *grown;
        while (next < b->len + n + 1) {
            if (next > SIZE_MAX / 2) {
                return ERROR_OVERFLOW;
            }
            next *= 2;
        }
        grown = (char *)realloc(b->data, next);
        if (grown == NULL) {
            return ERROR_MEMORY_ALLOCATION;
        }
        b->data = grown;
        b->cap = next;
    }
    memcpy(b->data + b->len, text, n);
    b->len += n;
    b->data[b->len] = '\0';
    return SUCCESS;
}

static status buf_add_str(buf *b, const char *text)
{
    return buf_add(b, text, strlen(text));
}

#define EMIT(buffer, spec_text, ...)                                                         \
    do {                                                                                     \
        int _need = snprintf(NULL, 0, (spec_text), __VA_ARGS__);                             \
        char *_tmp;                                                                          \
        status _st;                                                                          \
        if (_need < 0 || _need > FORMAT_CAP) {                                               \
            return ERROR_OVERFLOW;                                                           \
        }                                                                                    \
        _tmp = (char *)malloc((size_t)_need + 1);                                            \
        if (_tmp == NULL) {                                                                  \
            return ERROR_MEMORY_ALLOCATION;                                                  \
        }                                                                                    \
        snprintf(_tmp, (size_t)_need + 1, (spec_text), __VA_ARGS__);                         \
        _st = buf_add((buffer), _tmp, (size_t)_need);                                        \
        free(_tmp);                                                                          \
        if (_st != SUCCESS) {                                                                \
            return _st;                                                                      \
        }                                                                                    \
    } while (0)

static int is_custom_pair(char a, char b)
{
    return (a == 'R' && b == 'o') || (a == 'Z' && b == 'r') ||
           (a == 'C' && (b == 'v' || b == 'V')) || (a == 't' && b == 'o') ||
           (a == 'T' && b == 'O') ||
           (a == 'm' && (b == 'i' || b == 'u' || b == 'd' || b == 'f'));
}

static int is_conversion(char ch)
{
    return ch == 'd' || ch == 'i' || ch == 'u' || ch == 'o' || ch == 'x' || ch == 'X' ||
           ch == 'f' || ch == 'F' || ch == 'e' || ch == 'E' || ch == 'g' || ch == 'G' ||
           ch == 'a' || ch == 'A' || ch == 'c' || ch == 's' || ch == 'p' || ch == 'n' ||
           ch == '%';
}

static status parse_spec(const char **fmt, spec *sp)
{
    const char *p;
    const char *start;
    size_t len;

    memset(sp, 0, sizeof(*sp));
    p = *fmt;
    if (*p != '%') {
        return ERROR_INVALID_FORMAT;
    }
    start = p;
    p++;
    if (p[0] != '\0' && p[1] != '\0' && is_custom_pair(p[0], p[1])) {
        sp->custom = 1;
        sp->c1 = p[0];
        sp->c2 = p[1];
        *fmt = p + 2;
        return SUCCESS;
    }
    while (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') {
        p++;
    }
    if (*p == '*') {
        sp->width_star = 1;
        p++;
    } else {
        while (isdigit((unsigned char)*p)) {
            p++;
        }
    }
    if (*p == '.') {
        p++;
        if (*p == '*') {
            sp->prec_star = 1;
            p++;
        } else {
            while (isdigit((unsigned char)*p)) {
                p++;
            }
        }
    }
    if (*p == 'h') {
        p++;
        if (*p == 'h') {
            sp->length = 2;
            p++;
        } else {
            sp->length = 1;
        }
    } else if (*p == 'l') {
        p++;
        if (*p == 'l') {
            sp->length = 4;
            p++;
        } else {
            sp->length = 3;
        }
    } else if (*p == 'L') {
        sp->length = 5;
        p++;
    } else if (*p == 'j') {
        sp->length = 6;
        p++;
    } else if (*p == 'z') {
        sp->length = 7;
        p++;
    } else if (*p == 't') {
        sp->length = 8;
        p++;
    }
    if (!is_conversion(*p)) {
        return ERROR_INVALID_FORMAT;
    }
    sp->conv = *p;
    p++;
    if (sp->conv == '%' && (p - start) != 2) {
        return ERROR_INVALID_FORMAT;
    }
    len = (size_t)(p - start);
    if (len >= sizeof(sp->text)) {
        return ERROR_INVALID_FORMAT;
    }
    memcpy(sp->text, start, len);
    sp->text[len] = '\0';
    *fmt = p;
    return SUCCESS;
}

static status append_custom(buf *b, const spec *sp, va_list *ap)
{
    char *text = NULL;
    status st = SUCCESS;

    if (sp->c1 == 'R') {
        st = format_roman(va_arg(*ap, int), &text);
    } else if (sp->c1 == 'Z') {
        st = format_zeckendorf(va_arg(*ap, unsigned int), &text);
    } else if (sp->c1 == 'C') {
        int value = va_arg(*ap, int);
        int base = va_arg(*ap, int);
        st = format_int_base(value, base, sp->c2 == 'V', &text);
    } else if (sp->c1 == 't' || sp->c1 == 'T') {
        const char *number = va_arg(*ap, const char *);
        int base = va_arg(*ap, int);
        st = format_to_decimal(number, base, sp->c1 == 'T', &text);
    } else if (sp->c2 == 'i') {
        int32_t value = va_arg(*ap, int);
        st = format_memory(&value, sizeof(value), &text);
    } else if (sp->c2 == 'u') {
        uint32_t value = va_arg(*ap, unsigned int);
        st = format_memory(&value, sizeof(value), &text);
    } else if (sp->c2 == 'd') {
        double value = va_arg(*ap, double);
        st = format_memory(&value, sizeof(value), &text);
    } else if (sp->c2 == 'f') {
        float value = (float)va_arg(*ap, double);
        st = format_memory(&value, sizeof(value), &text);
    } else {
        st = ERROR_INVALID_FORMAT;
    }
    if (st == SUCCESS) {
        st = buf_add_str(b, text);
    }
    free(text);
    return st;
}

static status store_count(size_t len, int length, va_list *ap)
{
    if (length == 2) {
        signed char *ptr = va_arg(*ap, signed char *);
        if (ptr == NULL) {
            return ERROR_NULL_POINTER;
        }
        if (len > 127) {
            return ERROR_OVERFLOW;
        }
        *ptr = (signed char)len;
    } else if (length == 1) {
        short *ptr = va_arg(*ap, short *);
        if (ptr == NULL) {
            return ERROR_NULL_POINTER;
        }
        if (len > SHRT_MAX) {
            return ERROR_OVERFLOW;
        }
        *ptr = (short)len;
    } else if (length == 3) {
        long *ptr = va_arg(*ap, long *);
        if (ptr == NULL) {
            return ERROR_NULL_POINTER;
        }
        if (len > LONG_MAX) {
            return ERROR_OVERFLOW;
        }
        *ptr = (long)len;
    } else if (length == 4) {
        long long *ptr = va_arg(*ap, long long *);
        if (ptr == NULL) {
            return ERROR_NULL_POINTER;
        }
        *ptr = (long long)len;
    } else if (length == 6) {
        intmax_t *ptr = va_arg(*ap, intmax_t *);
        if (ptr == NULL) {
            return ERROR_NULL_POINTER;
        }
        *ptr = (intmax_t)len;
    } else if (length == 7 || length == 8) {
        ptrdiff_t *ptr = va_arg(*ap, ptrdiff_t *);
        if (ptr == NULL) {
            return ERROR_NULL_POINTER;
        }
        *ptr = (ptrdiff_t)len;
    } else {
        int *ptr = va_arg(*ap, int *);
        if (ptr == NULL) {
            return ERROR_NULL_POINTER;
        }
        if (len > INT_MAX) {
            return ERROR_OVERFLOW;
        }
        *ptr = (int)len;
    }
    return SUCCESS;
}

static status append_standard(buf *b, const spec *sp, va_list *ap)
{
    int stars = (sp->width_star ? 1 : 0) + (sp->prec_star ? 2 : 0);
    int width = 0;
    int prec = 0;
    int signed_conv;
    int float_conv;

    if (sp->width_star) {
        width = va_arg(*ap, int);
    }
    if (sp->prec_star) {
        prec = va_arg(*ap, int);
    }
    if (sp->conv == '%') {
        return buf_add(b, "%", 1);
    }
    if (sp->conv == 'n') {
        return store_count(b->len, sp->length, ap);
    }

    signed_conv = sp->conv == 'd' || sp->conv == 'i';
    float_conv = sp->conv == 'f' || sp->conv == 'F' || sp->conv == 'e' || sp->conv == 'E' ||
                 sp->conv == 'g' || sp->conv == 'G' || sp->conv == 'a' || sp->conv == 'A';

    if (sp->conv == 's') {
        char *text = va_arg(*ap, char *);
        if (text == NULL) {
            return ERROR_NULL_POINTER;
        }
        if (stars == 0) {
            EMIT(b, sp->text, text);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, text);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, text);
        } else {
            EMIT(b, sp->text, width, prec, text);
        }
        return SUCCESS;
    }
    if (sp->conv == 'p') {
        void *ptr = va_arg(*ap, void *);
        if (stars == 0) {
            EMIT(b, sp->text, ptr);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, ptr);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, ptr);
        } else {
            EMIT(b, sp->text, width, prec, ptr);
        }
        return SUCCESS;
    }
    if (sp->conv == 'c' || (signed_conv && (sp->length == 0 || sp->length == 1 || sp->length == 2))) {
        int value = va_arg(*ap, int);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (!signed_conv && !float_conv && sp->conv != 's' && sp->conv != 'p' &&
        (sp->length == 1 || sp->length == 2)) {
        unsigned int value = (unsigned int)va_arg(*ap, int);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (signed_conv && sp->length == 3) {
        long value = va_arg(*ap, long);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (signed_conv && sp->length == 4) {
        long long value = va_arg(*ap, long long);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (!signed_conv && !float_conv && sp->length == 0) {
        unsigned int value = va_arg(*ap, unsigned int);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (!signed_conv && !float_conv && sp->length == 3) {
        unsigned long value = va_arg(*ap, unsigned long);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (!signed_conv && !float_conv && sp->length == 4) {
        unsigned long long value = va_arg(*ap, unsigned long long);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (float_conv && sp->length != 5) {
        double value = va_arg(*ap, double);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (float_conv && sp->length == 5) {
        long double value = va_arg(*ap, long double);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (sp->length == 6 && signed_conv) {
        intmax_t value = va_arg(*ap, intmax_t);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (sp->length == 6 && !signed_conv && !float_conv) {
        uintmax_t value = va_arg(*ap, uintmax_t);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if (sp->length == 7 && !signed_conv && !float_conv) {
        size_t value = va_arg(*ap, size_t);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    if ((sp->length == 7 && signed_conv) || sp->length == 8) {
        ptrdiff_t value = va_arg(*ap, ptrdiff_t);
        if (stars == 0) {
            EMIT(b, sp->text, value);
        } else if (stars == 1) {
            EMIT(b, sp->text, width, value);
        } else if (stars == 2) {
            EMIT(b, sp->text, prec, value);
        } else {
            EMIT(b, sp->text, width, prec, value);
        }
        return SUCCESS;
    }
    return ERROR_INVALID_FORMAT;
}

static status format_list(char **out, const char *format, va_list *ap)
{
    buf buffer;
    status st;

    if (out == NULL) {
        return ERROR_NULL_POINTER;
    }
    *out = NULL;
    if (format == NULL) {
        return ERROR_NULL_POINTER;
    }
    st = buf_init(&buffer);
    if (st != SUCCESS) {
        return st;
    }
    while (*format != '\0' && st == SUCCESS) {
        if (*format != '%') {
            const char *start = format;
            while (*format != '\0' && *format != '%') {
                format++;
            }
            st = buf_add(&buffer, start, (size_t)(format - start));
        } else {
            spec sp;
            st = parse_spec(&format, &sp);
            if (st == SUCCESS) {
                if (sp.custom) {
                    st = append_custom(&buffer, &sp, ap);
                } else {
                    st = append_standard(&buffer, &sp, ap);
                }
            }
        }
    }
    if (st != SUCCESS) {
        buf_free(&buffer);
        return st;
    }
    if (buffer.len > INT_MAX) {
        buf_free(&buffer);
        return ERROR_OVERFLOW;
    }
    *out = buffer.data;
    return SUCCESS;
}

static int format_entry(char **out, const char *format, va_list *ap)
{
    status st = format_list(out, format, ap);
    if (st != SUCCESS) {
        return -1;
    }
    return (int)strlen(*out);
}

int oversprintf(char *str, const char *format, ...)
{
    char *built = NULL;
    va_list ap;
    int count;

    if (str == NULL || format == NULL) {
        return -1;
    }
    va_start(ap, format);
    count = format_entry(&built, format, &ap);
    va_end(ap);
    if (count < 0) {
        free(built);
        return -1;
    }
    memcpy(str, built, (size_t)count + 1);
    free(built);
    return count;
}

int overfprintf(FILE *stream, const char *format, ...)
{
    char *built = NULL;
    va_list ap;
    int count;
    size_t written;

    if (stream == NULL || format == NULL) {
        return -1;
    }
    va_start(ap, format);
    count = format_entry(&built, format, &ap);
    va_end(ap);
    if (count < 0) {
        free(built);
        return -1;
    }
    written = fwrite(built, 1, (size_t)count, stream);
    free(built);
    if (written != (size_t)count) {
        return -1;
    }
    return count;
}
