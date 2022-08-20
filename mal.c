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
} Chunk;

typedef struct ChunkList {
    Chunk chunks[CHUNK_NUM];
    size_t size;
} ChunkList;

static ChunkList alloc_chunks = {0};
static ChunkList free_chunks = {
    .chunks = {{
        .head = heap,
        .sz = HEAPSIZE
    }},
    .size = 1
};

void *alloc(size_t size){
    size_t rounded_size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;

    void *ret = NULL;
    for(size_t i = 0; i < free_chunks.size; i++){
        if(rounded_size <= free_chunks.chunks[i].sz){
            ret = free_chunks.chunks[i].head;
            if(rounded_size == free_chunks.chunks[i].sz){
                memmove(&free_chunks.chunks[i], &free_chunks.chunks[i + 1], (free_chunks.size - 1) * sizeof(Chunk));
                free_chunks.size--;
            }
            else{
                free_chunks.chunks[i].head += rounded_size;
                free_chunks.chunks[i].sz -= rounded_size;
            }
            break;
        }
    }
    if(ret == NULL){
        return NULL;
    }

    Chunk chunk = {
        .head = ret,
        .sz = size,
    };

    alloc_chunks.chunks[alloc_chunks.size++] = chunk;

    return ret;
}

void freec(void *p){
    for(size_t i = 0; i < alloc_chunks.size; i++){
        if(alloc_chunks.chunks[i].head == p){
            Chunk *chunk = &free_chunks.chunks[free_chunks.size];
            *chunk = alloc_chunks.chunks[i];
            size_t *sz = &chunk->sz;
            *sz = (*sz + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT; // Round up for free chunks
            printf("freec(%ld): alloc_chunks.size = %lu, moving %lu\n", (unsigned char*)p - heap, alloc_chunks.size, alloc_chunks.size - i - 1);
            free_chunks.size++;
            for(size_t j = 0; j < free_chunks.size - 1; j++){
                Chunk *chunk_j = &free_chunks.chunks[j];
                if(chunk->head + chunk->sz == chunk_j->head){
                    printf("Merging back %lu\n", j);
                    chunk->sz += chunk_j->sz;
                    memmove(&free_chunks.chunks[j], &free_chunks.chunks[j + 1], (free_chunks.size - 1) * sizeof(Chunk));
                    free_chunks.size--;
                    break;
                }
                else if(chunk_j->head + chunk_j->sz == chunk->head){
                    printf("Merging front %lu\n", j);
                    chunk_j->sz += chunk->sz;
                    free_chunks.size--;
                }
            }
            alloc_chunks.size--;
            memmove(&alloc_chunks.chunks[i], &alloc_chunks.chunks[i + 1], (alloc_chunks.size - i) * sizeof(Chunk));
            return;
        }
    }
    printf("WARNING! couldn't find ptr in freec\n");
}

void list_heap(const ChunkList* chunk_list){
    printf("----------------\n");
    for(size_t i = 0; i < chunk_list->size; i++){
        printf("[%lu] head: %ld, sz: %lu\n", i, chunk_list->chunks[i].head - heap, chunk_list->chunks[i].sz);
    }
}

void dump_heap(){
    const Chunk* chunks = alloc_chunks.chunks;
    for(size_t i = 0; i < HEAPSIZE; i++){
        unsigned char* p = &heap[i];

        if (i % 128 == 0)
            printf("%06lu: ", p - heap);

        int found = -1;
        char c = '?';
        for(size_t j = 0; j < alloc_chunks.size; j++){
            if (chunks[j].head <= p && p < chunks[j].head + chunks[j].sz){
                found = j;
                c = chunks[j].head == p ? '[' : chunks[j].head + chunks[j].sz - 1 == p ? ']' : j % 2 ? '*' : '+';
                break;
            }
        }

        for(size_t j = 0; j < free_chunks.size; j++){
            const Chunk* chunk = &free_chunks.chunks[j];
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
        double *all = alloc(8 + i);
        *all = i;
        // printf("heap head = %p, all = %p\n", heap, all);
        ptrs[i] = all;
    }

    freec(ptrs[0]);
    ptrs[0] = NULL;
    // for(int i = 0; i < 10; i++){
    //     if(i % 2 == 0){
    //         freec(ptrs[i]);
    //         ptrs[i] = NULL;
    //     }
    // }

    for(int i = 0; i < 5; i++){
        double *p = alloc(8 + i + 5);
        *p = i;
        ptrs[i + 10] = p;
    }

    for(int i = 0; i < 15; i++){
        if(ptrs[i]){
            printf("ptr[%d]: %p = %g\n", i, ptrs[i], *ptrs[i]);
            freec(ptrs[i]);
        }
        else
            printf("ptr[%d]: NULL\n", i);
    }

    alloc(128);

    dump_heap();

    list_heap(&alloc_chunks);
    list_heap(&free_chunks);


    printf("sizeof size_t: %lu\n", sizeof(size_t));
    printf("sizeof Chunk: %lu\n", sizeof(Chunk));
}