#ifndef LLP_LAB1_ALLOCATOR_H
#define LLP_LAB1_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>

typedef struct Allocator Allocator;

Allocator* alloc_create(char const* filename, size_t initial_size);
void* alloc_malloc(Allocator* allocator, size_t size);
void alloc_free(Allocator* allocator, void* ptr);
void alloc_destroy(Allocator* allocator);

#endif //LLP_LAB1_ALLOCATOR_H
