#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "status.h"
#include "vector.h"

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
            printf("Ошибка: некорректные размерность или количество.\n");
            break;
        case ERROR_INVALID_NORM:
            printf("Ошибка: норма не вычислилась для переданных данных.\n");
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

static void print_vectors(const char *title, const norm_result *result, size_t n)
{
    size_t i;
    size_t j;

    printf("%s (%zu):\n", title, result->count);
    for (i = 0; i < result->count; i++) {
        printf("  (");
        for (j = 0; j < n; j++) {
            if (j > 0) {
                printf(", ");
            }
            printf("%g", result->vectors[i]->coords[j]);
        }
        printf(")\n");
    }
}

static int contains(const norm_result *result, const vector *item)
{
    size_t i;
    for (i = 0; i < result->count; i++) {
        if (result->vectors[i] == item) {
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    double c1[] = {3.0, 4.0};
    double c2[] = {0.0, 5.0};
    double c3[] = {-5.0, 0.0};
    double c4[] = {6.0, 8.0};
    vector v1 = {c1};
    vector v2 = {c2};
    vector v3 = {c3};
    vector v4 = {c4};
    double p2 = 2.0;
    double p1 = 1.0;
    double bad_p = 0.5;
    double identity[] = {1.0, 0.0, 0.0, 1.0};
    double scaled[] = {2.0, 0.0, 0.0, 2.0};
    norm_result *results = NULL;
    status st;
    int failed = 0;

    st = matrix_is_spd(identity, 2, 1e-9);
    printf("Единичная матрица положительно определена: %s\n", st == SUCCESS ? "да" : "нет");
    if (st != SUCCESS) {
        failed++;
    }

    st = find_longest(2, 1e-9, &results, 4, 3,
                      &v1, &v2, &v3, &v4,
                      norm_inf, (const void *)NULL,
                      norm_p, (const void *)&p2,
                      norm_A, (const void *)identity);
    if (st != SUCCESS) {
        print_error(st);
        return (int)st;
    }
    print_vectors("Бесконечная норма", &results[0], 2);
    print_vectors("Евклидова норма", &results[1], 2);
    print_vectors("Норма единичной матрицы", &results[2], 2);
    if (results[0].count != 1 || !contains(&results[0], &v4)) {
        printf("Ожидался один самый длинный вектор по бесконечной норме: (6, 8)\n");
        failed++;
    }
    if (results[1].count != 1 || !contains(&results[1], &v4)) {
        printf("Ожидался один самый длинный вектор по евклидовой норме: (6, 8)\n");
        failed++;
    }
    if (results[2].count != 1 || !contains(&results[2], &v4)) {
        printf("Ожидался один самый длинный вектор по (Ax, x): (6, 8)\n");
        failed++;
    }
    free_norm_results(results, 3);
    results = NULL;

    st = find_longest(2, 1e-9, &results, 3, 3,
                      &v1, &v2, &v3,
                      norm_inf, (const void *)NULL,
                      norm_p, (const void *)&p1,
                      norm_A, (const void *)scaled);
    if (st != SUCCESS) {
        print_error(st);
        return (int)st;
    }
    print_vectors("Ничья по бесконечной норме", &results[0], 2);
    print_vectors("Манхэттенская норма p = 1", &results[1], 2);
    print_vectors("Квадратичная форма 2I", &results[2], 2);
    if (results[0].count != 2 || !contains(&results[0], &v2) || !contains(&results[0], &v3)) {
        printf("По бесконечной норме ожидались (0, 5) и (-5, 0)\n");
        failed++;
    }
    if (results[1].count != 1 || !contains(&results[1], &v1)) {
        printf("По норме p = 1 ожидался (3, 4)\n");
        failed++;
    }
    if (results[2].count != 3) {
        printf("По форме 2I все три вектора одинаковой длины\n");
        failed++;
    }
    free_norm_results(results, 3);
    results = NULL;

    st = find_longest(2, 1e-9, &results, 1, 1, &v1, norm_p, (const void *)&bad_p);
    printf("p < 1 -> %s\n", st == ERROR_INVALID_NORM ? "отклонено" : "ОШИБКА ПРОВЕРКИ");
    if (st != ERROR_INVALID_NORM) {
        failed++;
        free_norm_results(results, 1);
    }

    st = find_longest(0, 1e-9, &results, 1, 1, &v1, norm_inf, (const void *)NULL);
    printf("n = 0 -> %s\n", st == ERROR_INVALID_ARGUMENT ? "отклонено" : "ОШИБКА ПРОВЕРКИ");
    if (st != ERROR_INVALID_ARGUMENT) {
        failed++;
    }

    st = find_longest(2, 0.0, &results, 1, 1, &v1, norm_inf, (const void *)NULL);
    printf("eps = 0 -> %s\n", st == ERROR_INVALID_EPSILON ? "отклонено" : "ОШИБКА ПРОВЕРКИ");
    if (st != ERROR_INVALID_EPSILON) {
        failed++;
    }

    if (failed != 0) {
        printf("Не пройдено проверок: %d\n", failed);
        return 1;
    }
    printf("Все проверки задания 7 пройдены.\n");
    return 0;
}
