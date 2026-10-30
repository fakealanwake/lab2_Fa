#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

#include "status.h"

typedef struct vector {
    const double *coords;
} vector;

typedef double (*norm_fn)(const double *coords, size_t n, const void *param);

typedef struct norm_result {
    const vector **vectors;
    size_t count;
} norm_result;

double norm_inf(const double *coords, size_t n, const void *param);
double norm_p(const double *coords, size_t n, const void *param);
double norm_A(const double *coords, size_t n, const void *param);

status matrix_is_spd(const double *matrix, size_t n, double eps);

/*
 * После именованных аргументов идут vector_count указателей const vector *,
 * затем norm_count пар (norm_fn, const void *).
 * Для каждой нормы в результат попадают все векторы максимальной длины.
 */
status find_longest(size_t n, double eps, norm_result **results,
                    size_t vector_count, size_t norm_count, ...);
void free_norm_results(norm_result *results, size_t norm_count);

#endif
