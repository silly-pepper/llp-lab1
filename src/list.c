#include <stdio.h>
#include <assert.h>
#include "internals.h"

// double-linked, circular list. double-linked makes remove
// fast. circular simplifies code, because don't have to check for
// empty list in insert and remove.

void lst_init(List* lst) {
    lst->next = lst;
    lst->prev = lst;
}

int lst_empty(List* lst) {
    return lst->next == lst;
}

void lst_remove(List* e) {
    e->prev->next = e->next;
    e->next->prev = e->prev;
}

void* lst_pop(List* lst) {
    assert(lst->next != lst);
    List* p = lst->next;
    lst_remove(p);
    return (void*) p;
}

void lst_push(List* lst, void* p) {
    List* e = (List*) p;
    e->next = lst->next;
    e->prev = lst;
    lst->next->prev = p;
    lst->next = e;
}

void lst_print(List* lst) {
    for (List* p = lst->next; p != lst; p = p->next) {
        fprintf(stderr, " %p", p);
    }
    fprintf(stderr, "\n");
}
