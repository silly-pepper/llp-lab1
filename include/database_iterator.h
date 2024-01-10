#ifndef LLP_LAB1_DATABASE_ITERATOR_H
#define LLP_LAB1_DATABASE_ITERATOR_H

#include "types.h"

typedef struct Iterator {
    Node* _ptr;
} Iterator;

Node const* iterator_get(Iterator const* it);
Types iterator_get_type(Iterator const* it);
Value const* iterator_get_value(Iterator const* it);
char const* iterator_get_name(Iterator const* it);
bool iterator_is_valid(Iterator const* it);
bool iterator_has_next(Iterator const* it);
bool iterator_next(Iterator* it);

#endif //LLP_LAB1_DATABASE_ITERATOR_H
