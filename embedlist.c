// Doubly-linked list in embedded heap memory
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#define HEAPSIZE (256 * 1)
#define ALIGNMENT 8
#define CHUNK_NUM 256

typedef struct Node {
    size_t sz;
    size_t id;
    struct Node *prev, *next;
} Node;

static unsigned char heap[HEAPSIZE] = {0};
static size_t id_gen = 1;

static Node *active_list = NULL;
static Node *free_list = NULL;

static Node *find_node(void *p);

void init_bogoalloc() {
    Node *root = (Node*)heap;
    root->sz = HEAPSIZE - sizeof(Node);
    root->id = id_gen++;
    root->prev = NULL;
    root->next = NULL;
    free_list = root;
}

void *bogoalloc(size_t size) {
    size_t rounded_size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;

    Node *new_node = free_list;

    size_t free_size = new_node->sz;
    Node *free_next = free_list->next;

    new_node->sz = rounded_size;
    new_node->next = active_list;
    if (active_list) active_list->prev = new_node;
    new_node->prev = NULL;
    if (active_list == new_node) {
        fprintf(stderr, active_list == new_node);
        exit(1);
    }
    active_list = new_node;

    printf("rounded_size + sizeof(Node): %lu free_size: %lu\n", rounded_size + sizeof(Node), free_size);
    if (rounded_size + sizeof(Node) < free_size) {
        free_size -= (sizeof(Node) + rounded_size);
        Node *new_free = (Node*)((unsigned char*)new_node + sizeof(Node) + rounded_size);
        new_free->sz = free_size;
        new_free->id = id_gen++;
        new_free->next = NULL;
        new_free->prev = NULL;

        free_list = new_free;
    }
    else {
        free_list = free_next;
        if (free_list) free_list->prev = NULL;
    }

    return (unsigned char*)new_node + sizeof(Node);
}

void bogofree(void *p) {
    Node *node = find_node(p);
    if (!node) {
        fprintf(stderr, "Free unknown heap!");
        return;
    }
    if (node->next) node->next->prev = node->prev;
    if (node->prev) node->prev->next = node->next;
    if (active_list == node) active_list = node->next;
    node->prev = NULL;
    node->next = free_list;
    if (free_list) free_list->prev = node;
    free_list = node;
}

static Node *find_node(void *p) {
    Node *start = (Node*)active_list;
    while (start) {
        if ((unsigned char*)start + sizeof(Node) <= (unsigned char*)p
            && (unsigned char*)p < (unsigned char*)start + sizeof(Node) + start->sz)
        {
            return start;
        }
        start = start->next;
    }
    return NULL;
}

static Node *find_free_node(void *p) {
    Node *start = free_list;
    while (start) {
        if ((unsigned char*)start + sizeof(Node) <= (unsigned char*)p
            && (unsigned char*)p < (unsigned char*)start + sizeof(Node) + start->sz)
        {
            return start;
        }
        start = start->next;
    }
    return NULL;
}

static size_t rel_ptr(void *p) {
    return !p ? UINT_MAX : (size_t)((unsigned char*)p - heap);
}

void list_nodes() {
    Node *node = active_list;
    while (node) {
        printf("Active Node[%lu] p: %0lx sz: %lu prev: %0lx next: %0lx\n",
            node->id, rel_ptr(node), node->sz, rel_ptr(node->prev), rel_ptr(node->next));
        node = node->next;
    }

    node = free_list;
    while (node) {
        printf("Free Node[%lu] p: %0lx sz: %lu prev: %0lx next: %0lx\n",
            node->id, rel_ptr(node), node->sz, rel_ptr(node->prev), rel_ptr(node->next));
        node = node->next;
    }
}

void dump_heap(){
    for(size_t i = 0; i < HEAPSIZE; i++){
        unsigned char* p = &heap[i];

        if (i % 64 == 0)
            printf("%06lx: ", p - heap);

        char c = '?';
        const Node *node = find_node(p);
        if (node) {
            c = (unsigned char*)node == p ? '<' :
                (unsigned char*)node + sizeof(Node) - 1 == p ? '>' :
                (unsigned char*)node + sizeof(Node) == p ? '[' :
                (unsigned char*)node + sizeof(Node) + node->sz - 1 == p ? ']' :
                (unsigned char*)node + sizeof(Node) + 1 == p ? (node->id / 10 % 10) + '0' :
                (unsigned char*)node + sizeof(Node) + 2 == p ? (node->id % 10) + '0' :
                node->id % 2 ? '*' : '+';
        }
        else if (node = find_free_node(p)) {
            c = (unsigned char*)node + sizeof(Node) == p ? '<' :
                (unsigned char*)node + sizeof(Node) + node->sz - 1 == p ? '>' :
                (unsigned char*)node + sizeof(Node) + 1 == p ? (node->id / 10 % 10) + '0' :
                (unsigned char*)node + sizeof(Node) + 2 == p ? (node->id % 10) + '0' :
                node->id % 2 ? '*' : '+';
        }

        putchar(c);

        if ((i + 1) % 16 == 0)
            putchar((i + 1) % 64 == 0 ? '\n' : ' ');
    }
}

int main(){
    printf("heap head = %p\n", heap);

    init_bogoalloc();
    // printf("Unused chunks: %lu\n", count_chunks(unused_chunks));

    list_nodes();
    dump_heap();

    void *ptr = bogoalloc(16);

    list_nodes();
    dump_heap();

    void *ptr2 = bogoalloc(32);

    list_nodes();
    dump_heap();

    bogofree(ptr2);

    list_nodes();
    dump_heap();

    void *ptr3 = bogoalloc(16);

    list_nodes();
    dump_heap();

    return 0;
}
