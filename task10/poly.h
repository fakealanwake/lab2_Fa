#ifndef POLY_H
#define POLY_H

#include "status.h"

/* Коэффициенты f0..fn передаются после degree. В *out записывается массив g0..gn. */
status shift_polynomial(double eps, double a, double **out, int degree, ...);
void free_coefficients(double *coeffs);

#endif
