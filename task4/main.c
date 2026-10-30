#include <stdio.h>

#include "backup.h"
#include "input.h"
#include "status.h"

static void print_error(status err)
{
    switch (err) {
        case ERROR_NULL_POINTER:
            fprintf(stderr, "Ошибка: передан нулевой указатель.\n");
            break;
        case ERROR_INVALID_DATA:
            fprintf(stderr, "Ошибка: неверный формат входных данных.\n");
            break;
        case ERROR_MEMORY_ALLOCATION:
            fprintf(stderr, "Ошибка: не удалось выделить память.\n");
            break;
        case ERROR_OVERFLOW:
            fprintf(stderr, "Ошибка: переполнение при вычислении.\n");
            break;
        case ERROR_LIMIT:
            fprintf(stderr, "Ошибка: слишком большой размер входных данных.\n");
            break;
        case ERROR_IO:
            fprintf(stderr, "Ошибка: сбой чтения входного потока.\n");
            break;
        default:
            fprintf(stderr, "Ошибка: неизвестный код (%d).\n", (int)err);
            break;
    }
}

static status print_selection(const selection *sel)
{
    size_t i;

    if (printf("TOTAL_SIZE %llu\n", sel->total_size) < 0 ||
        printf("TOTAL_IMPORTANCE %llu\n", sel->total_importance) < 0 ||
        printf("FILES %zu\n", sel->count) < 0) {
        return ERROR_IO;
    }
    for (i = 0; i < sel->count; i++) {
        if (printf("%s\n", sel->names[i]) < 0) {
            return ERROR_IO;
        }
    }
    return SUCCESS;
}

int main(void)
{
    file_info *files = NULL;
    size_t count = 0;
    unsigned long long capacity = 0;
    selection sel;
    status st;

    st = read_backup_problem(stdin, &files, &count, &capacity);
    if (st != SUCCESS) {
        print_error(st);
        return (int)st;
    }

    st = select_backup(files, count, capacity, &sel);
    free_file_list(files, count);
    if (st != SUCCESS) {
        print_error(st);
        return (int)st;
    }

    st = print_selection(&sel);
    free_selection(&sel);
    if (st != SUCCESS) {
        print_error(st);
        return (int)st;
    }
    return SUCCESS;
}
