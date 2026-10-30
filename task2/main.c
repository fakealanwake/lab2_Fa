#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "employee.h"
#include "employee_array.h"
#include "func.h"
#include "status.h"

#define EPSILON 1e-9

static int compare_employees(const employee *a, const employee *b, const double eps) {
    if (fabs(a->salary - b->salary) > eps) {
        return (a->salary > b->salary) ? 1 : -1;
    }

    int c = strcmp(a->last_name, b->last_name);
    if (c != 0) return (c < 0) ? -1 : 1;

    c = strcmp(a->first_name, b->first_name);
    if (c != 0) return (c < 0) ? -1 : 1;

    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;
}

static int compare_asc(const void *a, const void *b) {
    return compare_employees((const employee *)a, (const employee *)b, EPSILON);
}

static int compare_desc(const void *a, const void *b) {
    return compare_employees((const employee *)b, (const employee *)a, EPSILON);
}

static bool is_same_file(const char *path_a, const char *path_b) {
    struct stat sa, sb;

    if (stat(path_a, &sa) != 0 || stat(path_b, &sb) != 0) {
        return false;
    }

    return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
}

void print_error(const status err) {
    switch (err) {
        case ERROR_INVALID_ARGUMENTS:
            printf("Ошибка: Некорректное количество или значение аргументов.\n");
            break;
        case ERROR_UNKNOWN_FLAG:
            printf("Ошибка: Неизвестный флаг (допустимы -a, /a, -d, /d).\n");
            break;
        case ERROR_FILE_OPEN:
            printf("Ошибка: Не удалось открыть файл.\n");
            break;
        case ERROR_FILE_READ:
            printf("Ошибка: Ошибка чтения из файла.\n");
            break;
        case ERROR_FILE_WRITE:
            printf("Ошибка: Ошибка записи в файл.\n");
            break;
        case ERROR_MEMORY_ALLOCATION:
            printf("Ошибка: Не удалось выделить память.\n");
            break;
        case ERROR_INVALID_DATA:
            printf("Ошибка: Неверный формат данных во входном файле.\n");
            break;
        case ERROR_OVERFLOW:
            printf("Ошибка: Переполнение переменной.\n");
            break;
        case ERROR_TOKEN_TOO_LONG:
            printf("Ошибка: Слишком длинное значение или строка.\n");
            break;
        case ERROR_EMPTY_FILE:
            printf("Ошибка: Входной файл пуст.\n");
            break;
        default:
            printf("Ошибка: Неизвестный код ошибки (%d).\n", (int)err);
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Использование: %s <входной_файл> <флаг -a|-d|/a|/d> <выходной_файл>\n",
               argc > 0 ? argv[0] : "program");
        print_error(ERROR_INVALID_ARGUMENTS);
        return ERROR_INVALID_ARGUMENTS;
    }

    char flag;
    status st = parse_flag(argv[2], &flag);
    if (st != SUCCESS) {
        print_error(st);
        return st;
    }

    if (is_same_file(argv[1], argv[3])) {
        printf("Входной и выходной файлы не должны совпадать.\n");
        print_error(ERROR_INVALID_ARGUMENTS);
        return ERROR_INVALID_ARGUMENTS;
    }

    employee *employees = NULL;
    size_t count = 0;

    st = read_employees_from_file(argv[1], &employees, &count);
    if (st != SUCCESS) {
        print_error(st);
        return st;
    }

    qsort(employees, count, sizeof(employee), (flag == 'a') ? compare_asc : compare_desc);

    st = write_employees_to_file(argv[3], employees, count);
    const size_t written = count;
    free_employees(&employees, &count);

    if (st != SUCCESS) {
        print_error(st);
        return st;
    }

    printf("Готово. Записано записей: %zu\n", written);
    return SUCCESS;
}