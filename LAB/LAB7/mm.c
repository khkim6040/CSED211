/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * explicit free list
 * first fit
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) // definition of size_t to align 8 bytes

#define ALLOCATED 1         // allocated block
#define FREE 0              // unallocated block
#define WSIZE 4             // word size
#define DSIZE 8             // double word size
#define MIN_BLOCK_SIZE 24   // minimum block size
#define CHUNKSIZE (1 << 12) // chunk size of heap extension(4KB)

#define MAX(x, y) ((x) > (y) ? (x) : (y)) // max value

#define PACK(size, alloc) ((size) | (alloc)) // package size and allocated bit.

#define GET(p) (*(unsigned int *)(p))              // read 4 bytes from addr p.
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // write 4 bytes val to addr p.

#define GET_SIZE(p) (GET(p) & ~0x7) // read size from addr p. TODO: refactor to use HDRP in it
#define GET_ALLOC(p) (GET(p) & 0x1) // read allocated bit from addr p.

#define HDRP(bp) ((char *)(bp)-WSIZE)                        // get header addr from block ptr bp.
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // get footer addr from block ptr bp.

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) // get next block ptr from block ptr bp.
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))   // get prev block ptr from block ptr bp.

#define NEXT_FREEP(bp) (*(char **)(bp))         // get next free block ptr from free block ptr bp.
#define PREV_FREEP(bp) (*(char **)(bp + WSIZE)) // get prev free block ptr from free block ptr bp.

static char *heap_listp = 0; // heap start pointer
static char *free_listp;     // free list start pointer

static void *extend_heap(size_t size);     // extend heap by size
static void *coalesce(void *bp);           // coalesce free blocks
static void *find_fit(size_t asize);       // find free block by first fit policy
static void place(void *bp, size_t asize); // place block by asize
static void *push(void *bp);               // push free block to free list
static void pop(void *bp);                 // pop free block from free list
static void mm_check();                    // check heap consistency

#define DEBUG

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    // 초기 heap 생성
    if ((heap_listp = mem_sbrk(MIN_BLOCK_SIZE)) == (void *)-1) {
        return -1;
    }
    // heap의 모양: padding | header(4bytes) | payload(8bytes) | footer(4bytes) | epilogue
    PUT(heap_listp, ALLOCATED);                              // alignment padding
    PUT(heap_listp + 1 * WSIZE, PACK(MIN_BLOCK_SIZE, FREE)); // header
    PUT(heap_listp + 2 * WSIZE, NULL);                       // next
    PUT(heap_listp + 3 * WSIZE, NULL);                       // prev
    // PUT(heap_listp + MIN_BLOCK_SIZE, PACK(MIN_BLOCK_SIZE, FREE)); // footer
    // PUT(heap_listp + MIN_BLOCK_SIZE + WSIZE, PACK(0, ALLOCATED)); // epilogue
    PUT(heap_listp + 4 * WSIZE, PACK(MIN_BLOCK_SIZE, FREE)); // footer
    PUT(heap_listp + 5 * WSIZE, PACK(0, ALLOCATED));         // epilogue
    heap_listp += DSIZE;                                     // heap start pointer
    free_listp = heap_listp;                                 // free list start pointer

    // heap 확장
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
#ifdef DEBUG
    printf("in mm_init\n");
    mm_check();
#endif
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    // size를 적절한 크기asize로 맞추기
    // first fit 정책에 따라 적절한 곳 찾기
    // 찾은 곳에서 asize만큼 할당하기
    // 만약 찾지 못했다면, heap을 확장하고 할당하기
    int asize = MAX(ALIGN(size + SIZE_T_SIZE), MIN_BLOCK_SIZE);
    printf("asize: %d\n", asize);
    char *bp = find_fit(asize);
    printf("bp: %d\n", bp);
    // if bp == NULL, extend heap
    if (bp == NULL) {
        size_t extend_size = MAX(asize, CHUNKSIZE);
        printf("extend_size: %d\n", extend_size);
        if ((bp = extend_heap(extend_size / WSIZE)) == NULL) {
            printf("extend heap failed\n");
            return NULL;
        }
    }

    place(bp, asize);
    printf("bp: %d\n", bp);

#ifdef DEBUG
    printf("in mm_malloc\n");
    mm_check();
#endif
    return bp;
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
    //     return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
}

/*
 * find fit block by first fit policy
 */
static void *find_fit(size_t asize) {
    char *bp = free_listp;
    while (bp) {
        if (GET_SIZE(HDRP(bp)) >= asize) {
            return bp;
        }
        bp = NEXT_FREEP(bp);
    }

    return NULL;
}

/*
 *
 * param: bp-block point of free list, asize-aligned size of malloc
 */
static void place(void *bp, size_t asize) {
    size_t free_block_size = GET_SIZE(HDRP(bp));

    size_t size_diff = free_block_size - asize;
    // split
    if (size_diff >= MIN_BLOCK_SIZE) {
        PUT(HDRP(bp), PACK(asize, ALLOCATED));
        PUT(FTRP(bp), PACK(asize, ALLOCATED));
        // split
        PUT(HDRP(bp + asize), PACK(size_diff, FREE));
        PUT(FTRP(bp + asize), PACK(size_diff, FREE));
        coalesce(bp + asize);
    } else {
        PUT(HDRP(bp), PACK(free_block_size, ALLOCATED));
        PUT(FTRP(bp), PACK(free_block_size, ALLOCATED));
    }
    // remove from free list
    pop(bp);
}

// free list에 push
static void *coalesce(void *bp) {
    int is_prev_allocated = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    int is_next_allocated = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    char *prev = PREV_BLKP(bp);
    char *next = NEXT_BLKP(bp);
    int size = GET_SIZE(HDRP(bp));
    printf("coalesce: %d\n", bp);
    printf("is_prev_allocated: %d\n", is_prev_allocated);
    printf("is_next_allocated: %d\n", is_next_allocated);

    if (is_prev_allocated && is_next_allocated) {
        return push(bp);
    } else if (!is_prev_allocated && is_next_allocated) {
        // disconnect prev from free list
        pop(prev);
        size += GET_SIZE(HDRP(prev));
        PUT(HDRP(prev), PACK(size, FREE));
        PUT(FTRP(bp), PACK(size, FREE));
        return push(prev);
    } else if (is_prev_allocated && !is_next_allocated) {
        // disconnect next from free list
        pop(next);
        size += GET_SIZE(HDRP(next));
        PUT(HDRP(bp), PACK(size, FREE));
        PUT(FTRP(next), PACK(size, FREE));
        return push(bp);
    } else {
        // disconnect prev and next
        pop(prev);
        pop(next);
        GET_SIZE(HDRP(next));
        size += GET_SIZE(HDRP(prev));
        size += GET_SIZE(HDRP(next));
        PUT(HDRP(prev), PACK(size, FREE));
        PUT(FTRP(next), PACK(size, FREE));
        return push(prev);
    }
}

static void pop(void *bp) {
    if (PREV_FREEP(bp) == NULL) {
        free_listp = NEXT_FREEP(bp);
    } else {
        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);
    }
    PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
}

static void *push(void *bp) {
    NEXT_FREEP(bp) = free_listp;
    PREV_FREEP(bp) = NULL;
    PREV_FREEP(free_listp) = bp;
    free_listp = bp;
    return bp;
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    // ptr이 NULL이라면 아무것도 하지 않는다.
    // ptr이 가리키는 블럭의 헤더와 푸터를 수정한다.
    // ptr이 가리키는 블럭을 free list에 추가한다.
    // free list에 추가할 때, coalescing을 수행한다.
    // coalescing을 수행할 때, prev, next를 수정한다.
    if (ptr == NULL) {
        return;
    }
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, FREE));
    PUT(FTRP(ptr), PACK(size, FREE));

    coalesce(ptr);
#ifdef DEBUG
    printf("in mm_free\n");
    mm_check();
#endif
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t asize = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    printf("asize: %d\n", asize);
    if ((long)(bp = mem_sbrk(asize)) == -1) {
        return NULL;
    }
    printf("extend_heap: 0x%x\n", bp);

    PUT(HDRP(bp), PACK(asize, FREE));
    PUT(FTRP(bp), PACK(asize, FREE));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, ALLOCATED));
    return coalesce(bp);
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    // ptr이 NULL이라면 mm_malloc(size)를 호출한 결과를 반환한다.
    // size가 0보다 작다면 mm_free(ptr)를 호출한 결과를 반환한다.
    // ptr과 size 모두 있다면 기존 ptr을 새로운 size로 재할당한다.
    // 만약 새로운 size가 기존 size보다 작다면, ptr 유지한 채로 size를 줄인다.
    // 줄여서 남은 공간이 24바이트 이상이라면, 나머지 공간을 free list에 추가한다. 아니라면 뭐 하지
    // 말고 그냥 ptr 반환. 새로운 size가 기존 size보다 크다면, mm_malloc(size)를 호출하고 기존 ptr을
    // free하고 새로운 ptr을 반환한다.
    size_t asize = MAX(size + DSIZE, MIN_BLOCK_SIZE);
    if (ptr == NULL) {
        return mm_malloc(asize);
    }
    if (size <= 0) {
        mm_free(ptr);
    }
    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t size_diff = old_size - asize;
    void *new_ptr;
    if (size_diff < 0) {
        new_ptr = mm_malloc(asize);
        memcpy(new_ptr, ptr, old_size);
        mm_free(ptr);
        return new_ptr;
    } else if (size_diff >= MIN_BLOCK_SIZE) {
        PUT(HDRP(ptr), PACK(asize, ALLOCATED));
        PUT(FTRP(ptr), PACK(asize, ALLOCATED));
        PUT(HDRP(ptr + asize), PACK(size_diff, FREE));
        PUT(FTRP(ptr + asize), PACK(size_diff, FREE));
        coalesce(ptr + asize);

    } else {
        PUT(HDRP(ptr), PACK(old_size, ALLOCATED));
        PUT(FTRP(ptr), PACK(old_size, ALLOCATED));
    }
#ifdef DEBUG
    printf("in mm_realloc\n");
    mm_check();
#endif
    return ptr;
}

/*
 * mm_check - check heap consistency
 */
static void mm_check() {
    char *bp = heap_listp;
    printf("heap list: 0x%x\n", bp);

    while (GET_SIZE(HDRP(bp)) != 0) {
        printf("header: %d, footer: %d\n", GET_SIZE(HDRP(bp)), GET_SIZE(FTRP(bp)));
        if (GET_ALLOC(HDRP(bp)) == ALLOCATED) {
            if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))) {
                printf("header size and footer size are different\n");
                // exit(1);
            }
        }
        bp = NEXT_BLKP(bp);
        printf("next block: 0x%x\n", bp);
    }
    bp = free_listp;
    printf("free list: 0x%x\n", bp);
    printf("mem_heap_lo: 0x%x\n", (unsigned int *)mem_heap_lo());
    printf("mem_heap_hi: 0x%x\n", (unsigned int *)mem_heap_hi());
    while (bp) {
        printf("free list: %d\n", GET_SIZE(HDRP(bp)));
        if (GET_ALLOC(HDRP(bp)) == ALLOCATED) {
            printf("free list has allocated block\n");
            // exit(1);
        }
        // ???? 왜 sigfault?
        // if (GET(bp) == NULL) {
        //     printf("free list has NULL\n");
        //     break;
        // }
        bp = NEXT_FREEP(bp);
        printf("next free block: %d\n", GET_SIZE(HDRP(bp)));
    }
    printf("free list end\n");
}
