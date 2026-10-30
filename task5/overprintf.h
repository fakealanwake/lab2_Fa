#ifndef OVERPRINTF_H
#define OVERPRINTF_H

#include <stdio.h>

int overfprintf(FILE *stream, const char *format, ...);
int oversprintf(char *str, const char *format, ...);

#endif
