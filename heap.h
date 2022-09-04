#include <stdio.h>
#include <stdint.h>

#ifndef HEAP_H
#define HEAP_H
struct memory_chunk_t
{
    int checksum;
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    int free;
}__attribute__((__packed__));

struct memory_manager_t
{
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
}__attribute__((__packed__));

enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

#endif



void *heap_malloc(size_t size);
void heap_free(void *address);
int heap_validate(void);

int heap_setup(void);
void heap_clean(void);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
size_t   heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);
void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);
