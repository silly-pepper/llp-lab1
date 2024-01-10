#include "database_iterator.h"
#include "internals.h"

#include <stddef.h>

Node const* iterator_get(Iterator const* it) {
    return it ? it->_ptr : NULL;
}

bool iterator_is_valid(Iterator const* it) {
    return it != NULL && it->_ptr != NULL;
}

Types iterator_get_type(Iterator const* it) {
    if (!iterator_is_valid(it)) return 0;
    return it->_ptr->type;
}

Value const* iterator_get_value(Iterator const* it) {
    if (!iterator_is_valid(it) || it->_ptr->type == DIR) return NULL;
    return &it->_ptr->data;
}

char const* iterator_get_name(Iterator const* it) {
    if (!iterator_is_valid(it)) return NULL;
    return it->_ptr->name;
}

bool iterator_has_next(Iterator const* it) {
    return iterator_is_valid(it) && it->_ptr->next != NULL;
}

bool iterator_next(Iterator* it) {
    if (!iterator_has_next(it)) return false;
    it->_ptr = it->_ptr->next;
    return true;
}

