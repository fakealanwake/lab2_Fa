#ifndef FUNC_H
#define FUNC_H

#include <stddef.h>

#include "status.h"

status str_length(const char *s, size_t *out);
status str_reverse(const char *s, char **out);
status str_upper_odd(const char *s, char **out);
status str_group_chars(const char *s, char **out);
status str_concat_random(const char *first, const char *const *rest, size_t rest_count, unsigned int seed, char **out);

#endif