#include "vector.h"

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

double norm_inf(const double *coords, size_t n, const void *param)
{
    size_t i;
    double best = 0.0;

    (void)param;
    if (coords == NULL || n == 0) {
        return nan("");
    }
    for (i = 0; i < n; i++) {
        double value = fabs(coords[i]);
        if (!isfinite(value)) {
            return nan("");
        }
        if (value > best) {
            best = value;
        }
    }
    return best;
}

double norm_p(const double *coords, size_t n, const void *param)
{
    const double *p_ptr;
    double p;
    double sum = 0.0;
    size_t i;

    if (coords == NULL || param == NULL || n == 0) {
        return nan("");
    }
    p_ptr = (const double *)param;
    p = *p_ptr;
    if (!isfinite(p) || p < 1.0) {
        return nan("");
    }
    for (i = 0; i < n; i++) {
        double term;
        if (!isfinite(coords[i])) {
            return nan("");
        }
        term = pow(fabs(coords[i]), p);
        if (!isfinite(term) || sum > HUGE_VAL - term) {
            return nan("");
        }
        sum += term;
    }
    return pow(sum, 1.0 / p);
}

double norm_A(const double *coords, size_t n, const void *param)
{
    const double *matrix;
    double sum = 0.0;
    size_t i;

    if (coords == NULL || param == NULL || n == 0) {
        return nan("");
    }
    matrix = (const double *)param;
    for (i = 0; i < n; i++) {
        double axi = 0.0;
        size_t j;
        if (!isfinite(coords[i])) {
            return nan("");
        }
        for (j = 0; j < n; j++) {
            double product = matrix[i * n + j] * coords[j];
            if (!isfinite(matrix[i * n + j]) || !isfinite(product)) {
                return nan("");
            }
            axi += product;
        }
        sum += coords[i] * axi;
        if (!isfinite(sum)) {
            return nan("");
        }
    }
    return sum;
}

status matrix_is_spd(const double *matrix, size_t n, double eps)
{
    double *lower = NULL;
    size_t i;
    size_t j;
    size_t k;
    status st = SUCCESS;

    if (matrix == NULL) {
        return ERROR_NULL_POINTER;
    }
    if (n == 0 || !isfinite(eps) || eps <= 0.0) {
        return ERROR_INVALID_ARGUMENT;
    }
    if (n > SIZE_MAX / n || n * n > SIZE_MAX / sizeof(double)) {
        return ERROR_OVERFLOW;
    }
    lower = (double *)calloc(n * n, sizeof(double));
    if (lower == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }

    for (i = 0; st == SUCCESS && i < n; i++) {
        for (j = 0; st == SUCCESS && j <= i; j++) {
            double value = matrix[i * n + j];
            if (fabs(matrix[i * n + j] - matrix[j * n + i]) > eps) {
                st = ERROR_INVALID_ARGUMENT;
                break;
            }
            for (k = 0; k < j; k++) {
                value -= lower[i * n + k] * lower[j * n + k];
            }
            if (i == j) {
                if (!(value > eps)) {
                    st = ERROR_INVALID_ARGUMENT;
                    break;
                }
                lower[i * n + j] = sqrt(value);
            } else {
                if (lower[j * n + j] == 0.0) {
                    st = ERROR_INVALID_ARGUMENT;
                    break;
                }
                lower[i * n + j] = value / lower[j * n + j];
            }
        }
    }

    free(lower);
    return st;
}

void free_norm_results(norm_result *results, size_t norm_count)
{
    size_t i;

    if (results == NULL) {
        return;
    }
    for (i = 0; i < norm_count; i++) {
        free((void *)results[i].vectors);
        results[i].vectors = NULL;
        results[i].count = 0;
    }
    free(results);
}

status find_longest(size_t n, double eps, norm_result **results,
                    size_t vector_count, size_t norm_count, ...)
{
    const vector **vectors = NULL;
    norm_fn *functions = NULL;
    const void **params = NULL;
    double *values = NULL;
    norm_result *out = NULL;
    va_list args;
    size_t i;
    size_t k;
    status st = SUCCESS;

    if (results == NULL) {
        return ERROR_NULL_POINTER;
    }
    *results = NULL;
    if (!isfinite(eps) || eps <= 0.0) {
        return ERROR_INVALID_EPSILON;
    }
    if (n == 0 || vector_count == 0 || norm_count == 0) {
        return ERROR_INVALID_ARGUMENT;
    }

    vectors = (const vector **)calloc(vector_count, sizeof(*vectors));
    functions = (norm_fn *)calloc(norm_count, sizeof(*functions));
    params = (const void **)calloc(norm_count, sizeof(*params));
    values = (double *)malloc(vector_count * sizeof(*values));
    out = (norm_result *)calloc(norm_count, sizeof(*out));
    if (vectors == NULL || functions == NULL || params == NULL || values == NULL || out == NULL) {
        st = ERROR_MEMORY_ALLOCATION;
    }

    if (st == SUCCESS) {
        va_start(args, norm_count);
        for (i = 0; i < vector_count; i++) {
            const vector *item = va_arg(args, const vector *);
            if (item == NULL || item->coords == NULL) {
                st = ERROR_NULL_POINTER;
                break;
            }
            vectors[i] = item;
        }
        for (k = 0; st == SUCCESS && k < norm_count; k++) {
            norm_fn fn = va_arg(args, norm_fn);
            const void *param = va_arg(args, const void *);
            if (fn == NULL) {
                st = ERROR_NULL_POINTER;
                break;
            }
            functions[k] = fn;
            params[k] = param;
        }
        va_end(args);
    }

    for (k = 0; st == SUCCESS && k < norm_count; k++) {
        double best = 0.0;
        size_t winners = 0;
        size_t written = 0;
        int have_best = 0;

        for (i = 0; i < vector_count; i++) {
            double length = functions[k](vectors[i]->coords, n, params[k]);
            if (!isfinite(length) || length < -eps) {
                st = ERROR_INVALID_NORM;
                break;
            }
            if (length < 0.0) {
                length = 0.0;
            }
            values[i] = length;
            if (!have_best || length > best) {
                best = length;
                have_best = 1;
            }
        }
        if (st != SUCCESS) {
            break;
        }
        for (i = 0; i < vector_count; i++) {
            if (fabs(values[i] - best) <= eps) {
                winners++;
            }
        }
        out[k].vectors = (const vector **)calloc(winners, sizeof(const vector *));
        if (out[k].vectors == NULL) {
            st = ERROR_MEMORY_ALLOCATION;
            break;
        }
        out[k].count = winners;
        for (i = 0; i < vector_count; i++) {
            if (fabs(values[i] - best) <= eps) {
                out[k].vectors[written++] = vectors[i];
            }
        }
    }

    free(vectors);
    free(functions);
    free(params);
    free(values);
    if (st != SUCCESS) {
        free_norm_results(out, norm_count);
        return st;
    }
    *results = out;
    return SUCCESS;
}
