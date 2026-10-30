#include "poly.h"

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void free_coefficients(double *coeffs)
{
    free(coeffs);
}

status shift_polynomial(double eps, double a, double **out, int degree, ...)
{
    size_t m;
    size_t i;
    double *tmp = NULL;
    double *res = NULL;
    va_list args;
    status st = SUCCESS;

    if (out == NULL) {
        return ERROR_NULL_POINTER;
    }
    *out = NULL;
    if (!isfinite(eps) || eps <= 0.0) {
        return ERROR_INVALID_EPSILON;
    }
    if (!isfinite(a) || degree < 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    if ((size_t)degree > SIZE_MAX / sizeof(double) - 1) {
        return ERROR_OVERFLOW;
    }
    m = (size_t)degree + 1;

    tmp = (double *)malloc(m * sizeof(double));
    res = (double *)malloc(m * sizeof(double));
    if (tmp == NULL || res == NULL) {
        free(tmp);
        free(res);
        return ERROR_MEMORY_ALLOCATION;
    }

    va_start(args, degree);
    for (i = 0; i < m; i++) {
        double coeff = va_arg(args, double);
        if (!isfinite(coeff)) {
            st = ERROR_INVALID_COEFFICIENT;
            break;
        }
        tmp[i] = coeff;
    }
    va_end(args);

    if (st == SUCCESS) {
        int deg;
        for (deg = 0; deg <= degree; deg++) {
            int d = degree - deg;
            int k;
            for (k = d - 1; k >= 0; k--) {
                tmp[k] = tmp[k] + a * tmp[k + 1];
                if (!isfinite(tmp[k])) {
                    st = ERROR_OVERFLOW;
                    break;
                }
            }
            if (st != SUCCESS) {
                break;
            }
            res[deg] = tmp[0];
            for (k = 0; k < d; k++) {
                tmp[k] = tmp[k + 1];
            }
        }
    }

    if (st == SUCCESS) {
        for (i = 0; i < m; i++) {
            if (!isfinite(res[i])) {
                st = ERROR_OVERFLOW;
                break;
            }
            if (fabs(res[i]) < eps) {
                res[i] = 0.0;
            }
        }
    }

    free(tmp);
    if (st != SUCCESS) {
        free(res);
        return st;
    }
    *out = res;
    return SUCCESS;
}
