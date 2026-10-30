#ifndef FUNC_H
#define FUNC_H

#include <stdbool.h>
#include "status.h"

status is_finite_representation(double num, int base, double epsilon, bool *result);

status process_fractions(double epsilon, int base, bool *results, int count, ...);

#endif