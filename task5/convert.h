#ifndef CONVERT_H
#define CONVERT_H

#include <stddef.h>

#include "status.h"

status format_roman(int value, char **out);
status format_zeckendorf(unsigned int value, char **out);
status format_int_base(int value, int base, int uppercase, char **out);
status format_to_decimal(const char *number, int base, int uppercase, char **out);
status format_memory(const void *data, size_t nbytes, char **out);

#endif
