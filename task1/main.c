#include <stdio.h>
#include <stdlib.h>

#include "func.h"
#include "parse.h"
#include "status.h"

void print_error(const status err) {
    switch (err) {
        case ERROR_NULL_POINTER:
            printf("Ошибка: Передан нулевой указатель.\n");
            break;
        case ERROR_INVALID_ARGUMENTS:
            printf("Ошибка: Некорректное количество или значение аргументов.\n");
            break;
        case ERROR_MEMORY_ALLOCATION:
            printf("Ошибка: Не удалось выделить память.\n");
            break;
        case ERROR_OVERFLOW:
            printf("Ошибка: Переполнение переменной.\n");
            break;
        case ERROR_UNKNOWN_FLAG:
            printf("Ошибка: Неизвестный флаг (допустимы -l, -r, -u, -n, -c).\n");
            break;
        case ERROR_INVALID_NUMBER:
            printf("Ошибка: Seed должен быть неотрицательным целым числом, помещающимся в unsigned int.\n");
            break;
        default:
            printf("Ошибка: Неизвестный код ошибки (%d).\n", (int)err);
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Использование: %s <флаг -l|-r|-u|-n|-c> <строка> [seed строки...]\n",
               argc > 0 ? argv[0] : "program");
        print_error(ERROR_INVALID_ARGUMENTS);
        return ERROR_INVALID_ARGUMENTS;
    }

    char flag;
    status st = parse_flag(argv[1], &flag);
    if (st != SUCCESS) {
        print_error(st);
        return st;
    }

    if ((flag != 'c' && argc != 3) || (flag == 'c' && argc < 4)) {
        print_error(ERROR_INVALID_ARGUMENTS);
        return ERROR_INVALID_ARGUMENTS;
    }

    if (flag == 'l') {
        size_t length = 0;
        st = str_length(argv[2], &length);
        if (st != SUCCESS) {
            print_error(st);
            return st;
        }
        printf("Длина строки: %zu\n", length);
        return SUCCESS;
    }

    char *result = NULL;

    switch (flag) {
        case 'r':
            st = str_reverse(argv[2], &result);
            break;
        case 'u':
            st = str_upper_odd(argv[2], &result);
            break;
        case 'n':
            st = str_group_chars(argv[2], &result);
            break;
        case 'c': {
            unsigned int seed = 0;
            st = parse_seed(argv[3], &seed);
            if (st == SUCCESS) {
                st = str_concat_random(argv[2], (const char *const *)&argv[4], (size_t)(argc - 4), seed, &result);
            }
            break;
        }
        default:
            st = ERROR_UNKNOWN_FLAG;
            break;
    }

    if (st != SUCCESS) {
        print_error(st);
        return st;
    }

    printf("Результат: %s\n", result);
    free(result);
    return SUCCESS;
}