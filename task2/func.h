#ifndef FUNC_H
#define FUNC_H

#include <stddef.h>
#include <stdbool.h>
#include "status.h"

status next_token(const char **pos, char *out, size_t out_size);
bool is_blank(const char *s);
status parse_ull_base(const char *s, int base, unsigned long long *out);
status parse_salary(const char *str, double *out);
status parse_name(const char *str);
status parse_flag(const char *arg, char *out_flag);

#endif