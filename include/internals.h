#ifndef LLP_LAB1_INTERNALS_H
#define LLP_LAB1_INTERNALS_H

#include "types.h"

typedef struct List {
    struct List* next;
    struct List* prev;
} List;

void lst_init(List*);
void lst_remove(List*);
void lst_push(List*, void*);
void* lst_pop(List*);
void lst_print(List*);
int lst_empty(List*);

struct Node {
    Types type;
    Node* next; // todo use List
    Node* prev;
    char* name;
    union {
        Node* child;
        Value data;
    };
};

#endif //LLP_LAB1_INTERNALS_H
