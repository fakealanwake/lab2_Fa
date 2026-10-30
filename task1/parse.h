#ifndef PARSE_H
#define PARSE_H

#include "status.h"

status parse_flag(const char *text, char *out);
status parse_seed(const char *text, unsigned int *out);

#endif