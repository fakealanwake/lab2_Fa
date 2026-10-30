#ifndef OVERSCANF_H
#define OVERSCANF_H

#include <stdio.h>

int overfscanf(FILE *stream, const char *format, ...);
int oversscanf(const char *str, const char *format, ...);

#endif
