# Bogoalloc

A self-learning project that implements malloc, inspired by
https://youtu.be/sZ8GJ1TiMdk.

## Memory allocation

C is still the best language to learn something low level like this.

We keep track of allocated and freed chunks of memory with this data structure.

```C
typedef struct Chunk {
    unsigned char *head;
    size_t sz;
    size_t id; // Unique id to make debugging easier
    struct Chunk *next; // Pointer to the next node in the linked list
} Chunk;
```

Chunks are a linked list with three heads.
There is a global chunk pool `chunk_list` that provides the storage of the elements in the linked list.

```c
static Chunk chunk_list[CHUNK_NUM] = /**/;
static Chunk *alloc_chunks = NULL;
static Chunk *free_chunks;
static Chunk *unused_chunks;
```

* `alloc_chunks` obviously means a list of allocated chunks.
* `free_chunks` means a list of empty gaps in the allocations.
* `unused_chunks` means a chunk entry buffer that are not used. Initialized with a full linked list in `chunk_list`. Necessary for reusing merged chunks.

The difference between `free_chunks` and `unused_chunks` may not be obvious, but important.
Since we can merge adjacent free chunks into one big free chunk, we may "un-use" a chunk.
An unused chunk does not have valid `head` or `sz`, but necessary to keep track of available elements
in `chunk_list`, since "un-using" a chunk can happen at any index.
If we don't use `unused_chunks`, merged chunks will leak and consume `chunk_list` over time, even if the amount of total allocated memory does not
increase.

Alternative implementation could use "in-use" flag for each chunk, but it would
require O(n) to find an un-used chunk.
Since we already use a linked list for managing chunks among allocated and free
chunk list, we can just use it with additional head pointer.
However, this implementation requires initializing the `chunk_list` before using.

The linked list has advantage over the array implementation in that:

* Inserting/deleting a node is O(1)
* We can share the same node pool for both free list and allocated list

However, it has a downside:

* Searching a node with a given address is O(n), and cannot be optimized

Therefore, `bogoalloc` is O(n) in the worst case.

We can dump the memory usage like this.

```
000000: <01---->[02+++++]???????[03+++++ +++++++][04*******]?????[05***** *******][06+++++++++]???[07+++++ +++++++][08***********]?[09*****
000128: *******][10+++++++++++++]??????? [11*************]???????[12+++++ ++++++++++++++++++++++++++++++++ ++++++++++++++++++++++++++++++++
000256: ++++++++++++++++++++++++++++++++ +++++++++++++++++++++++]<00----- -------------------------------- --------------------------------
000384: -------------------------------- -------------------------------- -------------------------------- --------------------------------
000512: -------------------------------- -------------------------------- -------------------------------- --------------------------------
000640: -------------------------------- -------------------------------- -------------------------------- --------------------------------
```

Each character represents a byte. The `<--->` indicates free chunk, `[++++]` or `[****]` mean allocated chunk (the difference is to
make it more distinguishable), and the character `?` means a memory that are not managed by the list.
The numbers in the brackets are the ids of the chunks.

Note that we are aligning the address to 8 bytes, so there are some padding between chunks, which shows up as `?`.
