#ifndef EMPLOYEE_H
#define EMPLOYEE_H
#define SIZE 100

typedef struct employee {
    unsigned long long id;
    double salary;
    char first_name[SIZE];
    char last_name[SIZE];
} employee;

#endif

