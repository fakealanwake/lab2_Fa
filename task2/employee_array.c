#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "employee_array.h"
#include "func.h"

#define INITIAL_CAPACITY 8
#define LINE_SIZE 512

void free_employees(employee **employees, size_t *count) {
    if (employees != NULL) {
        free(*employees);
        *employees = NULL;
    }
    if (count != NULL) {
        *count = 0;
    }
}

static status parse_employee_line(const char *line, employee *out) {
    employee emp;
    char id_str[SIZE];
    char salary_str[SIZE];
    const char *p = line;
    status st;

    if ((st = next_token(&p, id_str, sizeof(id_str))) != SUCCESS) return st;
    if ((st = parse_ull_base(id_str, 10, &emp.id)) != SUCCESS) return st;

    if ((st = next_token(&p, emp.first_name, sizeof(emp.first_name))) != SUCCESS) return st;
    if ((st = parse_name(emp.first_name)) != SUCCESS) return st;

    if ((st = next_token(&p, emp.last_name, sizeof(emp.last_name))) != SUCCESS) return st;
    if ((st = parse_name(emp.last_name)) != SUCCESS) return st;

    if ((st = next_token(&p, salary_str, sizeof(salary_str))) != SUCCESS) return st;
    if ((st = parse_salary(salary_str, &emp.salary)) != SUCCESS) return st;

    if (!is_blank(p)) return ERROR_INVALID_DATA; /* лишние данные в строке */

    *out = emp;
    return SUCCESS;
}

static status ensure_capacity(employee **arr, size_t *capacity, size_t count) {
    if (count < *capacity) return SUCCESS;
    if (*capacity > SIZE_MAX / 2 / sizeof(employee)) return ERROR_OVERFLOW;

    size_t new_capacity = *capacity * 2;
    employee *tmp = realloc(*arr, new_capacity * sizeof(employee));
    if (tmp == NULL) return ERROR_MEMORY_ALLOCATION;

    *arr = tmp;
    *capacity = new_capacity;
    return SUCCESS;
}

status read_employees_from_file(const char *filename, employee **employees, size_t *count) {
    if (filename == NULL || employees == NULL || count == NULL) {
        return ERROR_INVALID_ARGUMENTS;
    }
    *employees = NULL;
    *count = 0;

    FILE *file = fopen(filename, "r");
    if (file == NULL) return ERROR_FILE_OPEN;

    size_t capacity = INITIAL_CAPACITY;
    size_t size = 0;
    employee *arr = malloc(capacity * sizeof(employee));
    if (arr == NULL) {
        fclose(file);
        return ERROR_MEMORY_ALLOCATION;
    }

    status st = SUCCESS;
    char line[LINE_SIZE];

    while (st == SUCCESS && fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        if (line[len - 1] != '\n' && !feof(file)) {
            st = ERROR_TOKEN_TOO_LONG; /* строка длиннее буфера */
            break;
        }
        if (is_blank(line)) continue;

        employee emp;
        st = parse_employee_line(line, &emp);
        if (st != SUCCESS) break;

        st = ensure_capacity(&arr, &capacity, size);
        if (st != SUCCESS) break;

        arr[size++] = emp;
    }

    if (st == SUCCESS && ferror(file)) st = ERROR_FILE_READ;
    if (st == SUCCESS && size == 0) st = ERROR_EMPTY_FILE;
    fclose(file);

    if (st != SUCCESS) {
        free(arr);
        return st;
    }

    if (st != SUCCESS) {
        free(arr);
        return st;
    }

    *employees = arr;
    *count = size;
    return SUCCESS;
}

status write_employees_to_file(const char *filename, const employee *employees, size_t count) {
    if (filename == NULL || (employees == NULL && count > 0)) {
        return ERROR_INVALID_ARGUMENTS;
    }

    FILE *file = fopen(filename, "w");
    if (file == NULL) return ERROR_FILE_OPEN;

    status st = SUCCESS;
    for (size_t i = 0; i < count && st == SUCCESS; ++i) {
        if (fprintf(file, "%llu %s %s %.2f\n",
                    employees[i].id,
                    employees[i].first_name,
                    employees[i].last_name,
                    employees[i].salary) < 0) {
            st = ERROR_FILE_WRITE;
        }
    }

    if (fclose(file) != 0 && st == SUCCESS) st = ERROR_FILE_WRITE;
    return st;
}