#ifndef FUNC_H
#define FUNC_H

#include <stddef.h>
#include "status.h"

typedef struct {
    size_t line_number;
    size_t char_number;
} MatchPosition;

typedef struct {
    const char *path;
    status state;
    MatchPosition *positions;
    size_t count;
    size_t capacity;
} FileSearchResult;

status search_substring_in_file(const char *const file_path, const char *const substring, FileSearchResult *const result);

status search_in_files(const char *const substring, FileSearchResult **const results, const size_t file_count, ...);

status print_search_results(const char *const substring, const FileSearchResult *const results, const size_t file_count);

void free_search_result(FileSearchResult *const result);

void free_search_results(FileSearchResult *const results, const size_t file_count);

#endif