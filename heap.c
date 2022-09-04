#include <stdio.h>
#include <stdint.h>
#include "heap.h"

#define PLOTEK 16
#define MEM_SIZE 4096

struct memory_manager_t memory_manager;

int main()
{

    return 0;
}

int heap_setup(void)
{
    void *p = custom_sbrk(MEM_SIZE);
    if(p == NULL)
        return -1;

    memory_manager.memory_size += MEM_SIZE;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = p;
    return 0;
}

void heap_clean(void)
{
    custom_sbrk(-memory_manager.memory_size);
    memory_manager.memory_size = 0;
    memory_manager.first_memory_chunk = NULL;
    memory_manager.memory_start = NULL;
}

void* heap_calloc(size_t number, size_t size)
{
    void* p = heap_malloc(number * size);
    if(p == NULL)
        return NULL;
    for(int i = 0; i < (int)(size * number); i++)
        *((unsigned char*)p + i) = 0;
    return p;
}

void* heap_realloc(void* memblock, size_t count)
{
    if(heap_validate())
        return NULL;
    if(memblock == NULL)
        return heap_malloc(count);
    if(get_pointer_type(memblock) != pointer_valid)
        return NULL;
    struct memory_chunk_t* a = (struct memory_chunk_t*)((unsigned char*)memblock - PLOTEK - sizeof(struct memory_chunk_t));
    if(count == 0)
    {
        heap_free(memblock);
        return NULL;
    }
    if(count < a->size)
    {
        a->size = count;
        for(unsigned int i = 0; i < PLOTEK; i++)
            *((unsigned char*)(a + 1) + PLOTEK + count + i) = '#';
        a->checksum = 0;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            a->checksum += *((unsigned char*)a + i);
        return memblock;
    }
    if(count == a->size)
        return memblock;

    if(a->next != NULL)
    {
        if((unsigned char*)a->next - (unsigned char*)a - sizeof(struct memory_chunk_t) - PLOTEK * 2 > count)
        {
            a->size = count;
            for(unsigned int i = 0; i < PLOTEK; i++)
                *((unsigned char*)memblock + count + i)= '#';
            a->checksum = 0;
            for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                a->checksum += *((unsigned char*)a + i);
            return memblock;
        }
        if(a->next->free == 1)
        {
            if((unsigned char*)a->next->next - (unsigned char*)a - sizeof(struct memory_chunk_t) - PLOTEK * 2 > count)
            {
                a->size = count;
                a->next = a->next->next;
                a->next->prev = a;
                for(unsigned int i = 0; i < PLOTEK; i++)
                    *((unsigned char*)memblock + count + i)= '#';
                a->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->checksum += *((unsigned char*)a + i);
                a->next->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->next->checksum += *((unsigned char*)(a->next) + i);
                return memblock;
            }
        }
        void *p = heap_malloc(count);
        if(p == NULL)
            return NULL;
        for(unsigned int i = 0; i < count; i++)
            *((unsigned char*)p + i) = *((unsigned char*)memblock + i);
        heap_free(memblock);
        return p;

    }
    void *p;
    while(memory_manager.memory_size - ((unsigned char*)a - (unsigned char*)memory_manager.first_memory_chunk) < count + sizeof(struct memory_chunk_t) + PLOTEK * 2)
    {
        p = custom_sbrk(MEM_SIZE);
        if((char *)p == (char *)-1)
            return NULL;
        memory_manager.memory_size += MEM_SIZE;
    }
    a->size = count;
    for(unsigned int i = 0; i < PLOTEK; i++)
        *((unsigned char*)memblock + count + i)= '#';
    a->checksum = 0;
    for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
        a->checksum += *((unsigned char*)a + i);
    return memblock;
}

size_t   heap_get_largest_used_block_size(void)
{
    if(heap_validate())
        return 0;
    struct memory_chunk_t* a = memory_manager.first_memory_chunk;
    unsigned long long max = 0;
    while(a != NULL)
    {
        if(a->free == 0)
        {
            if (a->size > max)
                max = a->size;
        }
        a = a->next;
    }
    return max;
}

enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer == NULL)
        return pointer_null;
    int x = heap_validate();
    if(x == 1 || x == 3)
        return pointer_heap_corrupted;
    if(x == 2)
        return pointer_unallocated;
    unsigned char* a;
    a = (unsigned char*)memory_manager.first_memory_chunk;
    struct memory_chunk_t* b;
    b = memory_manager.first_memory_chunk;
    while(b != NULL)
    {
        for(int i = 0; i < (int)sizeof(struct memory_chunk_t); i++)
        {
            if(a == pointer)
                return pointer_control_block;
            a++;
        }
        if(b->free == 0)
        {
            for (int i = 0; i < PLOTEK; i++) {
                if (a == pointer)
                    return pointer_inside_fences;
                a++;
            }
            if (a == pointer)
                return pointer_valid;
            for (int i = 0; i < (int) b->size; i++) {
                if (a == pointer)
                    return pointer_inside_data_block;
                a++;
            }
            for (int i = 0; i < PLOTEK; i++) {
                if (a == pointer)
                    return pointer_inside_fences;
                a++;
            }
        }
        b = b->next;
        a = (unsigned char*)b;
    }

    return pointer_unallocated;
}

void* heap_malloc_aligned(size_t count)
{
    if(heap_validate())
        return NULL;
    if(count == 0)
        return NULL;
    void *p;
    if(memory_manager.first_memory_chunk == NULL)
    {
        if(count + sizeof(struct memory_chunk_t) + PLOTEK * 2 + MEM_SIZE> memory_manager.memory_size)
        {
            p = custom_sbrk(MEM_SIZE * ((count + sizeof(struct memory_chunk_t) + PLOTEK * 2 + MEM_SIZE - memory_manager.memory_size) / MEM_SIZE + 1));
            if((char *)p == (char *)-1)
                return NULL;
            memory_manager.memory_size += MEM_SIZE * ((count + sizeof(struct memory_chunk_t) + PLOTEK * 2 + MEM_SIZE - memory_manager.memory_size) / MEM_SIZE + 1);
        }
        struct memory_chunk_t *a = memory_manager.memory_start;
        a->prev = NULL;
        a->next = NULL;
        a->free = 1;
        a->size = MEM_SIZE - sizeof(struct memory_chunk_t) * 2 - PLOTEK;
        a->checksum = 0;
        memory_manager.first_memory_chunk = a;
        struct memory_chunk_t *b = (struct memory_chunk_t*)((unsigned char*)a + sizeof(struct memory_chunk_t) + a->size);
        b->prev = a;
        b->next = NULL;
        b->free = 0;
        b->size = count;
        a->next = b;
        for(unsigned int i = 0; i < PLOTEK; i++)
            *((unsigned char*)(b + 1) + i) = '#';
        for(unsigned int i = 0; i < PLOTEK; i++)
            *((unsigned char*)(b + 1) + i + count + PLOTEK) = '#';
        a->checksum = 0;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            a->checksum += *((unsigned char*)a + i);
        b->checksum = 0;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            b->checksum += *((unsigned char*)b + i);
        return (unsigned char*)(b + 1) + PLOTEK;
    }
    struct memory_chunk_t *a = memory_manager.first_memory_chunk;
    while(a != NULL)
    {
        if(a->free == 1)
        {
            if(a->size >= count + PLOTEK * 2 && ((unsigned char*)a - (unsigned char*)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + PLOTEK) % MEM_SIZE == 0)
            {
                a->size = count;
                a->free = 0;
                for(unsigned int i = 0; i < PLOTEK; i++)
                    *((unsigned char*)(a + 1) + i) = '#';
                for(unsigned int i = 0; i < PLOTEK; i++)
                    *((unsigned char*)(a + 1) + i + count + PLOTEK) = '#';
                a->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->checksum += *((unsigned char*)a + i);
                return (unsigned char*)(a + 1) + PLOTEK;
            }
        }
        if(a->next == NULL)
        {

            struct memory_chunk_t *b;
            b = (struct memory_chunk_t*)((unsigned char*)a + sizeof(struct memory_chunk_t) + a->size + PLOTEK * 2);
            unsigned char* c = (unsigned char*)b;
            long long needed =  (long long)(count + sizeof(struct memory_chunk_t) + PLOTEK * 2) - (long long)(memory_manager.memory_size - ((unsigned char*)a + sizeof(struct memory_chunk_t) + a->size + PLOTEK * 2 - (unsigned char*)memory_manager.first_memory_chunk));
            if(needed > 0)
            {
                int flag1 = 1;
                if((c - (unsigned char*)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + PLOTEK) % MEM_SIZE != 0)
                {
                    c += MEM_SIZE - ((c - (unsigned char*)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + PLOTEK) % MEM_SIZE);
                    flag1 = 2;
                }
                p = custom_sbrk(MEM_SIZE * (needed / MEM_SIZE + flag1));
                if((char *)p == (char *)-1)
                    return NULL;
                memory_manager.memory_size += MEM_SIZE * ((count + sizeof(struct memory_chunk_t) + PLOTEK * 2 - (memory_manager.memory_size - ((unsigned char*)a + sizeof(struct memory_chunk_t) + a->size + PLOTEK * 2 - (unsigned char*)memory_manager.first_memory_chunk))) / MEM_SIZE + flag1);
            }
            else if((c - (unsigned char*)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + PLOTEK) % MEM_SIZE != 0)
            {
                c += MEM_SIZE - ((c - (unsigned char*)memory_manager.first_memory_chunk + sizeof(struct memory_chunk_t) + PLOTEK) % MEM_SIZE);
                if(c + sizeof(struct memory_chunk_t) + PLOTEK * 2 + count - (unsigned char*)memory_manager.first_memory_chunk > (long)memory_manager.memory_size)
                {
                    p = custom_sbrk(MEM_SIZE);
                    if((char *)p == (char *)-1)
                        return NULL;
                    memory_manager.memory_size += MEM_SIZE;
                }
            }
            b = (struct memory_chunk_t*)c;
            b->prev = a;
            b->next = NULL;
            b->free = 0;
            b->size = count;
            a->next = b;
            a->checksum = 0;
            b->checksum = 0;
            for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                b->checksum += *((unsigned char*)b + i);
            for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                a->checksum += *((unsigned char*)a + i);
            for(unsigned int i = 0; i < PLOTEK; i++)
                *((unsigned char*)(b + 1) + i) = '#';
            for(unsigned int i = 0; i < PLOTEK; i++)
                *((unsigned char*)(b + 1) + i + count + PLOTEK) = '#';
            return (unsigned char*)(b + 1) + PLOTEK;
        }
        a = a->next;
    }
    return NULL;//wont reach this point
}

void* heap_calloc_aligned(size_t number, size_t size)
{
    void* p = heap_malloc_aligned(number * size);
    if(p == NULL)
        return NULL;
    for(int i = 0; i < (int)(size * number); i++)
        *((unsigned char*)p + i) = 0;
    return p;
}

void* heap_realloc_aligned(void* memblock, size_t size)
{
    if(heap_validate())
        return NULL;
    if(memblock == NULL)
        return heap_malloc_aligned(size);
    if(get_pointer_type(memblock) != pointer_valid)
        return NULL;
    struct memory_chunk_t* a = (struct memory_chunk_t*)((unsigned char*)memblock - PLOTEK - sizeof(struct memory_chunk_t));
    if(size == 0)
    {
        heap_free(memblock);
        return NULL;
    }
    if(size < a->size)
    {
        a->size = size;
        for(unsigned int i = 0; i < PLOTEK; i++)
            *((unsigned char*)(a + 1) + PLOTEK + size + i) = '#';
        a->checksum = 0;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            a->checksum += *((unsigned char*)a + i);
        return memblock;
    }
    if(size == a->size)
        return memblock;

    if(a->next != NULL)
    {
        if((unsigned char*)a->next - (unsigned char*)a - sizeof(struct memory_chunk_t) - PLOTEK * 2 > size)
        {
            a->size = size;
            for(unsigned int i = 0; i < PLOTEK; i++)
                *((unsigned char*)memblock + size + i)= '#';
            a->checksum = 0;
            for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                a->checksum += *((unsigned char*)a + i);
            return memblock;
        }
        if(a->next->free == 1)
        {
            if((unsigned char*)a->next->next - (unsigned char*)a - sizeof(struct memory_chunk_t) - PLOTEK * 2 > size)
            {
                a->size = size;
                a->next = a->next->next;
                a->next->prev = a;
                for(unsigned int i = 0; i < PLOTEK; i++)
                    *((unsigned char*)memblock + size + i)= '#';
                a->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->checksum += *((unsigned char*)a + i);
                a->next->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->next->checksum += *((unsigned char*)(a->next) + i);
                return memblock;
            }
        }
        void *p = heap_malloc_aligned(size);
        if(p == NULL)
            return NULL;
        for(unsigned int i = 0; i < size; i++)
            *((unsigned char*)p + i) = *((unsigned char*)memblock + i);
        heap_free(memblock);
        return p;

    }
    void *p;
    if(memory_manager.memory_size - ((unsigned char*)a - (unsigned char*)memory_manager.first_memory_chunk) < size + sizeof(struct memory_chunk_t) + PLOTEK * 2)
    {
        p = custom_sbrk(MEM_SIZE * ((size + sizeof(struct memory_chunk_t) + PLOTEK * 2 - (memory_manager.memory_size - ((unsigned char*)a - (unsigned char*)memory_manager.first_memory_chunk))) / MEM_SIZE + 1));
        if((char *)p == (char *)-1)
            return NULL;
        memory_manager.memory_size += MEM_SIZE * ((size + sizeof(struct memory_chunk_t) + PLOTEK * 2 - (memory_manager.memory_size - ((unsigned char*)a - (unsigned char*)memory_manager.first_memory_chunk))) / MEM_SIZE + 1);
    }
    a->size = size;
    for(unsigned int i = 0; i < PLOTEK; i++)
        *((unsigned char*)memblock + size + i)= '#';
    a->checksum = 0;
    for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
        a->checksum += *((unsigned char*)a + i);

    return memblock;
}

int heap_validate(void)
{
    if(memory_manager.memory_start == NULL)
        return 2;
    if(memory_manager.first_memory_chunk == NULL)
        return 0;
    struct memory_chunk_t *a = memory_manager.first_memory_chunk;
    //check if control structure is correct
    struct memory_chunk_t *z = memory_manager.first_memory_chunk;
    uint32_t sum;
    while(z != NULL)
    {
        sum = 0;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            sum += *((unsigned char*)z + i);
        if(sum != *(uint32_t*)z)
            return 3;
        z = z->next;
    }
    //end check
    while(a != NULL)
    {
        if(a->free == 0)
        {
            for(unsigned int i = 0; i < PLOTEK; i++)
            {
                if(*((unsigned char*)(a+ 1) + i) != '#')
                    return 1;
            }
            for(unsigned int i = 0; i < PLOTEK; i++)
            {
                if(*((unsigned char*)(a + 1) + i + a->size + PLOTEK) != '#')
                    return 1;
            }
        }
        a = a->next;
    }

    return 0;
}

void *heap_malloc(size_t size)
{
    if(heap_validate())
        return NULL;
    if(size == 0)
        return NULL;
    void *p;

    if(memory_manager.first_memory_chunk == NULL)
    {
        if(size + sizeof(struct memory_chunk_t) + PLOTEK * 2 > memory_manager.memory_size)
        {
            p = custom_sbrk(MEM_SIZE * ((size + sizeof(struct memory_chunk_t) + PLOTEK * 2 - memory_manager.memory_size) / MEM_SIZE + 1));
            if((char *)p == (char *)-1)
                return NULL;
            memory_manager.memory_size += MEM_SIZE * ((size + sizeof(struct memory_chunk_t) + PLOTEK * 2 - memory_manager.memory_size) / MEM_SIZE + 1);
        }
        struct memory_chunk_t *a = memory_manager.memory_start;
        a->prev = NULL;
        a->next = NULL;
        a->free = 0;
        a->size = size;
        a->checksum = 0;
        memory_manager.first_memory_chunk = a;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            a->checksum += *((unsigned char*)a + i);
        for(unsigned int i = 0; i < PLOTEK; i++)
            *((unsigned char*)(a + 1) + i) = '#';
        for(unsigned int i = 0; i < PLOTEK; i++)
            *((unsigned char*)(a + 1) + i + size + PLOTEK) = '#';
        return (unsigned char*)(a + 1) + PLOTEK;
    }
    struct memory_chunk_t *a = memory_manager.first_memory_chunk;
    while(a != NULL)
    {
        if(a->free == 1)
        {
            if(a->size >= size + PLOTEK * 2)
            {
                a->size = size;
                a->free = 0;
                for(unsigned int i = 0; i < PLOTEK; i++)
                    *((unsigned char*)(a + 1) + i) = '#';
                for(unsigned int i = 0; i < PLOTEK; i++)
                    *((unsigned char*)(a + 1) + i + size + PLOTEK) = '#';
                a->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->checksum += *((unsigned char*)a + i);
                return (unsigned char*)(a + 1) + PLOTEK;
            }
        }
        if(a->next == NULL)
        {
            long long needed = (int)(size + sizeof(struct memory_chunk_t) + PLOTEK * 2) - (int)(memory_manager.memory_size - ((unsigned char*)a + sizeof(struct memory_chunk_t) + a->size + PLOTEK * 2 - (unsigned char*)memory_manager.first_memory_chunk));
            if(needed > 0)
            {
                if(needed % MEM_SIZE == 0)
                {
                    p = custom_sbrk(needed);
                    if((char *)p == (char *)-1)
                        return NULL;
                    memory_manager.memory_size += needed;
                }
                else
                {
                    p = custom_sbrk(MEM_SIZE * (needed / MEM_SIZE + 1));
                    if((char *)p == (char *)-1)
                        return NULL;
                    memory_manager.memory_size += MEM_SIZE * (needed / MEM_SIZE + 1);
                }
            }
            struct memory_chunk_t *b;
            b = (struct memory_chunk_t*)((unsigned char*)a + sizeof(struct memory_chunk_t) + a->size + PLOTEK * 2);
            b->prev = a;
            b->next = NULL;
            b->free = 0;
            b->size = size;
            b->checksum = 0;
            a->checksum = 0;
            a->next = b;
            for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                a->checksum += *((unsigned char*)a + i);
            for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                b->checksum += *((unsigned char*)b+ i);
            for(unsigned int i = 0; i < PLOTEK; i++)
                *((unsigned char*)(b + 1) + i) = '#';
            for(unsigned int i = 0; i < PLOTEK; i++)
                *((unsigned char*)(b + 1) + i + size + PLOTEK) = '#';
            return (unsigned char*)(b + 1) + PLOTEK;
        }
        a = a->next;
    }
    return NULL; //wont reach this point
}

void heap_free(void *address) {
    if(get_pointer_type(address) != pointer_valid)
        return;
    struct memory_chunk_t *a = memory_manager.first_memory_chunk;
    if ((unsigned char *) (a + 1) + PLOTEK == address)
    {
        if (a->next == NULL)
        {
            memory_manager.first_memory_chunk = NULL;
            return;
        }
        a->free = 1;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            a->checksum += *((unsigned char*)a + i);
        a = a->next;
        if (a->free == 1)
            a = a->next;
        memory_manager.first_memory_chunk->size = (unsigned char *) a - sizeof(struct memory_chunk_t) -
                                                  (unsigned char *) memory_manager.first_memory_chunk;
        memory_manager.first_memory_chunk->next = a;
        a->prev = memory_manager.first_memory_chunk;
        a->checksum = 0;
        memory_manager.first_memory_chunk->checksum = 0;
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            a->checksum += *((unsigned char*)a + i);
        for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
            memory_manager.first_memory_chunk->checksum += *((unsigned char*)memory_manager.first_memory_chunk + i);
        return;
    }
    a = a->next;
    while (a != NULL)
    {
        if ((unsigned char *) (a + 1) + PLOTEK == address)
        {
            if (a->next == NULL && a->prev->free == 1 && a->prev == memory_manager.first_memory_chunk)
            {
                memory_manager.first_memory_chunk = NULL;
            }
            else if (a->prev->free == 1 && a->next == NULL)
            {
                a->prev->prev->next = NULL;
                a->prev->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->prev->checksum += *((unsigned char*)(a->prev) + i);
                a->prev->prev->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->prev->prev->checksum += *((unsigned char*)(a->prev->prev) + i);
            }
            else if (a->prev->free == 1 && a->next->free == 0)
            {
                a->prev->next = a->next;
                a->next->prev = a->prev;
                a->prev->size = (unsigned char *) a->next - sizeof(struct memory_chunk_t) - (unsigned char *) a->prev;
                a->prev->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->prev->checksum += *((unsigned char*)(a->prev) + i);
                a->next->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->next->checksum += *((unsigned char*)(a->next) + i);
            }
            else if (a->prev->free == 1)
            {
                a->prev->next = a->next->next;
                a->next->next->prev = a->prev;
                a->prev->size =
                        (unsigned char *) a->next->next - sizeof(struct memory_chunk_t) - (unsigned char *) a->prev;
                a->prev->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->prev->checksum += *((unsigned char*)(a->prev) + i);
                a->next->next->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->next->next->checksum += *((unsigned char*)(a->next->next) + i);
            }
            else if (a->prev->free == 0 && a->next == NULL)
            {
                a->prev->next = NULL;
                a->prev->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->prev->checksum += *((unsigned char*)(a->prev) + i);
            }
            else if (a->prev->free == 0 && a->next->free == 0)
            {
                a->free = 1;
                a->size = (unsigned char *) a->next - sizeof(struct memory_chunk_t) - (unsigned char *) a;
                a->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->checksum += *((unsigned char*)a + i);
            }
            else if (a->prev->free == 0 && a->next->free == 1)
            {
                a->free = 1;
                a->next = a->next->next;
                a->next->prev = a;
                a->size = (unsigned char *) a->next - sizeof(struct memory_chunk_t) - (unsigned char *) a;
                a->checksum= 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->checksum += *((unsigned char*)a + i);
                a->next->checksum = 0;
                for(unsigned int i = sizeof(int); i < sizeof(struct memory_chunk_t); i++)
                    a->next->checksum += *((unsigned char*)(a->next) + i);
            }

            return;
        }
        a = a->next;
    }
}

