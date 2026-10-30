#ifndef BACKUP_H
#define BACKUP_H

#include <stddef.h>

#include "file_info.h"
#include "status.h"

typedef struct selection {
    char **names;
    size_t count;
    unsigned long long total_size;
    unsigned long long total_importance;
} selection;

status select_backup(const file_info *files, size_t count, unsigned long long capacity, selection *out);
void free_selection(selection *out);

#endif
