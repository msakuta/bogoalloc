#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEAPSIZE (1024 * 2)
#define ALIGNMENT 8
#define CHUNK_NUM 1024

static unsigned char heap[HEAPSIZE] = {0};

typedef struct Chunk {
    unsigned char *head;
    size_t sz;
    struct Chunk *next;
} Chunk;

static Chunk chunk_list[CHUNK_NUM] = {
    {
        .head = heap,
        .sz = HEAPSIZE,
        .next = NULL
    },
};

static Chunk *alloc_chunks = NULL;
static Chunk *free_chunks = &chunk_list[0];
static size_t used_chunks = 1;

void *bogoalloc(size_t size){
    size_t rounded_size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;

    void *ret = NULL;
    Chunk **free_iter = &free_chunks;
    while(*free_iter){
        if(rounded_size <= (*free_iter)->sz){
            ret = (*free_iter)->head;
            if(rounded_size == (*free_iter)->sz){
                Chunk *new_chunk = *free_iter;
                *free_iter = (*free_iter)->next;
                new_chunk->next = alloc_chunks;
                alloc_chunks = new_chunk;
            }
            else{
                (*free_iter)->head += rounded_size;
                (*free_iter)->sz -= rounded_size;

                Chunk chunk = {
                    .head = ret,
                    .sz = size,
                    .next = alloc_chunks
                };

                alloc_chunks = &chunk_list[used_chunks++];
                *alloc_chunks = chunk;
            }
            break;
        }
        free_iter = &(*free_iter)->next;
    }

    return ret;
}

void bogofree(void *p){
    Chunk **alloc_iter = &alloc_chunks;
    while(*alloc_iter){
        if((*alloc_iter)->head == p){
            Chunk *deleting_chunk = *alloc_iter;
            *alloc_iter = deleting_chunk->next;

            Chunk *chunk = &chunk_list[used_chunks++];
            *chunk = *deleting_chunk;
            chunk->next = free_chunks;
            free_chunks = chunk;
            size_t *sz = &chunk->sz;
            *sz = (*sz + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT; // Round up for free chunks
            return;
        }
        alloc_iter = &(*alloc_iter)->next;
    }
    printf("WARNING! couldn't find ptr in bogofree %p\n", p);
}

void list_heap(const Chunk* chunk){
    printf("----------------\n");
    while(chunk){
        printf("[%lu] head: %ld, sz: %lu\n", (size_t)(chunk - chunk_list), chunk->head - heap, chunk->sz);
        chunk = chunk->next;
    }
}

void dump_heap(){
    for(size_t i = 0; i < HEAPSIZE; i++){
        unsigned char* p = &heap[i];

        if (i % 128 == 0)
            printf("%06lu: ", p - heap);

        char c = '?';
        const Chunk *chunk = alloc_chunks;
        size_t counter = 0;
        while(chunk){
            if (chunk->head <= p && p < chunk->head + chunk->sz){
                c = chunk->head == p ? '[' : chunk->head + chunk->sz - 1 == p ? ']' : counter % 2 ? '*' : '+';
                break;
            }
            chunk = chunk->next;
            counter++;
        }

        chunk = free_chunks;
        while(chunk){
            if (chunk->head == p){
                c = '<';
                break;
            }
            else if(chunk->head + chunk->sz - 1 == p){
                c = '>';
                break;
            }
            else if(chunk->head <= p && p < chunk->head + chunk->sz){
                c = '-';
                break;
            }
            chunk = chunk->next;
        }

        putchar(c);

        if ((i + 1) % 32 == 0)
            putchar((i + 1) % 128 == 0 ? '\n' : ' ');
    }
}

int main(){
    printf("heap head = %p\n", heap);

    double *ptrs[15] = {NULL};

    for(int i = 0; i < 10; i++){
        double *all = bogoalloc(8 + i);
        *all = i;
        // printf("heap head = %p, all = %p\n", heap, all);
        ptrs[i] = all;
    }

    bogofree(ptrs[0]);
    ptrs[0] = NULL;
    for(int i = 2; i < 10; i++){
        if(i % 2 == 0){
            bogofree(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    for(int i = 0; i < 5; i++){
        double *p = bogoalloc(8 + i + 5);
        *p = i;
        ptrs[i + 10] = p;
    }

    // for(int i = 0; i < 15; i++){
    //     if(ptrs[i]){
    //         printf("ptr[%d]: %p = %g\n", i, ptrs[i], *ptrs[i]);
    //         bogofree(ptrs[i]);
    //     }
    //     else
    //         printf("ptr[%d]: NULL\n", i);
    // }

    bogoalloc(128);

    dump_heap();

    list_heap(alloc_chunks);
    list_heap(free_chunks);


    printf("sizeof size_t: %lu\n", sizeof(size_t));
    printf("sizeof Chunk: %lu\n", sizeof(Chunk));
}