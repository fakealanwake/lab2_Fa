#include <stdio.h>
#include "status.h"
#include "func.h"

int main(void) {
    FileSearchResult *results = NULL;

    search_in_files("ab", &results, 3, "start_end.txt", "overlap.txt", "missing.txt");
    print_search_results("ab", results, 3);
    free_search_results(results, 3);

    search_in_files("aa", &results, 2, "overlap.txt", "one_char.txt");
    print_search_results("aa", results, 2);
    free_search_results(results, 2);

    search_in_files("b\t\tc\n\n d", &results, 4, "spaces.txt", "start_end.txt", "empty.txt", "newlines.txt");
    print_search_results("b\t\tc\n\n d", results, 4);
    free_search_results(results, 4);

    search_in_files("\n\n", &results, 1, "newlines.txt");
    print_search_results("\n\n", results, 1);
    free_search_results(results, 1);

    search_in_files("", &results, 2, "newlines.txt", "spaces.txt");
    print_search_results("", results, 2);
    free_search_results(results, 2);

    return 0;
}