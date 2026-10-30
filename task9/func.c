#include "func.h"
#include <stdarg.h>
#include <stddef.h>
#include <math.h>

#define MAX_ITERATIONS 64
#define MAX_DENOMINATOR 1000000000LL

static long long gcd(long long a, long long b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long long t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static status validate_epsilon(double epsilon) {
    if (!isfinite(epsilon) || epsilon <= 0.0 || epsilon >= 1.0) {
        return ERROR_INVALID_EPSILON;
    }
    return SUCCESS;
}

static status find_denominator(double x, double epsilon, long long *out_q) {
    if (out_q == NULL) {
        return ERROR_NULL_POINTER;
    }
    status st = validate_epsilon(epsilon);
    if (st != SUCCESS) {
        return st;
    }

    double val = x;
    long long p0 = 0, q0 = 1;
    long long p1 = 1, q1 = 0;

    for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
        double fl = floor(val);
        if (fl > (double)MAX_DENOMINATOR) {
            return ERROR_LIMIT_EXCEEDED;
        }
        long long a = (long long)fl;
        if (q1 != 0 && a > (MAX_DENOMINATOR - q0) / q1) {
            return ERROR_LIMIT_EXCEEDED;
        }

        long long p2 = a * p1 + p0;
        long long q2 = a * q1 + q0;
        if (q2 <= 0) {
            return ERROR_LIMIT_EXCEEDED;
        }

        if (fabs(x - (double)p2 / (double)q2) < epsilon) {
            *out_q = q2;
            return SUCCESS;
        }

        double diff = val - fl;
        if (diff <= 0.0) {
            return ERROR_LIMIT_EXCEEDED;
        }

        val = 1.0 / diff;
        p0 = p1;
        q0 = q1;
        p1 = p2;
        q1 = q2;
    }

    return ERROR_LIMIT_EXCEEDED;
}

status is_finite_representation(double num, int base, double epsilon, bool *result) {
    if (result == NULL) {
        return ERROR_NULL_POINTER;
    }
    status st = validate_epsilon(epsilon);
    if (st != SUCCESS) {
        return st;
    }
    if (base < 2) {
        return ERROR_INVALID_BASE;
    }
    if (!isfinite(num) || num <= 0.0 || num >= 1.0) {
        return ERROR_INVALID_FRACTION;
    }

    long long q = 0;
    st = find_denominator(num, epsilon, &q);
    if (st != SUCCESS) {
        return st;
    }

    long long b = (long long)base;
    long long g = gcd(q, b);
    while (g > 1) {
        while (q % g == 0) {
            q /= g;
        }
        g = gcd(q, b);
    }

    *result = (q == 1);
    return SUCCESS;
}

status process_fractions(double epsilon, int base, bool *results, int count, ...) {
    if (results == NULL) {
        return ERROR_NULL_POINTER;
    }
    status st = validate_epsilon(epsilon);
    if (st != SUCCESS) {
        return st;
    }
    if (base < 2) {
        return ERROR_INVALID_BASE;
    }
    if (count <= 0) {
        return ERROR_INVALID_COUNT;
    }

    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; ++i) {
        double fraction = va_arg(args, double);
        st = is_finite_representation(fraction, base, epsilon, &results[i]);
        if (st != SUCCESS) {
            va_end(args);
            return st;
        }
    }

    va_end(args);
    return SUCCESS;
}