#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HEAPSIZE (256 * 1)
#define ALIGNMENT 8
#define NODE_NUM 256

static unsigned char heap[HEAPSIZE] = {0};

typedef struct Node {
    unsigned char *head;
    size_t sz;
    size_t id;
    struct Node *left, *right;
} Node;

static Node node_list[NODE_NUM];

static Node *alloc_chunks = NULL;
static Node *unused_chunks = NULL;
static size_t id_gen = 1;

static Node *find_best(Node **root, size_t size) {
    if (!root) return NULL;
    if (!*root) {
        // Fetch a node for the root node
        Node *new_node = unused_chunks;
        unused_chunks = new_node->left;

        // Init root node
        new_node->head = &heap[0];
        new_node->sz = size;
        new_node->id = id_gen++;
        new_node->left = NULL;
        new_node->right = NULL;

        *root = new_node;

        return new_node;
    }
    if ((*root)->left) {
        Node *left = find_best(&(*root)->left, size);
        if (left) return left;
    }
    if ((*root)->right) {
        Node *right = find_best(&(*root)->right, size);
        if (right) return right;
    }
    if ((*root)->left && (*root)->right) {
        size_t gap_size = (*root)->right->head - ((*root)->left->head + (*root)->left->sz);
        printf("Gap size: %lu at %p\n", gap_size, (*root)->head);
        if (size == gap_size) {
            // Fetch a node for parent node from free list
            Node *new_parent = unused_chunks;
            unused_chunks = new_parent->left;

            // Fetch a node for actual block from free list
            Node *new_node = unused_chunks;
            unused_chunks = new_node->left;

            // Init leaf node
            new_node->left = NULL;
            new_node->right = NULL;
            new_node->id = id_gen++;
            new_node->head = ((*root)->left->head + (*root)->left->sz);
            new_node->sz = size;

            // Graft the tree
            new_parent->left = (*root)->left;
            new_parent->right = new_node;
            (*root)->left = new_parent;

            return new_node;
        }
    }

    return NULL;
}

static Node *allocate_end(size_t size) {
    // If we don't find a best fit, we append at the end.
    printf("If we don't find a best fit, we append at the end.\n");

    // Fetch a node for parent node from free list
    Node *new_parent = unused_chunks;
    unused_chunks = new_parent->left;

    // Fetch a node for actual block from free list
    Node *new_node = unused_chunks;
    unused_chunks = new_node->left;

    // Init parent node
    new_parent->head = alloc_chunks->head;
    new_parent->sz = alloc_chunks->sz + size;
    new_parent->id = id_gen++;
    new_parent->left = alloc_chunks;
    new_parent->right = new_node;
    printf("%lu new_parent->head: %p, size: %lu\n", new_parent->id, new_parent->head, new_parent->sz);

    // Init leaf node
    new_node->head = alloc_chunks->head + alloc_chunks->sz;
    new_node->sz = size;
    new_node->id = id_gen++;
    new_node->left = NULL;
    new_node->right = NULL;
    printf("%lu new_node->head: %p, size: %lu\n", new_node->id, new_node->head, new_node->sz);

    alloc_chunks = new_parent;
    return new_node;
}

void init_bogoalloc(){
    for(size_t i = 0; i < NODE_NUM-1; i++){
        node_list[i].left = &node_list[i + 1];
        node_list[i].right = NULL;
    }

    unused_chunks = &node_list[0];

    Node *last = &node_list[NODE_NUM-1];
    last->head = NULL;
    last->sz = 0;
    last->id = 0;
    last->left = NULL;
    last->right = NULL;

    // printf("unused %p free %p\n", )
}

void *bogoalloc(size_t size){
    size_t rounded_size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;

    Node *node = find_best(&alloc_chunks, rounded_size);

    if (!node) {
        node = allocate_end(rounded_size);
    }

    node->sz = size;

    return node->head;
}

size_t count_nodes(const Node* node){
    size_t ret = 0;
    while(node){
        ret++;
        if (node->right)
            ret += count_nodes(node->right);
        node = node->left;
    }
    return ret;
}

static int free_recursive(void *p, Node **node) {
    if (!(*node)->left && !(*node)->right) {
        if ((*node)->head == p) {
            (*node)->left = unused_chunks;
            unused_chunks = *node;
            *node = NULL;
            return 1;
        }
    }
    if ((*node)->left) {
        if (free_recursive(p, &(*node)->left)) {
            if (!(*node)->left) {
                Node *to_be_freed = *node;
                *node = (*node)->right;

                // Return node to unused list
                to_be_freed->left = unused_chunks;
                unused_chunks = to_be_freed;
            }
            return 1;
        }
    }
    if ((*node)->right) {
        if (free_recursive(p, &(*node)->right)) {
            if (!(*node)->right) {
                Node *to_be_freed = *node;
                *node = (*node)->left;

                // Return node to unused list
                to_be_freed->left = unused_chunks;
                unused_chunks = to_be_freed;
            }
            return 1;
        }
    }
    return 0;
}

int bogofree(void *p){
    return free_recursive(p, &alloc_chunks);
}

static const Node *find_node(void *p, const Node *node) {
    if(!node) return NULL;
    if(node->left) {
        const Node *left = find_node(p, node->left);
        if(left) return left;
    }
    if(node->right) {
        const Node *right = find_node(p, node->right);
        if(right) return right;
    }
    if(!node->left && !node->right) {
        if((size_t)node->head <= (size_t)p && (size_t)p < (size_t)(node->head + node->sz))
            return node;
    }
    return NULL;
}

void dump_heap(){
    for(size_t i = 0; i < HEAPSIZE; i++){
        unsigned char* p = &heap[i];

        if (i % 64 == 0)
            printf("%06lx: ", p - heap);

        char c = '?';
        const Node *node = find_node(p, alloc_chunks);
        if (node) {
            c = node->head == p ? '[' :
                node->head + node->sz - 1 == p ? ']' :
                node->head + 1 == p ? (node->id / 10 % 10) + '0' :
                node->head + 2 == p ? (node->id % 10) + '0' :
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
    printf("Unused chunks: %lu\n", count_nodes(unused_chunks));

    double *ptrs[15] = {NULL};

    for(int i = 0; i < 10; i++){
        double *all = bogoalloc(8 + i);
        *all = i;
        printf("heap head = %p, all = %p\n", heap, all);
        ptrs[i] = all;
    }

    printf("Unused chunks: %lu\n", count_nodes(unused_chunks));

    dump_heap();

    bogofree(ptrs[0]);

    printf("After free: Unused chunks: %lu\n", count_nodes(unused_chunks));

    dump_heap();

    bogofree(ptrs[3]);

    printf("After free2: Unused chunks: %lu\n", count_nodes(unused_chunks));

    dump_heap();

    void *ptr16 = bogoalloc(16);

    printf("After alloc2: Unused chunks: %lu\n", count_nodes(unused_chunks));

    dump_heap();

    bogofree(ptr16);

    printf("After free3: Unused chunks: %lu\n", count_nodes(unused_chunks));

    dump_heap();

    ptr16 = bogoalloc(16);

    printf("After alloc3: Unused chunks: %lu\n", count_nodes(unused_chunks));

    dump_heap();

    printf("sizeof size_t: %lu\n", sizeof(size_t));
    printf("sizeof Node: %lu\n", sizeof(Node));

    return 0;
}
