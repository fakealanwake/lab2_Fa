#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "poly.h"
#include "status.h"

static void print_error(status err)
{
    switch (err) {
        case ERROR_NULL_POINTER:
            printf("Ошибка: передан нулевой указатель.\n");
            break;
        case ERROR_INVALID_EPSILON:
            printf("Ошибка: точность должна быть положительным конечным числом.\n");
            break;
        case ERROR_INVALID_ARGUMENT:
            printf("Ошибка: некорректные степень или точка разложения.\n");
            break;
        case ERROR_INVALID_COEFFICIENT:
            printf("Ошибка: коэффициент должен быть конечным числом.\n");
            break;
        case ERROR_MEMORY_ALLOCATION:
            printf("Ошибка: не удалось выделить память.\n");
            break;
        case ERROR_OVERFLOW:
            printf("Ошибка: переполнение при вычислении.\n");
            break;
        default:
            printf("Ошибка: неизвестный код (%d).\n", (int)err);
            break;
    }
}

static double eval_power(const double *c, int degree, double x)
{
    double y = 0.0;
    int i;
    for (i = degree; i >= 0; i--) {
        y = y * x + c[i];
    }
    return y;
}

static double eval_shift(const double *g, int degree, double x, double a)
{
    return eval_power(g, degree, x - a);
}

static int check_case(const char *title, double eps, double a, int degree, const double *f)
{
    double *g = NULL;
    status st;
    double points[4];
    int i;
    int failed = 0;

    if (degree == 0) {
        st = shift_polynomial(eps, a, &g, degree, f[0]);
    } else if (degree == 1) {
        st = shift_polynomial(eps, a, &g, degree, f[0], f[1]);
    } else if (degree == 2) {
        st = shift_polynomial(eps, a, &g, degree, f[0], f[1], f[2]);
    } else if (degree == 3) {
        st = shift_polynomial(eps, a, &g, degree, f[0], f[1], f[2], f[3]);
    } else {
        printf("%s: степень для демонстрации не поддержана\n", title);
        return 1;
    }
    if (st != SUCCESS) {
        printf("%s: ", title);
        print_error(st);
        return 1;
    }

    printf("%s, a = %g:", title, a);
    for (i = 0; i <= degree; i++) {
        printf(" g%d=%g", i, g[i]);
    }
    printf("\n");

    points[0] = a;
    points[1] = 0.0;
    points[2] = a + 1.5;
    points[3] = a - 2.0;
    for (i = 0; i < 4; i++) {
        double left = eval_power(f, degree, points[i]);
        double right = eval_shift(g, degree, points[i], a);
        double diff = fabs(left - right);
        if (!isfinite(diff) || diff > eps * 1000.0 + 1e-8) {
            printf("  расхождение в x=%g: %g\n", points[i], diff);
            failed = 1;
        }
    }
    free_coefficients(g);
    return failed;
}

int main(void)
{
    double quad[] = {1.0, 2.0, 3.0};
    double square[] = {4.0, -4.0, 1.0};
    double cubic[] = {-1.0, 3.0, -3.0, 1.0};
    double constant[] = {7.0};
    double *dummy = NULL;
    int failed = 0;
    status st;

    failed += check_case("1 + 2x + 3x^2", 1e-9, 1.0, 2, quad);
    failed += check_case("та же в нуле", 1e-9, 0.0, 2, quad);
    failed += check_case("(x-2)^2", 1e-9, 2.0, 2, square);
    failed += check_case("(x-1)^3", 1e-9, 1.0, 3, cubic);
    failed += check_case("константа", 1e-9, 3.0, 0, constant);

    st = shift_polynomial(0.0, 1.0, &dummy, 0, 1.0);
    printf("eps = 0 -> %s\n", st == ERROR_INVALID_EPSILON ? "отклонено" : "ОШИБКА ПРОВЕРКИ");
    if (st != ERROR_INVALID_EPSILON) {
        failed++;
    }
    free_coefficients(dummy);

    st = shift_polynomial(1e-9, 1.0, NULL, 0, 1.0);
    printf("NULL результат -> %s\n", st == ERROR_NULL_POINTER ? "отклонено" : "ОШИБКА ПРОВЕРКИ");
    if (st != ERROR_NULL_POINTER) {
        failed++;
    }

    st = shift_polynomial(1e-9, 0.0, &dummy, -1);
    printf("отрицательная степень -> %s\n", st == ERROR_INVALID_ARGUMENT ? "отклонено" : "ОШИБКА ПРОВЕРКИ");
    if (st != ERROR_INVALID_ARGUMENT) {
        failed++;
    }
    free_coefficients(dummy);

    if (failed != 0) {
        printf("Не пройдено проверок: %d\n", failed);
        return 1;
    }
    printf("Все проверки задания 10 пройдены.\n");
    return 0;
}
