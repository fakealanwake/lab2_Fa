#include "backup.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define BRUTE_MAX 22u
#define DP_CELLS_MAX 3000000u

void free_selection(selection *out)
{
    size_t i;

    if (out == NULL) {
        return;
    }
    if (out->names != NULL) {
        for (i = 0; i < out->count; i++) {
            free(out->names[i]);
        }
        free(out->names);
    }
    out->names = NULL;
    out->count = 0;
    out->total_size = 0;
    out->total_importance = 0;
}

static status copy_name(const char *src, char **out)
{
    size_t len;
    char *dup;

    if (src == NULL || out == NULL) {
        return ERROR_NULL_POINTER;
    }
    len = strlen(src);
    dup = (char *)malloc(len + 1);
    if (dup == NULL) {
        return ERROR_MEMORY_ALLOCATION;
    }
    memcpy(dup, src, len + 1);
    *out = dup;
    return SUCCESS;
}

static status fill_from_mask(const file_info *files, size_t count, unsigned long mask,
                             unsigned long long total_size, unsigned long long total_importance,
                             selection *out)
{
    size_t i;
    size_t written = 0;
    status st = SUCCESS;

    out->names = NULL;
    out->count = 0;
    out->total_size = total_size;
    out->total_importance = total_importance;

    for (i = 0; i < count; i++) {
        if ((mask & (1ul << i)) != 0) {
            out->count++;
        }
    }
    if (out->count == 0) {
        return SUCCESS;
    }
    out->names = (char **)calloc(out->count, sizeof(char *));
    if (out->names == NULL) {
        out->count = 0;
        return ERROR_MEMORY_ALLOCATION;
    }
    for (i = 0; i < count; i++) {
        if ((mask & (1ul << i)) == 0) {
            continue;
        }
        st = copy_name(files[i].name, &out->names[written]);
        if (st != SUCCESS) {
            free_selection(out);
            return st;
        }
        written++;
    }
    return SUCCESS;
}

static status select_brute(const file_info *files, size_t count, unsigned long long capacity, selection *out)
{
    unsigned long masks;
    unsigned long mask;
    unsigned long best_mask = 0;
    unsigned long long best_imp = 0;
    unsigned long long best_size = 0;

    masks = 1ul << count;
    for (mask = 0; mask < masks; mask++) {
        unsigned long long sz = 0;
        unsigned long long imp = 0;
        size_t i;
        int overflow = 0;

        for (i = 0; i < count; i++) {
            if ((mask & (1ul << i)) == 0) {
                continue;
            }
            if (sz > ULLONG_MAX - files[i].size || imp > ULLONG_MAX - files[i].importance) {
                overflow = 1;
                break;
            }
            sz += files[i].size;
            imp += files[i].importance;
        }
        if (overflow || sz > capacity) {
            continue;
        }
        if (imp > best_imp) {
            best_imp = imp;
            best_size = sz;
            best_mask = mask;
        }
    }
    return fill_from_mask(files, count, best_mask, best_size, best_imp, out);
}

static status select_dp(const file_info *files, size_t count, unsigned long long capacity, selection *out)
{
    unsigned long long eff = 0;
    size_t i;
    size_t cells;
    size_t width;
    unsigned long long *dp = NULL;
    unsigned char *reach = NULL;
    unsigned char *chosen = NULL;
    size_t best_w = 0;
    unsigned long long best_imp = 0;
    unsigned long long total_size = 0;
    status st = SUCCESS;
    size_t w;

    for (i = 0; i < count; i++) {
        if (files[i].size > capacity) {
            continue;
        }
        if (eff > ULLONG_MAX - files[i].size || eff + files[i].size > capacity) {
            eff = capacity;
            break;
        }
        eff += files[i].size;
    }
    if (eff > SIZE_MAX - 1) {
        return ERROR_LIMIT;
    }
    width = (size_t)eff + 1;
    if (count > SIZE_MAX / width || (count + 1) > DP_CELLS_MAX / width) {
        return ERROR_LIMIT;
    }
    cells = (count + 1) * width;

    dp = (unsigned long long *)calloc(cells, sizeof(*dp));
    reach = (unsigned char *)calloc(cells, sizeof(*reach));
    chosen = (unsigned char *)calloc(count, sizeof(*chosen));
    if (dp == NULL || reach == NULL || chosen == NULL) {
        free(dp);
        free(reach);
        free(chosen);
        return ERROR_MEMORY_ALLOCATION;
    }
    reach[0] = 1;

    for (i = 0; i < count; i++) {
        size_t row = (i + 1) * width;
        size_t prev = i * width;
        unsigned long long sz = files[i].size;
        unsigned long long imp = files[i].importance;

        memcpy(dp + row, dp + prev, width * sizeof(*dp));
        memcpy(reach + row, reach + prev, width);
        if (sz > eff) {
            continue;
        }
        for (w = (size_t)sz; w <= (size_t)eff; w++) {
            size_t from = w - (size_t)sz;
            unsigned long long cand;

            if (!reach[prev + from]) {
                continue;
            }
            if (dp[prev + from] > ULLONG_MAX - imp) {
                st = ERROR_OVERFLOW;
                break;
            }
            cand = dp[prev + from] + imp;
            if (!reach[row + w] || cand > dp[row + w]) {
                dp[row + w] = cand;
                reach[row + w] = 1;
            }
        }
        if (st != SUCCESS) {
            break;
        }
    }

    if (st == SUCCESS) {
        size_t last = count * width;
        for (w = 0; w <= (size_t)eff; w++) {
            if (reach[last + w] && dp[last + w] >= best_imp) {
                best_imp = dp[last + w];
                best_w = w;
            }
        }
        w = best_w;
        for (i = count; i > 0; i--) {
            size_t item = i - 1;
            size_t row = i * width;
            size_t prev = item * width;
            unsigned long long sz = files[item].size;
            unsigned long long imp = files[item].importance;
            int can_skip;
            int can_take;

            can_skip = reach[prev + w] && dp[prev + w] == dp[row + w];
            can_take = 0;
            if (sz <= w) {
                size_t from = w - (size_t)sz;
                if (reach[prev + from] && dp[prev + from] <= ULLONG_MAX - imp &&
                    dp[prev + from] + imp == dp[row + w]) {
                    can_take = 1;
                }
            }
            if (!can_skip && can_take) {
                chosen[item] = 1;
                w -= (size_t)sz;
            } else if (!can_skip && !can_take) {
                st = ERROR_INVALID_DATA;
                break;
            }
        }
        if (st == SUCCESS && w != 0) {
            st = ERROR_INVALID_DATA;
        }
    }

    if (st == SUCCESS) {
        size_t selected = 0;
        size_t written = 0;
        for (i = 0; i < count; i++) {
            if (!chosen[i]) {
                continue;
            }
            if (total_size > ULLONG_MAX - files[i].size) {
                st = ERROR_OVERFLOW;
                break;
            }
            total_size += files[i].size;
            selected++;
        }
        if (st == SUCCESS) {
            out->names = NULL;
            out->count = selected;
            out->total_size = total_size;
            out->total_importance = best_imp;
            if (selected > 0) {
                out->names = (char **)calloc(selected, sizeof(char *));
                if (out->names == NULL) {
                    st = ERROR_MEMORY_ALLOCATION;
                }
            }
        }
        for (i = 0; st == SUCCESS && i < count; i++) {
            if (!chosen[i]) {
                continue;
            }
            st = copy_name(files[i].name, &out->names[written]);
            if (st != SUCCESS) {
                free_selection(out);
                break;
            }
            written++;
        }
    }

    free(dp);
    free(reach);
    free(chosen);
    return st;
}

status select_backup(const file_info *files, size_t count, unsigned long long capacity, selection *out)
{
    size_t i;

    if (out == NULL) {
        return ERROR_NULL_POINTER;
    }
    out->names = NULL;
    out->count = 0;
    out->total_size = 0;
    out->total_importance = 0;
    if (files == NULL && count > 0) {
        return ERROR_NULL_POINTER;
    }
    for (i = 0; i < count; i++) {
        if (files[i].name == NULL || files[i].name[0] == '\0' || files[i].size == 0) {
            return ERROR_INVALID_DATA;
        }
    }
    if (count <= BRUTE_MAX) {
        return select_brute(files, count, capacity, out);
    }
    return select_dp(files, count, capacity, out);
}
