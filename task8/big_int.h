#ifndef BIG_INT_H
#define BIG_INT_H
 
#include "status.h"
 
status validate_string(const char *str, int base);
status add_large_numbers(const char *num1, const char *num2, int base, char **result);
status sum_multiple_numbers(int base, int count, char **result, ...);
 
#endif
 