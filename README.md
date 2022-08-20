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
} Chunk;

typedef struct ChunkList {
    Chunk chunks[CHUNK_NUM];
    size_t size;
} ChunkList;
```

We can dump the memory usage like this.

```
000000: <------>[+++++++]???????[******* *]??????[+++++++++]?????[******* ***]????[+++++++++++]???[******* *****]??[+++++++++++++]?[*******
000128: *******][+++++++++++++++]??????? [***********]???[++++++++++++]?? [*************]?[++++++++++++++] [***************]???????[+++++++
000256: ++++++++++++++++++++++++++++++++ ++++++++++++++++++++++++++++++++ ++++++++++++++++++++++++++++++++ +++++++++++++++++++++++]<-------
000384: -------------------------------- -------------------------------- -------------------------------- --------------------------------
000512: -------------------------------- -------------------------------- -------------------------------- --------------------------------
000640: -------------------------------- -------------------------------- -------------------------------- --------------------------------
```

Each character represents a byte. The `<--->` indicates free chunk, `[++++]` or `[****]` mean allocated chunk (the difference is to
make it more distinguishable), and the character `?` means a memory that are not managed by the list.

Note that we are aligning the address to 8 bytes, so there are some padding between chunks, which shows up as `?`.
