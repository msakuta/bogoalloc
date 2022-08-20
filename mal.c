#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HEAPSIZE (1024 * 1)
#define ALIGNMENT 8
#define CHUNK_NUM 1024

static unsigned char heap[HEAPSIZE] = {0};

typedef struct Chunk {
    unsigned char *head;
    size_t sz;
    size_t id;
    struct Chunk *next;
} Chunk;

static Chunk chunk_list[CHUNK_NUM];

static Chunk *alloc_chunks = NULL;
static Chunk *free_chunks = NULL;
static Chunk *unused_chunks = NULL;
static size_t id_gen = 1;

void init_bogoalloc(){
    for(size_t i = 0; i < CHUNK_NUM-1; i++){
        chunk_list[i].next = &chunk_list[i + 1];
    }

    unused_chunks = &chunk_list[0];

    Chunk *free = unused_chunks;
    unused_chunks = free->next;
    free->head = heap;
    free->sz = HEAPSIZE;
    free->id = 0;
    free->next = NULL;
    free_chunks = free;

    // printf("unused %p free %p\n", )
}

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

                Chunk *next_alloc = alloc_chunks;
                alloc_chunks = unused_chunks;
                unused_chunks = unused_chunks->next;
                alloc_chunks->head = ret;
                alloc_chunks->sz = size;
                alloc_chunks->id = id_gen++;
                alloc_chunks->next = next_alloc;
            }
            break;
        }
        free_iter = &(*free_iter)->next;
    }

    return ret;
}

#define BOGOFREE_DEBUG

void bogofree(void *p){
    Chunk **alloc_iter = &alloc_chunks;
    while(*alloc_iter){
        if((*alloc_iter)->head == p){
            Chunk *freeing_chunk = *alloc_iter;
            *alloc_iter = freeing_chunk->next;

            freeing_chunk->next = free_chunks;
            free_chunks = freeing_chunk;
            size_t *sz = &freeing_chunk->sz;
            *sz = (*sz + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT; // Round up for free chunks

            BOGOFREE_DEBUG("[%lu] Start searching neighbor free chunks (%lu, %lu)\n", freeing_chunk->id, freeing_chunk->head - heap, freeing_chunk->head + freeing_chunk->sz - heap);

            Chunk **neighbor_iter = &free_chunks;
            int merged = 0;
            while(*neighbor_iter){
                // size_t next_sz = ((*neighbor_iter)->sz + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;
                BOGOFREE_DEBUG("   Examining chunk %lu (%lu, %lu)\n", (*neighbor_iter)->id, (*neighbor_iter)->head - heap, (*neighbor_iter)->head + (*neighbor_iter)->sz - heap);
                if(*neighbor_iter != freeing_chunk && (*neighbor_iter)->head == freeing_chunk->head + freeing_chunk->sz){
                    BOGOFREE_DEBUG("[%lu] Merging %lu next (%ld, %ld)\n", freeing_chunk->id, (*neighbor_iter)->id, (intptr_t)((*neighbor_iter)->head - heap), (intptr_t)(freeing_chunk->head - heap));
                    freeing_chunk->sz += (*neighbor_iter)->sz;
                    Chunk *unused_next = unused_chunks;
                    unused_chunks = *neighbor_iter;
                    *neighbor_iter = (*neighbor_iter)->next;
                    unused_chunks->next = unused_next;
                    merged = 1;
                    break;
                }
                neighbor_iter = &(*neighbor_iter)->next;
            }
            neighbor_iter = &free_chunks;
            while(*neighbor_iter){
                if(*neighbor_iter != freeing_chunk && (*neighbor_iter)->head + (*neighbor_iter)->sz == freeing_chunk->head){
                    BOGOFREE_DEBUG("[%lu] Merging %lu prev (%ld, %ld)\n", freeing_chunk->id, (*neighbor_iter)->id, (intptr_t)((*neighbor_iter)->head - heap), (intptr_t)(freeing_chunk->head - heap));
                    (*neighbor_iter)->sz += freeing_chunk->sz;
                    Chunk *unused_next = unused_chunks;
                    unused_chunks = freeing_chunk;
                    free_chunks = freeing_chunk->next;
                    unused_chunks->next = unused_next;
                    merged = 1;
                    break;
                }
                neighbor_iter = &(*neighbor_iter)->next;
            }
            if(!merged)
                BOGOFREE_DEBUG("[%lu] Moving chunk to free list\n", freeing_chunk->id);
            return;
        }
        alloc_iter = &(*alloc_iter)->next;
    }
    BOGOFREE_DEBUG("WARNING! couldn't find ptr in bogofree %p\n", p);
}

// There is no trivial way to dump all lists without iterating each linked list
// void dump_chunk_list(){
//     for(size_t i = 0; i < used_chunks; i++){
//         const Chunk *chunk = &chunk_list[i];
//         printf("[%lu] head: %p, sz: %lu, id: %lu, next: %p\n", i, chunk->head, chunk->sz, chunk->id, chunk->next);
//     }
// }

void list_heap(const Chunk* chunk, const char* name){
    printf("<--------------- %s ----------->\n", name);
    while(chunk){
        printf("[%lu] head: %ld, sz: %lu\n", chunk->id, chunk->head - heap, chunk->sz);
        chunk = chunk->next;
    }
    printf("</-------------- %s ----------->\n", name);
}

size_t count_chunks(const Chunk* chunk){
    size_t ret = 0;
    while(chunk){
        ret++;
        chunk = chunk->next;
    }
    return ret;
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
                c = chunk->head == p ? '[' :
                    chunk->head + chunk->sz - 1 == p ? ']' :
                    chunk->head + 1 == p ? (chunk->id / 10 % 10) + '0' :
                    chunk->head + 2 == p ? (chunk->id % 10) + '0' :
                    counter % 2 ? '*' : '+';
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
            else if (chunk->head + 2 == p){
                c = '0' + (int)(chunk->id) % 10;
                break;
            }
            else if (chunk->head + 1 == p){
                c = '0' + (int)(chunk->id) / 10 % 10;
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

    init_bogoalloc();
    printf("Unused chunks: %lu\n", count_chunks(unused_chunks));

    double *ptrs[15] = {NULL};

    for(int i = 0; i < 10; i++){
        double *all = bogoalloc(8 + i);
        *all = i;
        // printf("heap head = %p, all = %p\n", heap, all);
        ptrs[i] = all;
    }

    dump_heap();

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

    for(int i = 0; i < 15; i++){
        if(ptrs[i]){
            printf("ptr[%d]: %p = %g\n", i, ptrs[i], *ptrs[i]);
            // dump_heap();
            bogofree(ptrs[i]);
        }
        else
            printf("ptr[%d]: NULL\n", i);
    }

    bogoalloc(128);

    dump_heap();

    list_heap(alloc_chunks, "Allocated");
    list_heap(free_chunks, "Freed");
    printf("Unused chunks: %lu\n", count_chunks(unused_chunks));

    // dump_chunk_list();

    printf("sizeof size_t: %lu\n", sizeof(size_t));
    printf("sizeof Chunk: %lu\n", sizeof(Chunk));
}