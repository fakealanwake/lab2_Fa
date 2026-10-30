#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "status.h"
#include "func.h"

static void print_status(status st) {
    switch (st) {
        case SUCCESS:
            printf("SUCCESS\n");
            break;
        case ERROR_INVALID_EPSILON:
            printf("Error: Invalid epsilon parameter.\n");
            break;
        case ERROR_INVALID_BASE:
            printf("Error: Invalid number system base.\n");
            break;
        case ERROR_INVALID_COUNT:
            printf("Error: Invalid count of arguments.\n");
            break;
        case ERROR_INVALID_FRACTION:
            printf("Error: Fraction must be in range (0, 1).\n");
            break;
        case ERROR_NULL_POINTER:
            printf("Error: Passed NULL pointer.\n");
            break;
        case ERROR_MEMORY_ALLOCATION:
            printf("Error: Memory allocation failed.\n");
            break;
        case ERROR_LIMIT_EXCEEDED:
            printf("Error: Computational limit reached.\n");
            break;
        default:
            printf("Error: Unknown error code.\n");
            break;
    }
}

int main(void) {
    double epsilon = 1e-6;
    int base = 10;
    int count = 4;

    double f1 = 0.5;
    double f2 = 0.125;
    double f3 = 0.3333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333;
    double f4 = 0.218724518346333333333333333333333666666666666666666666666666666666622222222222222817345187235418735182735418273415827346258174274561847125554721348143258172354124837651281237468154327465187245811182734545123765427814578236527417824572;

    bool *results = (bool *)malloc((size_t)count * sizeof(bool));
    if (results == NULL) {
        print_status(ERROR_MEMORY_ALLOCATION);
        return (int)ERROR_MEMORY_ALLOCATION;
    }

    printf("=== Test 1: Base = %d ===\n", base);
    status st = process_fractions(epsilon, base, results, count, f1, f2, f3, f4);
    if (st == SUCCESS) {
        double test_vals[4] = {f1, f2, f3, f4};
        for (int i = 0; i < count; ++i) {
            printf("Fraction %f in base %d: %s\n", 
                   test_vals[i], base, results[i] ? "FINITE" : "INFINITE");
        }
    } else {
        print_status(st);
    }

    base = 2;
    printf("\n=== Test 2: Base = %d ===\n", base);
    st = process_fractions(epsilon, base, results, count, f1, f2, f3, f4);
    if (st == SUCCESS) {
        double test_vals[4] = {f1, f2, f3, f4};
        for (int i = 0; i < count; ++i) {
            printf("Fraction %f in base %d: %s\n", 
                   test_vals[i], base, results[i] ? "FINITE" : "INFINITE");
        }
    } else {
        print_status(st);
    }

    printf("\n=== Test 3: Validation Error (Invalid Fraction) ===\n");
    st = process_fractions(epsilon, 10, results, 1, 1.5);
    if (st != SUCCESS) {
        print_status(st);
    }

    free(results);
    results = NULL;

    return 0;
}