#include "allocator.h"
#include "internals.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Buddy allocator

// The allocator has sz_info for each size k. Each sz_info has a free
// list, an array alloc to keep track which blocks have been
// allocated, and a split array to keep track which blocks have
// been split. The arrays are of type char (which is 1 byte), but the
// allocator uses 1 bit per block (thus, one char records the info of
// 8 blocks).
// Allocator supports allocation up to 8e6 TB of memory.
typedef struct {
    List free;
    char* split;
    char* pair_state;
} Sz_info;

typedef struct {
    int nsizes;     // the number of entries in bd_sizes array
    Sz_info* sizes; // array of size levels
    void* base;     // start address of memory managed by the buddy allocator
} BuddyAllocator;

struct Allocator {
    BuddyAllocator bd; // buddy allocator
    FILE* mmap_file;   // memory mapped file with data
    void* mmap_addr;   // start of memory mapped region
    size_t mmap_len;   // length of memory mapped file
};

#define LEAF_SIZE 16          // The smallest block size
#define MAXSIZE(nsizes) ((nsizes) - 1)  // Largest index in bd_sizes array
#define BLK_SIZE(k) ((1ULL << (k)) * LEAF_SIZE)  // Size of block at size k
#define HEAP_SIZE(nsizes) BLK_SIZE(MAXSIZE((nsizes)))
#define NBLK(k, nsizes) (1ULL << (MAXSIZE((nsizes)) - (k)))  // Number of blocks at size k
#define ROUNDUP(n, sz) \
  (((((n) - 1) / (sz)) + 1) * (sz))  // Round up to the next multiple of sz


#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

// Return 1 if a bit at position `index` in `array` is set to 1
bool bit_isset(char const* array, size_t index) {
    char b = array[index / 8];
    char m = (1 << (index % 8));
    return (b & m) == m;
}

// Set a bit at position `index` in `array` to 1
void bit_set(char* array, size_t index) {
    char b = array[index / 8];
    char m = (1 << (index % 8));
    array[index / 8] = (b | m);
}

// Clear bit at position `index` in `array`
void bit_clear(char* array, size_t index) {
    char b = array[index / 8];
    char m = (1 << (index % 8));
    array[index / 8] = (b & ~m);
}

// Flip bit at position `index` in `array`
void bit_flip(char* array, size_t index) {
    array[index / 8] ^= (1 << (index % 8));
}

#pragma clang diagnostic pop

// Print a bit vector as a list of ranges of 1 bits
void bd_print_vector(char const* vector, size_t len) {
    bool last = 1;
    size_t lb = 0;
    for (size_t b = 0; b < len; b++) {
        if (last == bit_isset(vector, b)) continue;
        if (last == 1) fprintf(stderr, " [%zu, %zu)", lb, b);
        lb = b;
        last = bit_isset(vector, b);
    }
    if (lb == 0 || last) {
        fprintf(stderr, " [%zu, %zu)", lb, len);
    }
    fprintf(stderr, "\n");
}

// Print buddy's data structures
__attribute__((unused)) void bd_print(BuddyAllocator const* bd) {
    for (int k = 0; k < bd->nsizes; k++) {
        fprintf(stderr, "size %d (blksz %llu nblk %llu): free list: ",
                k, BLK_SIZE(k), NBLK(k, bd->nsizes));
        lst_print(&bd->sizes[k].free);
        fprintf(stderr, "  pair_state:");
        bd_print_vector(bd->sizes[k].pair_state, NBLK(k > 0 ? k - 1 : 1, bd->nsizes));
        if (k > 0) {
            fprintf(stderr, "  split:");
            bd_print_vector(bd->sizes[k].split, NBLK(k, bd->nsizes));
        }
    }
}

// What is the first k such that 2^k >= n?
int firstk(uint64_t n) {
    int k = 0;
    uint64_t size = LEAF_SIZE;

    while (size < n) {
        k++;
        size *= 2;
    }
    return k;
}

// Compute the block index for address p at size k
size_t blk_index(BuddyAllocator const* bd, int k, char const* p) {
    size_t n = p - (char*) bd->base;
    return n / BLK_SIZE(k);
}

// Convert a block index at size k back into an address
void* addr(BuddyAllocator const* bd, int k, size_t bi) {
    size_t n = bi * BLK_SIZE(k);
    return (char*) bd->base + n;
}

// allocate nbytes, but malloc won't return anything smaller than LEAF_SIZE
void* bd_alloc(BuddyAllocator* bd, size_t nbytes) {

    // Find a free block >= nbytes, starting with smallest k possible
    int fk = firstk(nbytes);
    int k = fk;
    for (; k < bd->nsizes; k++) {
        if (!lst_empty(&bd->sizes[k].free)) break;
    }
    if (k >= bd->nsizes) {  // No free blocks?
        // todo grow file size (setup another buddy allocator for next mmap_len batch of bytes, call ftruncate)
        assert(false);
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
        return NULL;
#pragma clang diagnostic pop
    }

    // Found a block; pop it and potentially split it.
    char* p = lst_pop(&bd->sizes[k].free);
    bit_flip(bd->sizes[k].pair_state, blk_index(bd, k + 1, p));
    for (; k > fk; k--) {
        // split a block at nbytes k and mark one half allocated at nbytes k-1
        // and put the buddy on the free list at nbytes k-1
        char* q = p + BLK_SIZE(k - 1);  // p's buddy
        bit_set(bd->sizes[k].split, blk_index(bd, k, p));
        bit_set(bd->sizes[k - 1].pair_state, blk_index(bd, k, p));
        lst_push(&bd->sizes[k - 1].free, q);
    }

    return p;
}

void* alloc_malloc(Allocator* allocator, size_t size) {
    return bd_alloc(&allocator->bd, size);
}

// Find the size of the block that p points to.
int size(BuddyAllocator const* bd, char* p) {
    for (int k = 0; k < bd->nsizes; k++) {
        if (bit_isset(bd->sizes[k + 1].split, blk_index(bd, k + 1, p))) {
            return k;
        }
    }
    return 0;
}

// Free memory pointed to by p, which was earlier allocated using bd_alloc.
void bd_free(BuddyAllocator* bd, void* p) {
    int k;

    for (k = size(bd, p); k < MAXSIZE(bd->nsizes); k++) {
        size_t bi = blk_index(bd, k, p);
        size_t buddy = (bi % 2 == 0) ? bi + 1 : bi - 1;
        bit_flip(bd->sizes[k].pair_state, bi / 2);         // flip state
        if (bit_isset(bd->sizes[k].pair_state, bi / 2)) {  // is buddy allocated?
            break;                                                    // break out of loop
        }
        // buddy is free; merge with buddy
        void* q = addr(bd, k, buddy);
        lst_remove(q);  // remove buddy from free list
        if (buddy % 2 == 0) {
            p = q;
        }
        // at size k+1, mark that the merged buddy pair isn't split anymore
        bit_clear(bd->sizes[k + 1].split, blk_index(bd, k + 1, p));
    }
    lst_push(&bd->sizes[k].free, p);
}

void alloc_free(Allocator* allocator, void* ptr) {
    bd_free(&allocator->bd, ptr);
}

// Compute the first block at size k that doesn't contain p
size_t blk_index_next(BuddyAllocator const* bd, int k, char const* p) {
    size_t n = (p - (char*) bd->base) / BLK_SIZE(k);
    if ((p - (char*) bd->base) % BLK_SIZE(k) != 0) n++;
    return n;
}

int ilog2(uint64_t n) {
    int k = 0;
    while (n > 1) {
        k++;
        n = n >> 1;
    }
    return k;
}

// Mark memory from [start, stop), starting at size 0, as allocated.
void bd_mark(BuddyAllocator* bd, void* start, void* stop) {

    assert(((uint64_t) start % LEAF_SIZE == 0) && ((uint64_t) stop % LEAF_SIZE == 0));

    for (int k = 0; k < bd->nsizes; k++) {
        size_t bi = blk_index(bd, k, start);
        size_t bj = blk_index_next(bd, k, stop);
        for (; bi < bj; bi++) {
            if (k > 0) {
                // if a block is allocated at size k, mark it as split too.
                bit_set(bd->sizes[k].split, bi);
            }
            bit_flip(bd->sizes[k].pair_state, bi / 2);
            // maybe u can consider all cases like bi % 2 == 0,
            // bi % 2 != 0, bj % 2 == 0, bj % 2 != 0,
            // and that'll save us a few operations, but i'm too lazy
        }
    }
}

// If a block is marked as allocated and the buddy is free, put the
// buddy on the free list at size k.
size_t bd_initfree_pair(BuddyAllocator* bd, int k, size_t bi, bool is_left) {
    size_t buddy = (bi % 2 == 0) ? bi + 1 : bi - 1;
    size_t free = 0;
    if (bi < NBLK(k, bd->nsizes) && bit_isset(bd->sizes[k].pair_state, bi / 2)) {
        // one of the pair is free
        free = BLK_SIZE(k);
        if (is_left) {
            // put right block on free list
            size_t max = (bi > buddy ? bi : buddy);
            lst_push(&bd->sizes[k].free, addr(bd, k, max));
        } else {
            // put left block on free list
            size_t min = (bi < buddy ? bi : buddy);
            lst_push(&bd->sizes[k].free, addr(bd, k, min));
        }
    }
    return free;
}

// Initialize the free lists for each size k.  For each size k, there
// are only two pairs that may have a buddy that should be on free list:
// bd_left and bd_right.
size_t bd_initfree(BuddyAllocator* bd, void* bd_left, void* bd_right) {
    size_t free = 0;

    for (int k = 0; k < MAXSIZE(bd->nsizes); k++) {  // skip max size
        size_t left = blk_index_next(bd, k, bd_left);
        size_t right = blk_index(bd, k, bd_right);
        free += bd_initfree_pair(bd, k, left, 1);
        if (right <= left) continue;
        free += bd_initfree_pair(bd, k, right, 0);
    }
    return free;
}

// Mark the range [bd_base,p) as allocated
size_t bd_mark_data_structures(BuddyAllocator* bd, char* p) {
    size_t meta = p - (char*) bd->base;
    fprintf(stderr, "bd: %td meta bytes for managing %llu bytes of memory\n",
            meta, HEAP_SIZE(bd->nsizes));
    bd_mark(bd, bd->base, p);
    return meta;
}

// Mark the range [end, HEAPSIZE) as allocated
size_t bd_mark_unavailable(BuddyAllocator* bd, void* end) {
    size_t unavailable = HEAP_SIZE(bd->nsizes) - (end - bd->base);
    if (unavailable > 0) unavailable = ROUNDUP(unavailable, LEAF_SIZE);
    fprintf(stderr, "bd: 0x%zx bytes unavailable\n", unavailable);

    void* bd_end = bd->base + HEAP_SIZE(bd->nsizes) - unavailable;
    bd_mark(bd, bd_end, bd->base + HEAP_SIZE(bd->nsizes));
    return unavailable;
}

// Initialize the buddy allocator: it manages memory from [base, end).
void bd_init(BuddyAllocator* bd, void* base, void* end) {
    assert(bd);

    char* p = (char*) ROUNDUP((uint64_t) base, LEAF_SIZE);
    bd->base = (void*) p;

    // compute the number of sizes we need to manage [base, end)
    bd->nsizes = ilog2(((char*) end - p) / LEAF_SIZE) + 1;
    if ((char*) end - p > HEAP_SIZE(bd->nsizes)) {
        bd->nsizes++;  // round up to the next power of 2
    }

    fprintf(stderr, "bd: memory sz is %ld bytes; allocate an size array of length %d\n",
            (char*) end - p, bd->nsizes);

    // allocate bd_sizes array
    bd->sizes = (Sz_info*) p;
    p += sizeof(Sz_info) * bd->nsizes;
    memset(bd->sizes, 0, sizeof(Sz_info) * bd->nsizes);

    // initialize free list and allocate the alloc array for each size k
    for (int k = 0; k < bd->nsizes; k++) {
        lst_init(&bd->sizes[k].free);
        size_t sz = sizeof(char) * ROUNDUP((k < MAXSIZE(bd->nsizes)
                                            ? NBLK(k + 1, bd->nsizes)
                                            : NBLK(k, bd->nsizes)), 8) / 8;
        bd->sizes[k].pair_state = p;
        memset(bd->sizes[k].pair_state, 0, sz);
        p += sz;
    }

    // allocate the split array for each size k, except for k = 0, since
    // we will not split blocks of size k = 0, the smallest size.
    for (int k = 1; k < bd->nsizes; k++) {
        size_t sz = sizeof(char) * (ROUNDUP(NBLK(k, bd->nsizes), 8)) / 8;
        bd->sizes[k].split = p;
        memset(bd->sizes[k].split, 0, sz);
        p += sz;
    }
    p = (char*) ROUNDUP((uint64_t) p, LEAF_SIZE);

    // done allocating; mark the memory range [base, p) as allocated, so
    // that buddy will not hand out that memory.
    size_t meta = bd_mark_data_structures(bd, p);

    // mark the unavailable memory range [end, HEAP_SIZE) as allocated,
    // so that buddy will not hand out that memory.
    size_t unavailable = bd_mark_unavailable(bd, end);
    void* bd_end = bd->base + HEAP_SIZE(bd->nsizes) - unavailable;

    // initialize free lists for each size k
    size_t free = bd_initfree(bd, p, bd_end);

    // check if the amount that is free is what we expect
    if (free != HEAP_SIZE(bd->nsizes) - meta - unavailable) {
        fprintf(stderr, "free %zu %llu\n", free, HEAP_SIZE(bd->nsizes) - meta - unavailable);
        assert(false);
    }
}

Allocator* alloc_create(char const* filename, size_t initial_size) {
    FILE* fd;
    fd = fopen(filename, "r");
    if (!fd) {
        // the file does not exist
        // try to create
        fd = fopen(filename, "w+");
        if (!fd) {
            fprintf(stderr, "Unable to create file %s.", filename);
            return NULL;
        }
    } else {
        fclose(fd);
        fd = fopen(filename, "r+");
        if (!fd) {
            fprintf(stderr, "Unable to open existing file %s for reading and writing.", filename);
        }
    }

    Allocator* res = (Allocator*) malloc(sizeof(Allocator));
    if (!res) return NULL;
    res->mmap_file = fd;
    res->mmap_len = initial_size;
    res->mmap_addr = mmap(NULL, res->mmap_len, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fileno(fd), 0);
    if (res->mmap_addr == MAP_FAILED) return NULL;
    if (ftruncate(fileno(fd), res->mmap_len)) return NULL; // NOLINT(*-narrowing-conversions)
    bd_init(&res->bd, res->mmap_addr, res->mmap_addr + res->mmap_len);
    return res;
}

void alloc_destroy(Allocator* allocator) {
    munmap(allocator->mmap_addr, allocator->mmap_len);
    fclose(allocator->mmap_file);
    free(allocator);
}
