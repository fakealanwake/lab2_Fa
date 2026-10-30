#ifndef EMPLOYEE_ARRAY_H
#define EMPLOYEE_ARRAY_H

#include <stddef.h>
#include "status.h"
#include "employee.h"

status read_employees_from_file(const char *filename, employee **employees, size_t *count);
status write_employees_to_file(const char *filename, const employee *employees, size_t count);
void free_employees(employee **employees, size_t *count);

#endif