#include <stdio.h>
#include <stdlib.h>
#include "status.h"
#include "big_int.h"

static const char *status_message(status st) {
    switch (st) {
        case SUCCESS:                 return "успех";
        case ERROR_INVALID_ARGUMENTS: return "некорректные аргументы";
        case ERROR_MEMORY_ALLOCATION: return "не удалось выделить память";
        case ERROR_INVALID_BASE:      return "основание вне диапазона [2..36]";
        case ERROR_INVALID_DIGIT:     return "недопустимая цифра для данного основания";
        case ERROR_OVERFLOW:          return "переполнение размера";
        default:                      return "неизвестная ошибка";
    }
}

static void report(const char *title, status st, char *result) {
    if (st == SUCCESS) {
        printf("%-34s -> %s\n", title, result);
        free(result);
    } else {
        printf("%-34s -> ошибка: %s\n", title, status_message(st));
    }
}

int main(void) {
    char *result = NULL;
    status st;

    st = sum_multiple_numbers(10, 3, &result, "12300000000000000000000000000000000000000", "00000000000000000000000456", "789");
    report("10: 123 + 456 + 789", st, result);

    st = sum_multiple_numbers(16, 2, &result, "f", "1");
    report("16: f + 1", st, result);

    st = sum_multiple_numbers(16, 2, &result, "ff", "FF");
    report("16: ff + FF", st, result);

    st = sum_multiple_numbers(2, 3, &result, "1", "1", "1");
    report("2: 1 + 1 + 1", st, result);

    st = sum_multiple_numbers(10, 2, &result, "000", "0");
    report("10: 000 + 0", st, result);

    st = sum_multiple_numbers(10, 3, &result, "0007", "0003", "00");
    report("10: 0007 + 0003 + 00", st, result);

    st = sum_multiple_numbers(36, 2, &result, "ZZZZ", "1");
    report("36: ZZZZ + 1", st, result);

    st = sum_multiple_numbers(10, 1, &result, "42");
    report("10: одно число 42", st, result);

    st = sum_multiple_numbers(10, 2,
        &result,
        "99999999999999999999999999999999999999999999999999",
        "1");
    report("10: 50 девяток + 1", st, result);

    st = sum_multiple_numbers(10, 2, &result, "12a", "5");
    report("10: 12a + 5", st, result);

    st = sum_multiple_numbers(2, 2, &result, "101", "102");
    report("2: 101 + 102", st, result);

    st = sum_multiple_numbers(1, 2, &result, "1", "1");
    report("основание 1", st, result);

    st = sum_multiple_numbers(37, 2, &result, "1", "1");
    report("основание 37", st, result);

    st = sum_multiple_numbers(10, 0, &result, "1");
    report("count = 0", st, result);

    st = sum_multiple_numbers(10, 2, &result, "", "1");
    report("пустая строка", st, result);

    st = sum_multiple_numbers(10, 2, &result, NULL, "1");
    report("NULL вместо строки", st, result);

    return 0;
}