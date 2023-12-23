/*
 * Kim Gwanho, 20190650
 *
 * Use explicit free list and first fit policy to implement malloc, realloc, and free.
 * Free list is implemented by doubly linked list. Each free block has prev pointer and next pointer.
 *
 * Free block structure: header(4 bytes), prev pointer(4 bytes), next pointer(4 bytes), footer(4 bytes) = 16 bytes
 * | header | prev pointer | next pointer | footer |
 * Allocated block structure: header(4 bytes), payload, footer(4 bytes) = 8 bytes + payload
 * | header |           payload           | footer |
 *
 * Most of the code including macros and functions are from CS:APP3e textbook and lecture notes.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

// basic constants
#define ALIGNMENT 8         // single word (4) or double word (8) alignment
#define ALLOCATED 1         // allocated block
#define FREE 0              // unallocated block
#define WSIZE 4             // word size
#define DSIZE 8             // double word size
#define MIN_BLOCK_SIZE 16   // minimum block size
#define CHUNKSIZE (1 << 12) // chunk size of heap extension(4KB)
// basic macros
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)           // rounds up to the nearest multiple of ALIGNMENT
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))                       // definition of size_t to align 8 bytes
#define MAX(x, y) ((x) > (y) ? (x) : (y))                         // max value
#define PACK(size, alloc) ((size) | (alloc))                      // package size and allocated bit.
#define GET(p) (*(size_t *)(p))                                   // read 4 bytes from addr p.
#define PUT(p, val) (*(size_t *)(p) = (val))                      // write 4 bytes val to addr p.
#define GET_SIZE(p) (GET(p) & ~0x7)                               // read size from addr p.
#define GET_ALLOC(p) (GET(p) & 0x1)                               // read allocated bit from addr p.
#define HDRP(bp) ((void *)(bp)-WSIZE)                             // get header addr from block ptr bp.
#define FTRP(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)      // get footer addr from block ptr bp.
#define NEXT_BLKP(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)))         // get next block ptr from block ptr bp.
#define PREV_BLKP(bp) ((void *)(bp)-GET_SIZE((HDRP(bp) - WSIZE))) // get prev block ptr from block ptr bp.
#define NEXT_FREEP(bp) (*(void **)(bp))                           // get next free block ptr from free block ptr bp.
#define PREV_FREEP(bp) (*(void **)(bp + WSIZE))                   // get prev free block ptr from free block ptr bp.
// static variables to indicate heap and free list
static char *heap_listp = 0; // heap start pointer
static char *free_listp = 0; // free list start pointer
// static functions declaration
static void *find_fit(size_t size);        // find free block by first fit policy
static void place(void *bp, size_t asize); // place block by asize
static void *coalesce(void *bp);           // coalesce free blocks
static void *extend_heap(size_t words);    // extend heap by words * WSIZE
void pop(void *bp);                        // pop free block from free list
void push(void *bp);                       // push free block on top of free list
static void mm_check();                    // check heap consistency

// if below DEBUG is uncommented, print heap consistency in mm_init, mm_malloc, mm_free before return
// #define DEBUG

/*
 * mm_init - Initialize the malloc package.
 */
int mm_init(void) {
    // initialize the heap with a free block including the prologue and epilogue total size of 8 words which is aligned to 16 bytes
    if ((heap_listp = mem_sbrk(2 * MIN_BLOCK_SIZE)) == (void *)-1)
        return -1;
    // put padding at the beginning of the heap
    PUT(heap_listp + (0 * WSIZE), PACK(MIN_BLOCK_SIZE, 1)); // Prologue header
    PUT(heap_listp + (1 * WSIZE), PACK(MIN_BLOCK_SIZE, 0)); // Free block header
    PUT(heap_listp + (2 * WSIZE), PACK(0, 0));              // Space for next pointer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 0));              // Space for prev pointer
    PUT(heap_listp + (4 * WSIZE), PACK(MIN_BLOCK_SIZE, 0)); // Free block footer
    PUT(heap_listp + (5 * WSIZE), PACK(0, 1));              // Epilogue header
    // point free_list to the first header of the first free block
    free_listp = heap_listp + (WSIZE);
    // point heap_listp to the first payload of the first free block
    heap_listp += (2 * WSIZE);

#ifdef DEBUG
    printf("in mm_init\n");
    mm_check();
#endif
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 * Always allocate a block whose size is a multiple of the alignment.
 * param: size-size of malloc
 */
void *mm_malloc(size_t size) {
    // align size to 8 bytes
    size_t asize = MAX(ALIGN(size + SIZE_T_SIZE), MIN_BLOCK_SIZE);
    // find fit block by first fit policy
    char *bp = find_fit(asize);

    // if fit block is not found, extend heap
    if (bp == NULL) {
        // extend size should be more or equal than CHUNKSIZE(4KB)
        size_t extend_size = MAX(asize, CHUNKSIZE);
        // if extend_heap returns NULL which means mem_sbrk returns -1, return NULL
        if ((bp = extend_heap(extend_size / WSIZE)) == NULL) {
            return NULL;
        }
    }

    // place size to fit free block
    place(bp, asize);

#ifdef DEBUG
    printf("in mm_malloc\n");
    mm_check();
#endif
    return bp;
}

/*
 * find_fit - Find fit block by first fit policy
 * param: size-aligned size of malloc
 */
static void *find_fit(size_t size) {
    char *bp = free_listp;
    // from free_listp to epilogue header, find fit block
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)) {
        if (size <= GET_SIZE(HDRP(bp)))
            return bp;
    }

    return NULL;
}

/*
 * place - Place asize to bp of free list
 * param: bp-block point of free list, asize-aligned size of malloc
 */
static void place(void *bp, size_t asize) {
    size_t free_block_size = GET_SIZE(HDRP(bp));
    size_t size_diff = free_block_size - asize;
    // if size difference is bigger than MIN_BLOCK_SIZE(16 bytes), split to save memory
    if (size_diff >= MIN_BLOCK_SIZE) {
        // allocate asize to fornt of free block
        PUT(HDRP(bp), PACK(asize, ALLOCATED));
        PUT(FTRP(bp), PACK(asize, ALLOCATED));
        pop(bp);
        // then, make new free block with size difference(split)
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size_diff, FREE));
        PUT(FTRP(bp), PACK(size_diff, FREE));
        // push new free block to free list
        coalesce(bp);
    } else {
        // if size difference is smaller than MIN_BLOCK_SIZE(16 bytes), allocate all free block
        PUT(HDRP(bp), PACK(free_block_size, ALLOCATED));
        PUT(FTRP(bp), PACK(free_block_size, ALLOCATED));
        pop(bp);
    }
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 * param: bp-block point of free list
 */
static void *coalesce(void *bp) {
    // Determine the current allocation state of the previous and next blocks
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    // Get the size of the current free block
    size_t size = GET_SIZE(HDRP(bp));
    // if next block is free, coalese with next block
    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // pop next block from free list
        pop(NEXT_BLKP(bp));
        // modify current block's header and footer
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // If the previous block is free, coalesce with the previous block
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        // pop previous block from free list
        bp = PREV_BLKP(bp);
        pop(bp);
        // modify previous block's header and footer
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // If both the previous and next blocks are free, coalesce with both
    else if (!prev_alloc && !next_alloc) { // Case 4 (in text)
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // pop previous and next block from free list
        pop(PREV_BLKP(bp));
        pop(NEXT_BLKP(bp));
        // modify previous block's header and footer
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    // push coalesced block to free list
    push(bp);

#ifdef DEBUG
    printf("in coalesce\n");
    mm_check();
#endif
    return bp;
}

/*
 * pop - Pop free block from free list
 * param: bp-block point of free list
 */
void pop(void *bp) {
    // if bp is NULL, do nothing
    if (bp == NULL) {
        return;
    }
    // if bp is last block(top of list) of free list, modify free_listp
    if (PREV_FREEP(bp) == NULL)
        free_listp = NEXT_FREEP(bp);
    // if bp is not last block(top of list) of free list, modify prev block's next pointer
    else
        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);
    // if bp is not first block(bottom of list) of free list, modify next block's prev pointer
    if (NEXT_FREEP(bp) != NULL)
        PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
}

/*
 * push - Push free block to top of free list
 * param: bp-block point of free list
 */
void push(void *bp) {
    NEXT_FREEP(bp) = free_listp;
    PREV_FREEP(bp) = NULL;
    PREV_FREEP(free_listp) = bp;
    free_listp = bp;
}

/*
 * mm_free - Freeing a block does nothing.
 * param: ptr-pointer of block to free
 */
void mm_free(void *ptr) {
    // if ptr is NULL, do nothing
    if (ptr == NULL) {
        return;
    }
    size_t size = GET_SIZE(HDRP(ptr));
    // modify header and footer to free block
    PUT(HDRP(ptr), PACK(size, FREE));
    PUT(FTRP(ptr), PACK(size, FREE));
    // before return, coalesce free block with adjacent free blocks
    coalesce(ptr);
#ifdef DEBUG
    printf("in mm_free\n");
    mm_check();
#endif
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 * param: number of words to extend
 */
static void *extend_heap(size_t words) {
    // block pointer which points payload of new free block
    char *bp;
    // adjusted size to align 8 bytes
    size_t asize;
    // allocate an even number of words to maintain alignment
    asize = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // if asize is smaller than MIN_BLOCK_SIZE(16 bytes), set asize to MIN_BLOCK_SIZE(16 bytes)
    if (asize < MIN_BLOCK_SIZE)
        asize = MIN_BLOCK_SIZE;

    // if mem_sbrk returns -1, return NULL
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;

    // Initialize free block header/footer
    PUT(HDRP(bp), PACK(asize, 0));
    PUT(FTRP(bp), PACK(asize, 0));
    // Move the epilogue header to the end of the newly extended heap
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * param: ptr-pointer of block to realloc, size-size of realloc
 */
void *mm_realloc(void *ptr, size_t size) {
    // align size to 8 bytes
    size_t asize = MAX(ALIGN(size) + DSIZE, MIN_BLOCK_SIZE);
    // if ptr is NULL, do mm_malloc
    if (ptr == NULL) {
        return mm_malloc(asize);
    }
    // if size is 0, do mm_free
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    // otherwise, reallocate ptr to asize comparing with current payload size
    size_t current_size = GET_SIZE(HDRP(ptr));
    void *bp;
    int size_diff = current_size - asize;
    // Case 1: Size is equal to the current payload size
    // just return ptr
    if (size_diff == 0)
        return ptr;
    // Case 2: Requested size is less than the current payload size
    else if (size_diff > 0) {
        // if current block size is bigger than asize, and the remaining block size is bigger than MIN_BLOCK_SIZE(16 bytes), split
        if (asize > MIN_BLOCK_SIZE && size_diff > MIN_BLOCK_SIZE) {
            // split, merge, and release the remaining block
            PUT(HDRP(ptr), PACK(asize, ALLOCATED));
            PUT(FTRP(ptr), PACK(asize, ALLOCATED));
            bp = NEXT_BLKP(ptr);
            // free the remaining block
            PUT(HDRP(bp), PACK(size_diff, ALLOCATED));
            PUT(FTRP(bp), PACK(size_diff, ALLOCATED));
            mm_free(bp);
            return ptr;
        } else {
            // if current block size is bigger than asize, but the remaining block size is smaller than MIN_BLOCK_SIZE(16 bytes), just return ptr
            return ptr;
        }
    }
    // Case 3: Requested size is greater than the current payload size
    else {
        // consider next block size to merge
        char *next = HDRP(NEXT_BLKP(ptr));
        size_t newsize = current_size + GET_SIZE(next);
        int size_diff = newsize - asize;
        // if next block is free and the merged block size is bigger than asize, merge
        if (!GET_ALLOC(next) && size_diff >= 0) {
            // merge, split, and release
            pop(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(asize, ALLOCATED));
            PUT(FTRP(ptr), PACK(asize, ALLOCATED));
            // free the remaining block
            bp = NEXT_BLKP(ptr);
            PUT(HDRP(bp), PACK(size_diff, ALLOCATED));
            PUT(FTRP(bp), PACK(size_diff, ALLOCATED));
            mm_free(bp);
            return ptr;
        } else {
            // if next block is free but the merged block size is smaller than asize, do mm_malloc
            bp = mm_malloc(asize);
            // if bp is NULL, return NULL
            if (bp == NULL)
                return NULL;
            // copy payload to bp
            memcpy(bp, ptr, current_size);
            // free ptr
            mm_free(ptr);
            return bp;
        }
    }
}

/*
 * mm_check - check heap consistency
 * Check the following shown in the writeup:
 * 1. Is every block in the free list marked as free?
 * 2. Are there any contiguous free blocks that somehow escaped coalescing?
 * 3. Is every free block actually in the free list?
 * 4. Do the pointers in the free list point to valid free blocks?
 * 5. Do any allocated blocks overlap?
 * 6. Do the pointers in a heap block point to valid heap addresses?
 */
static void mm_check() {
    void *next;
    // 1. Is every block in the free list marked as free?
    for (next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREEP(next)) {
        // Check the header and footer of each free block whether it is marked as allocated
        if (GET_ALLOC(HDRP(next))) {
            printf("Consistency error: block %p in free list but marked allocated!", next);
            return;
        }
    }
    // 2. Are there any contiguous free blocks that somehow escaped coalescing?
    for (next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREEP(next)) {
        // Check the header and footer of each free block whether it is contiguous with its previous block
        char *prev = PREV_FREEP(HDRP(next));
        if (prev != NULL && HDRP(next) - FTRP(prev) == DSIZE) {
            printf("Consistency error: block %p missed coalescing!", next);
            return;
        }
    }
    // 3. Is every free block actually in the free list?
    for (next = heap_listp; GET_SIZE(HDRP(next)) > 0; next = NEXT_BLKP(next)) {
        // Check the header and footer of each free block whether it is in the free list
        if (GET_ALLOC(HDRP(next)) == 0) {
            char *bp;
            for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)) {
                if (bp == next)
                    break;
            }
            // if bp is NULL, free block is not in the free list
            if (bp == NULL) {
                printf("Consistency error: free block %p not in free list!", next);
                return;
            }
        }
    }
    // 4. Do the pointers in the free list point to valid free blocks?
    for (next = free_listp; GET_ALLOC(HDRP(next)) == 0; next = NEXT_FREEP(next)) {
        // Check the header and footer of each free block whether it is valid
        if (next < mem_heap_lo() || next > mem_heap_hi()) {
            // if address of free block is smaller than mem_heap_lo or bigger than mem_heap_hi, it is invalid
            printf("Consistency error: free block %p invalid", next);
            return;
        }
    }
    // 5. Do any allocated blocks overlap?
    for (next = heap_listp; GET_SIZE(HDRP(next)) > 0; next = NEXT_BLKP(next)) {
        // Check the header and footer of each allocated block whether it is overlapped with its previous block
        if (GET_ALLOC(HDRP(next))) {
            // if prev block is allocated and the size difference is smaller than WSIZE(4 bytes), it is overlapped
            char *prev = PREV_FREEP(HDRP(next));
            if (prev != NULL && GET_ALLOC(HDRP(prev)) && HDRP(next) - FTRP(prev) < DSIZE) {
                printf("Consistency error: allocated block %p overlaps with allocated block %p!", next, prev);
                return;
            }
        }
    }
    // 6. Do the pointers in a heap block point to valid heap addresses?
    for (next = heap_listp; GET_SIZE(HDRP(next)) > 0; next = NEXT_BLKP(next)) {
        if (next < mem_heap_lo() || next > mem_heap_hi()) {
            printf("Consistency error: block %p outside designated heap space", next);
            return;
        }
    }
}
