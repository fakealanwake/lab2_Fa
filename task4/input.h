#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include <stdio.h>

#include "file_info.h"
#include "status.h"

status read_backup_problem(FILE *in, file_info **files, size_t *count, unsigned long long *capacity);
void free_file_list(file_info *files, size_t count);

#endif
