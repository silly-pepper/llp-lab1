#ifndef LLP_LAB1_TYPES_H
#define LLP_LAB1_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum __attribute__((__packed__)) Types {
    DIR = 1,
    INT = 2,
    STR = 4,
    FLOAT = 8,
    BOOL = 16
} Types;

typedef struct String {
    uint64_t size;
    char* data;
} String;

typedef union Value {
    int32_t int_value;
    String str_value;
    float float_value;
    bool bool_value;
} Value;

typedef struct Node Node;

#endif //LLP_LAB1_TYPES_H
