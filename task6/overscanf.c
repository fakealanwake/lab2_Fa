#include "overscanf.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "status.h"

enum {
    SC_FORMAT = -2,
    SC_EOF = -1,
    SC_MATCH = 0,
    SC_OK = 1,
    SC_ASSIGNED = 2
};

typedef struct scanner {
    FILE *file;
    const char *str;
    size_t index;
    size_t consumed;
    int stack[64];
    int top;
} scanner;

static int sc_get(scanner *sc)
{
    int ch;

    if (sc->top > 0) {
        ch = sc->stack[--sc->top];
    } else if (sc->file != NULL) {
        ch = fgetc(sc->file);
    } else if (sc->str[sc->index] == '\0') {
        return EOF;
    } else {
        ch = (unsigned char)sc->str[sc->index++];
    }
    if (ch != EOF) {
        sc->consumed++;
    }
    return ch;
}

static void sc_unget(scanner *sc, int ch)
{
    if (ch == EOF || sc->top >= (int)(sizeof(sc->stack) / sizeof(sc->stack[0]))) {
        return;
    }
    sc->stack[sc->top++] = ch;
    if (sc->consumed > 0) {
        sc->consumed--;
    }
}

static void sc_skip_ws(scanner *sc)
{
    int ch = sc_get(sc);
    while (ch != EOF && isspace((unsigned char)ch)) {
        ch = sc_get(sc);
    }
    if (ch != EOF) {
        sc_unget(sc, ch);
    }
}

static int rom_value(int ch)
{
    switch (toupper((unsigned char)ch)) {
        case 'I': return 1;
        case 'V': return 5;
        case 'X': return 10;
        case 'L': return 50;
        case 'C': return 100;
        case 'D': return 500;
        case 'M': return 1000;
        default: return -1;
    }
}

static status zeckendorf_value(const char *bits, size_t count, unsigned int *out)
{
    unsigned int prev2 = 1;
    unsigned int prev1 = 2;
    unsigned int value = 0;
    size_t i;

    for (i = 0; i < count; i++) {
        unsigned int fib;
        if (i == 0) {
            fib = 1;
        } else if (i == 1) {
            fib = 2;
        } else {
            if (prev1 > UINT_MAX - prev2) {
                return ERROR_OVERFLOW;
            }
            fib = prev2 + prev1;
            prev2 = prev1;
            prev1 = fib;
        }
        if (bits[i] == '1') {
            if (value > UINT_MAX - fib) {
                return ERROR_OVERFLOW;
            }
            value += fib;
        }
    }
    *out = value;
    return SUCCESS;
}

static int is_signed_assign_zero(int length, va_list *args);

/* mode: 0 — любые буквы, 1 — только нижний регистр, 2 — только верхний. */
static int digit_of(int ch, int mode, int *digit)
{
    if (ch >= '0' && ch <= '9') {
        *digit = ch - '0';
        return 1;
    }
    if ((mode == 0 || mode == 1) && ch >= 'a' && ch <= 'z') {
        *digit = ch - 'a' + 10;
        return 1;
    }
    if ((mode == 0 || mode == 2) && ch >= 'A' && ch <= 'Z') {
        *digit = ch - 'A' + 10;
        return 1;
    }
    return 0;
}

static int read_digits(scanner *sc, int base, int mode, int width, int *used, unsigned long long *acc)
{
    int any = 0;
    int ch = sc_get(sc);

    while (ch != EOF && (width < 0 || *used < width)) {
        int digit = 0;
        if (!digit_of(ch, mode, &digit) || digit >= base) {
            sc_unget(sc, ch);
            break;
        }
        if (*acc > (ULLONG_MAX - (unsigned long long)digit) / (unsigned long long)base) {
            return SC_MATCH;
        }
        *acc = *acc * (unsigned long long)base + (unsigned long long)digit;
        any = 1;
        (*used)++;
        ch = sc_get(sc);
    }
    if (ch != EOF && (width >= 0 && *used >= width)) {
        sc_unget(sc, ch);
    }
    return any ? SC_OK : SC_MATCH;
}

static int assign_signed(int length, int negative, unsigned long long acc, va_list *args)
{
    long long value;
    unsigned long long limit;
    unsigned long long min_acc;

    if (length == 2) {
        limit = SCHAR_MAX;
        min_acc = (unsigned long long)SCHAR_MAX + 1;
    } else if (length == 1) {
        limit = SHRT_MAX;
        min_acc = (unsigned long long)SHRT_MAX + 1;
    } else if (length == 3) {
        limit = LONG_MAX;
        min_acc = (unsigned long long)LONG_MAX + 1ull;
    } else if (length == 4) {
        limit = LLONG_MAX;
        min_acc = (unsigned long long)LLONG_MAX + 1ull;
    } else {
        limit = INT_MAX;
        min_acc = (unsigned long long)INT_MAX + 1ull;
    }
    if ((!negative && acc > limit) || (negative && acc > min_acc)) {
        return SC_MATCH;
    }
    if (negative && acc == min_acc) {
        if (length == 2) {
            value = SCHAR_MIN;
        } else if (length == 1) {
            value = SHRT_MIN;
        } else if (length == 3) {
            value = LONG_MIN;
        } else if (length == 4) {
            value = LLONG_MIN;
        } else {
            value = INT_MIN;
        }
    } else {
        value = negative ? -(long long)acc : (long long)acc;
    }
    if (length == 2) {
        signed char *ptr = va_arg(*args, signed char *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (signed char)value;
    } else if (length == 1) {
        short *ptr = va_arg(*args, short *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (short)value;
    } else if (length == 3) {
        long *ptr = va_arg(*args, long *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (long)value;
    } else if (length == 4) {
        long long *ptr = va_arg(*args, long long *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = value;
    } else {
        int *ptr = va_arg(*args, int *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (int)value;
    }
    return SC_ASSIGNED;
}

static int assign_unsigned(int length, int negative, unsigned long long acc, va_list *args)
{
    unsigned long long limit;
    unsigned long long value;

    if (length == 2) {
        limit = UCHAR_MAX;
    } else if (length == 1) {
        limit = USHRT_MAX;
    } else if (length == 3) {
        limit = ULONG_MAX;
    } else if (length == 4) {
        limit = ULLONG_MAX;
    } else {
        limit = UINT_MAX;
    }
    if (acc > limit) {
        return SC_MATCH;
    }
    value = negative ? (0ull - acc) : acc;
    if (length == 2) {
        unsigned char *ptr = va_arg(*args, unsigned char *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (unsigned char)value;
    } else if (length == 1) {
        unsigned short *ptr = va_arg(*args, unsigned short *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (unsigned short)value;
    } else if (length == 3) {
        unsigned long *ptr = va_arg(*args, unsigned long *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (unsigned long)value;
    } else if (length == 4) {
        unsigned long long *ptr = va_arg(*args, unsigned long long *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = value;
    } else {
        unsigned int *ptr = va_arg(*args, unsigned int *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (unsigned int)value;
    }
    return SC_ASSIGNED;
}

static int write_preset_int(int *preset, int negative, unsigned long long acc)
{
    long long value;

    if (preset == NULL) {
        return SC_FORMAT;
    }
    if ((!negative && acc > INT_MAX) || (negative && acc > (unsigned long long)INT_MAX + 1ull)) {
        return SC_MATCH;
    }
    if (negative && acc == (unsigned long long)INT_MAX + 1ull) {
        value = INT_MIN;
    } else {
        value = negative ? -(long long)acc : (long long)acc;
    }
    *preset = (int)value;
    return SC_ASSIGNED;
}

static int scan_integer(scanner *sc, int base, int mode, int width, int length,
                        int is_signed, int suppressed, int *preset, va_list *args)
{
    int ch;
    int used = 0;
    int negative = 0;
    unsigned long long acc = 0;
    int code;

    sc_skip_ws(sc);
    ch = sc_get(sc);
    if (ch == EOF) {
        return SC_EOF;
    }
    if (ch == '+' || ch == '-') {
        negative = ch == '-';
        used++;
        if (width >= 0 && used >= width) {
            return SC_MATCH;
        }
    } else {
        sc_unget(sc, ch);
    }
    code = read_digits(sc, base, mode, width, &used, &acc);
    if (code != SC_OK) {
        return code == SC_MATCH && used == 0 ? SC_MATCH : code;
    }
    if (suppressed) {
        return SC_OK;
    }
    if (preset != NULL) {
        return write_preset_int(preset, negative, acc);
    }
    if (is_signed) {
        return assign_signed(length, negative, acc, args);
    }
    return assign_unsigned(length, negative, acc, args);
}

static int scan_i(scanner *sc, int width, int length, int suppressed, va_list *args)
{
    int ch;
    int used = 0;
    int negative = 0;
    int base = 10;
    int mode = 1;
    unsigned long long acc = 0;
    int code;

    sc_skip_ws(sc);
    ch = sc_get(sc);
    if (ch == EOF) {
        return SC_EOF;
    }
    if (ch == '+' || ch == '-') {
        negative = ch == '-';
        used++;
        ch = sc_get(sc);
    }
    if (ch == '0') {
        int next;
        used++;
        next = sc_get(sc);
        if (next == 'x' || next == 'X') {
            if (width >= 0 && used >= width) {
                sc_unget(sc, next);
                acc = 0;
                if (suppressed) {
                    return SC_OK;
                }
                return is_signed_assign_zero(length, args);
            }
            base = 16;
            mode = 0;
            used++;
            code = read_digits(sc, base, mode, width, &used, &acc);
            if (code != SC_OK) {
                acc = 0;
            }
        } else {
            base = 8;
            mode = 1;
            if (next != EOF) {
                sc_unget(sc, next);
            }
            code = read_digits(sc, base, mode, width, &used, &acc);
            if (code != SC_OK) {
                acc = 0;
            }
        }
    } else {
        if (ch == EOF) {
            return SC_MATCH;
        }
        sc_unget(sc, ch);
        mode = 1;
        code = read_digits(sc, 10, mode, width, &used, &acc);
        if (code != SC_OK) {
            return used == 0 || (negative && used == 1) ? SC_MATCH : code;
        }
    }
    if (suppressed) {
        return SC_OK;
    }
    return assign_signed(length, negative, acc, args);
}

static int is_signed_assign_zero(int length, va_list *args)
{
    return assign_signed(length, 0, 0, args);
}

static int scan_roman(scanner *sc, int suppressed, va_list *args)
{
    int values[64];
    int count = 0;
    int ch;
    long total = 0;
    int i;

    sc_skip_ws(sc);
    ch = sc_get(sc);
    if (ch == EOF) {
        return SC_EOF;
    }
    if (rom_value(ch) < 0) {
        sc_unget(sc, ch);
        return SC_MATCH;
    }
    values[count++] = rom_value(ch);
    while (count < 64) {
        ch = sc_get(sc);
        if (rom_value(ch) < 0) {
            if (ch != EOF) {
                sc_unget(sc, ch);
            }
            break;
        }
        values[count++] = rom_value(ch);
    }
    for (i = 0; i < count; i++) {
        if (i + 1 < count && values[i] < values[i + 1]) {
            total -= values[i];
        } else {
            total += values[i];
        }
    }
    if (total < 0 || total > INT_MAX) {
        return SC_MATCH;
    }
    if (suppressed) {
        return SC_OK;
    }
    return assign_signed(0, 0, (unsigned long long)total, args);
}

static int scan_zeckendorf(scanner *sc, int suppressed, va_list *args)
{
    char bits[128];
    int count = 0;
    int ch;
    unsigned int value = 0;
    status st;

    sc_skip_ws(sc);
    ch = sc_get(sc);
    if (ch == EOF) {
        return SC_EOF;
    }
    if (ch != '0' && ch != '1') {
        sc_unget(sc, ch);
        return SC_MATCH;
    }
    bits[count++] = (char)ch;
    while (count < (int)sizeof(bits) - 1) {
        ch = sc_get(sc);
        if (ch != '0' && ch != '1') {
            if (ch != EOF) {
                sc_unget(sc, ch);
            }
            break;
        }
        bits[count++] = (char)ch;
    }
    if (bits[count - 1] != '1') {
        return SC_MATCH;
    }
    st = zeckendorf_value(bits, (size_t)count - 1, &value);
    if (st != SUCCESS) {
        return SC_MATCH;
    }
    if (suppressed) {
        return SC_OK;
    }
    return assign_unsigned(0, 0, value, args);
}

static int scan_float(scanner *sc, int length, int width, int suppressed, va_list *args)
{
    char tmp[256];
    int n = 0;
    int used = 0;
    int saw_digit = 0;
    int saw_dot = 0;
    int saw_exp = 0;
    int ch;

    sc_skip_ws(sc);
    ch = sc_get(sc);
    if (ch == EOF) {
        return SC_EOF;
    }
    if (ch == '+' || ch == '-') {
        tmp[n++] = (char)ch;
        used++;
        ch = sc_get(sc);
    }
    while (ch != EOF && n < (int)sizeof(tmp) - 1 && (width < 0 || used < width)) {
        if (isdigit((unsigned char)ch)) {
            saw_digit = 1;
            tmp[n++] = (char)ch;
        } else if (ch == '.' && !saw_dot && !saw_exp) {
            saw_dot = 1;
            tmp[n++] = '.';
        } else if ((ch == 'e' || ch == 'E') && saw_digit && !saw_exp) {
            int mark = ch;
            int next;
            saw_exp = 1;
            tmp[n++] = (char)mark;
            used++;
            next = sc_get(sc);
            if ((next == '+' || next == '-') && (width < 0 || used < width) && n < (int)sizeof(tmp) - 1) {
                tmp[n++] = (char)next;
                used++;
                ch = sc_get(sc);
                continue;
            }
            ch = next;
            continue;
        } else {
            sc_unget(sc, ch);
            break;
        }
        used++;
        ch = sc_get(sc);
    }
    if (ch != EOF && width >= 0 && used >= width) {
        sc_unget(sc, ch);
    }
    if (!saw_digit) {
        return SC_MATCH;
    }
    tmp[n] = '\0';
    if (suppressed) {
        return SC_OK;
    }
    if (length == 5) {
        long double *ptr = va_arg(*args, long double *);
        char *end = NULL;
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = strtold(tmp, &end);
        if (end == tmp) {
            return SC_MATCH;
        }
    } else if (length == 3) {
        double *ptr = va_arg(*args, double *);
        char *end = NULL;
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = strtod(tmp, &end);
        if (end == tmp) {
            return SC_MATCH;
        }
    } else {
        float *ptr = va_arg(*args, float *);
        char *end = NULL;
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = strtof(tmp, &end);
        if (end == tmp) {
            return SC_MATCH;
        }
    }
    return SC_ASSIGNED;
}

static int scan_string(scanner *sc, int width, int suppressed, va_list *args)
{
    char *dest = NULL;
    int count = 0;
    int ch;

    if (!suppressed) {
        dest = va_arg(*args, char *);
        if (dest == NULL) {
            return SC_FORMAT;
        }
    }
    sc_skip_ws(sc);
    ch = sc_get(sc);
    if (ch == EOF) {
        return SC_EOF;
    }
    while (ch != EOF && !isspace((unsigned char)ch) && (width < 0 || count < width)) {
        if (dest != NULL) {
            dest[count] = (char)ch;
        }
        count++;
        ch = sc_get(sc);
    }
    if (ch != EOF) {
        sc_unget(sc, ch);
    }
    if (count == 0) {
        return SC_MATCH;
    }
    if (dest != NULL) {
        dest[count] = '\0';
    }
    return suppressed ? SC_OK : SC_ASSIGNED;
}

static int scan_char(scanner *sc, int width, int suppressed, va_list *args)
{
    char *dest = NULL;
    int need = width < 0 ? 1 : width;
    int i;
    int ch;

    if (need <= 0) {
        return SC_MATCH;
    }
    if (!suppressed) {
        dest = va_arg(*args, char *);
        if (dest == NULL) {
            return SC_FORMAT;
        }
    }
    for (i = 0; i < need; i++) {
        ch = sc_get(sc);
        if (ch == EOF) {
            return i == 0 ? SC_EOF : SC_MATCH;
        }
        if (dest != NULL) {
            dest[i] = (char)ch;
        }
    }
    return suppressed ? SC_OK : SC_ASSIGNED;
}

static int scan_set(scanner *sc, const char **fmt, int width, int suppressed, va_list *args)
{
    int table[256];
    int negated = 0;
    const char *p = *fmt;
    char *dest = NULL;
    int count = 0;
    int ch;

    memset(table, 0, sizeof(table));
    if (*p == '^') {
        negated = 1;
        p++;
    }
    if (*p == ']') {
        table[(unsigned char)']'] = 1;
        p++;
    }
    while (*p != '\0' && *p != ']') {
        if (p[1] == '-' && p[2] != '\0' && p[2] != ']') {
            unsigned char from = (unsigned char)p[0];
            unsigned char to = (unsigned char)p[2];
            unsigned char cur;
            if (from > to) {
                unsigned char swap = from;
                from = to;
                to = swap;
            }
            for (cur = from;; cur++) {
                table[cur] = 1;
                if (cur == to) {
                    break;
                }
            }
            p += 3;
        } else {
            table[(unsigned char)*p] = 1;
            p++;
        }
    }
    if (*p != ']') {
        return SC_FORMAT;
    }
    *fmt = p + 1;
    if (!suppressed) {
        dest = va_arg(*args, char *);
        if (dest == NULL) {
            return SC_FORMAT;
        }
    }
    ch = sc_get(sc);
    while (ch != EOF && (width < 0 || count < width)) {
        int inside = table[(unsigned char)ch];
        if (negated) {
            inside = !inside;
        }
        if (!inside) {
            break;
        }
        if (dest != NULL) {
            dest[count] = (char)ch;
        }
        count++;
        ch = sc_get(sc);
    }
    if (ch != EOF) {
        sc_unget(sc, ch);
    }
    if (count == 0) {
        return ch == EOF ? SC_EOF : SC_MATCH;
    }
    if (dest != NULL) {
        dest[count] = '\0';
    }
    return suppressed ? SC_OK : SC_ASSIGNED;
}

static int store_n(scanner *sc, int length, va_list *args)
{
    size_t count = sc->consumed;
    if (length == 2) {
        signed char *ptr = va_arg(*args, signed char *);
        if (ptr == NULL || count > 127) {
            return ptr == NULL ? SC_FORMAT : SC_MATCH;
        }
        *ptr = (signed char)count;
    } else if (length == 1) {
        short *ptr = va_arg(*args, short *);
        if (ptr == NULL || count > SHRT_MAX) {
            return ptr == NULL ? SC_FORMAT : SC_MATCH;
        }
        *ptr = (short)count;
    } else if (length == 3) {
        long *ptr = va_arg(*args, long *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (long)count;
    } else if (length == 4) {
        long long *ptr = va_arg(*args, long long *);
        if (ptr == NULL) {
            return SC_FORMAT;
        }
        *ptr = (long long)count;
    } else {
        int *ptr = va_arg(*args, int *);
        if (ptr == NULL || count > INT_MAX) {
            return ptr == NULL ? SC_FORMAT : SC_MATCH;
        }
        *ptr = (int)count;
    }
    return SC_OK;
}

static int is_custom(char a, char b)
{
    return (a == 'R' && b == 'o') || (a == 'Z' && b == 'r') ||
           (a == 'C' && (b == 'v' || b == 'V'));
}

static int scan_format(scanner *sc, const char *format, va_list *args)
{
    int assigned = 0;

    if (format == NULL) {
        return EOF;
    }
    while (*format != '\0') {
        int code;
        if (isspace((unsigned char)*format)) {
            while (isspace((unsigned char)*format)) {
                format++;
            }
            sc_skip_ws(sc);
            continue;
        }
        if (*format != '%') {
            int ch = sc_get(sc);
            if (ch == EOF) {
                return assigned == 0 ? EOF : assigned;
            }
            if (ch != *format) {
                sc_unget(sc, ch);
                return assigned;
            }
            format++;
            continue;
        }
        format++;
        {
            int suppressed = 0;
            int width = -1;
            int length = 0;
            if (*format == '*') {
                suppressed = 1;
                format++;
            }
            if (format[0] != '\0' && format[1] != '\0' && is_custom(format[0], format[1])) {
                char kind = format[0];
                char reg = format[1];
                format += 2;
                if (kind == 'R') {
                    code = scan_roman(sc, suppressed, args);
                } else if (kind == 'Z') {
                    code = scan_zeckendorf(sc, suppressed, args);
                } else {
                    int base;
                    int *dummy = NULL;
                    if (!suppressed) {
                        dummy = va_arg(*args, int *);
                        if (dummy == NULL) {
                            return assigned == 0 ? EOF : assigned;
                        }
                    }
                    base = va_arg(*args, int);
                    if (base < 2 || base > 36) {
                        base = 10;
                    }
                    code = scan_integer(sc, base, reg == 'V' ? 2 : 1, -1, 0, 1, suppressed, dummy, args);
                }
            } else {
                while (isdigit((unsigned char)*format)) {
                    int digit = *format - '0';
                    if (width < 0) {
                        width = 0;
                    }
                    if (width > (INT_MAX - digit) / 10) {
                        return assigned == 0 ? EOF : assigned;
                    }
                    width = width * 10 + digit;
                    format++;
                }
                if (*format == 'h') {
                    format++;
                    length = (*format == 'h') ? 2 : 1;
                    if (length == 2) {
                        format++;
                    }
                } else if (*format == 'l') {
                    format++;
                    length = (*format == 'l') ? 4 : 3;
                    if (length == 4) {
                        format++;
                    }
                } else if (*format == 'L') {
                    length = 5;
                    format++;
                }
                if (*format == '\0') {
                    return assigned == 0 ? EOF : assigned;
                }
                if (*format == 'd') {
                    code = scan_integer(sc, 10, 1, width, length, 1, suppressed, NULL, args);
                } else if (*format == 'i') {
                    code = scan_i(sc, width, length, suppressed, args);
                } else if (*format == 'u') {
                    code = scan_integer(sc, 10, 1, width, length, 0, suppressed, NULL, args);
                } else if (*format == 'o') {
                    code = scan_integer(sc, 8, 1, width, length, 0, suppressed, NULL, args);
                } else if (*format == 'x' || *format == 'X') {
                    code = scan_integer(sc, 16, 0, width, length, 0, suppressed, NULL, args);
                } else if (*format == 'f' || *format == 'F' || *format == 'e' || *format == 'E' ||
                           *format == 'g' || *format == 'G') {
                    code = scan_float(sc, length, width, suppressed, args);
                } else if (*format == 's') {
                    code = scan_string(sc, width, suppressed, args);
                } else if (*format == 'c') {
                    code = scan_char(sc, width, suppressed, args);
                } else if (*format == '[') {
                    format++;
                    code = scan_set(sc, &format, width, suppressed, args);
                    format--;
                } else if (*format == 'n') {
                    code = store_n(sc, length, args);
                } else if (*format == '%') {
                    int ch = sc_get(sc);
                    if (ch == EOF) {
                        code = SC_EOF;
                    } else if (ch != '%') {
                        sc_unget(sc, ch);
                        code = SC_MATCH;
                    } else {
                        code = SC_OK;
                    }
                } else {
                    code = SC_FORMAT;
                }
                if (*format != '\0' && *format != '[') {
                    format++;
                } else if (*format == '[') {
                    format++;
                }
            }
        }
        if (code == SC_ASSIGNED) {
            assigned++;
        } else if (code == SC_OK) {
            continue;
        } else if (code == SC_EOF) {
            return assigned == 0 ? EOF : assigned;
        } else {
            return assigned;
        }
    }
    return assigned;
}

int overfscanf(FILE *stream, const char *format, ...)
{
    scanner sc;
    va_list args;
    int count;

    if (stream == NULL || format == NULL) {
        return EOF;
    }
    memset(&sc, 0, sizeof(sc));
    sc.file = stream;
    va_start(args, format);
    count = scan_format(&sc, format, &args);
    va_end(args);
    return count;
}

int oversscanf(const char *str, const char *format, ...)
{
    scanner sc;
    va_list args;
    int count;

    if (str == NULL || format == NULL) {
        return EOF;
    }
    memset(&sc, 0, sizeof(sc));
    sc.str = str;
    va_start(args, format);
    count = scan_format(&sc, format, &args);
    va_end(args);
    return count;
}
