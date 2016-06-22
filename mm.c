/* 
 * Simple, 32-bit and 64-bit clean allocator based on an implicit free list,
 * first fit placement, and boundary tag coalescing, as described in the
 * CS:APP2e text.  Blocks are aligned to double-word boundaries.  This
 * yields 8-byte aligned blocks on a 32-bit processor, and 16-byte aligned
 * blocks on a 64-bit processor.  However, 16-byte alignment is stricter
 * than necessary; the assignment only requires 8-byte alignment.  The
 * minimum block size is four words.
 *
 * This allocator uses the size of a pointer, e.g., sizeof(void *), to
 * define the size of a word.  This allocator also uses the standard
 * type uintptr_t to define unsigned integers that are the same size
 * as a pointer, i.e., sizeof(uintptr_t) == sizeof(void *).
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
team_t team = {
    /* Team name */
    "April Coders"    ,
    /* First member's full name */
    "Nandini Nerurkar",
    /* First member's email address */
    "201401121@daiict.ac.in",
    /* Second member's full name (leave blank if none) */
    "Malvika Singh",
    /* Second member's email address (leave blank if none) */
    "201401428@daiict.ac.in"
};


/* Double word allignment */
#define ALIGNMENT 2 * sizeof(void *)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)+ DSIZE) & ~0x7)

/* Basic constants and macros */
#define WSIZE       sizeof(void *)/* Word and header/footer size (bytes) */ 
#define DSIZE       2 * WSIZE    /* doubleword size (bytes) */
#define CHUNKSIZE   1<<12    /* initial heap size (bytes) */
#define MINIMUM    6 * WSIZE  /* minimum block size */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(int *)(p))
#define PUT(p, val)  (*(int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((void *)(bp) - WSIZE)
#define FTRP(bp)       ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)  ((void *)(bp) - GET_SIZE(HDRP(bp) - WSIZE))

/* Given block ptr bp, compute address of next and previous free blocks */
#define NEXT_FREEP(bp)(*(void **)(bp + DSIZE))
#define PREV_FREEP(bp)(*(void **)(bp))

/* Global variables: */
static char *heap_listp = 0;  /* Pointer to the first block */
static char *free_listp = 0; /* Pointer to the first free block */

/* Function prototypes for internal helper routines: */
static void *extendHeap(size_t words);
static void place(void *bp, size_t asize);
static void *findFit(size_t asize);
static void *coalesce(void *bp);

/* Function prototypes for heap consistency checker routines: */
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);
static void add(void *bp);     /* insert in linked list */
static void delete(void *bp);  /* remove from linked list */

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Initialize the memory manager.  Returns 0 if the memory manager was
 *   successfully initialized and -1 otherwise.
 */
int mm_init(void)
{
   /* Create the initial empty heap. */
    if ((heap_listp = mem_sbrk(2*MINIMUM)) == NULL)
        return -1;

    PUT(heap_listp, 0);                               //Alignment padding   
    PUT(heap_listp + (1 * WSIZE), PACK(MINIMUM, 1));  //Header
    PUT(heap_listp + (2 * WSIZE), 0);                 //PREV pointer
    PUT(heap_listp + (3 * WSIZE), 0);                 //NEXT pointer   
   
    PUT(heap_listp + MINIMUM, PACK(MINIMUM, 1));      //Footer	
    PUT(heap_listp + WSIZE + MINIMUM, PACK(0, 1));    //Epilogue

    /* Set the pointer of the free list to start of the heap 
     * since initially the entire heap is free
     */
    free_listp = heap_listp + DSIZE;	

    /* Extend the empty heap with a free block of CHUNKSIZE bytes. */
    if (extendHeap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Allocate a block with at least "size" bytes of payload, unless "size" is
 *   zero.  Returns the address of this block if the allocation was successful
 *   and NULL otherwise.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

	asize = MAX(ALIGN(size) , MINIMUM);

	/* Search the free list for a fit. */
	if ((bp = findFit(asize)) != NULL) {
		place(bp, asize);
		return (bp);
	}

	/* No fit found.  Get more memory and place the block. */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extendHeap(extendsize / WSIZE)) == NULL)  
		return (NULL);
	place(bp, asize);
	return (bp);
    
}

/*
 * Requires:
 *   "bp" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Free a block.
 */
void mm_free(void *bp)
{
	/* Ignore spurious requests. */
    if(bp == NULL) 
	return; 
    size_t size = GET_SIZE(HDRP(bp));

    //set header and footer to unallocated
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);  //coalesce and add the block to the free list
}

/*
 * mm_realloc - Reallocate a block
 * This function extends or shrinks an allocated block.
 * If the new size is less than the old size, return the old pointer
 * If the new size is greater than the old size, checking it's previous and next blocks, and if free,
* add the sizes of respective adjacent free blocks and return the pointer.
 *
 * This function takes a block pointer and a new size as parameters and
 * returns a block pointer to the newly allocated block.
 */
void *mm_realloc(void *bp, size_t size)
{    
	/*Ignore spurious requests */
        if((int)size < 0) 
    		return NULL; 

	/* If size == 0 then this is just free, and we return NULL. */
  	else if((int)size == 0){ 
    		mm_free(bp); 
    		return NULL; 
  	} 
  	else if(size > 0){ 
      size_t oldsize = GET_SIZE(HDRP(bp)); 
      size_t newsize = size + 2 * WSIZE; // 2 words for header and footer

      /*if newsize is less than oldsize then return bp */
      if(newsize <= oldsize){ 
          return bp; 
      }
      /*if newsize is greater than oldsize */ 
      else { 
          size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); 
          size_t csize;
          /* if the next block is free and the size of the two blocks is greater than or equal the new size  */ 
          /* then combine both the blocks  */ 
          if(!next_alloc && ((csize = oldsize + GET_SIZE(HDRP(NEXT_BLKP(bp))))) >= newsize){ 
            delete(NEXT_BLKP(bp)); 
            PUT(HDRP(bp), PACK(csize, 1)); 
            PUT(FTRP(bp), PACK(csize, 1)); 
            return bp; 
          }
          else {  
            void *new_ptr = mm_malloc(newsize);  
            place(new_ptr, newsize);
            memcpy(new_ptr, bp, newsize); 
            mm_free(bp); 
            return new_ptr; 
          } 
      }
  }
	else 
    		return NULL;
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Extend the heap with a free block and return that block's address.
 */
static void *extendHeap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if (size < MINIMUM)
        size = MINIMUM;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/*
 * Requires:
 *   "bp" is the address of a newly freed block.
 *
 * Effects:
 *   Perform boundary tag coalescing.  Returns the address of the coalesced
 *   block.
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1, left coalesce */
     if (prev_alloc && !next_alloc)
    {           
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /* Case 2, right coalesce */
    else if (!prev_alloc && next_alloc)
    {       
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        delete(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /* Case 3, extend the block in both directions */
    else if (!prev_alloc && !next_alloc)
    {       
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete(PREV_BLKP(bp));
        delete(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
   
    add(bp);   
    return bp;
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Find a fit in the explicit free list for a block with "asize" bytes.  Returns that block's address
 *   or NULL if no suitable block was found.
 */
static void *findFit(size_t asize)
{
    void *bp;
    /* First fit search */   
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp))
    {
        if (asize <= (size_t)GET_SIZE(HDRP(bp)))
            return bp;
    }
    return NULL; // No fit
}

/*
 * Requires:
 *   "bp" is the address of a free block that is at least "asize" bytes.
 *
 * Effects:
 *   Place a block of "asize" bytes at the start of the free block "bp" and
 *   split that block if the remainder would be at least the minimum block
 *   size.
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    /* If the difference is at least MINIMUM bytes, change the header and footer
     * info for the allocated block (size = asize, allocated = 1) and
     * remove the block from the free list.
     * Then, split the block by:
     * (1) Changing the header and footer info for the free block created from the
     * remaining space (size = csize-asize, allocated = 0)
     * (2) Coalescing the new free block with adjacent free blocks
    */
    if ((csize - asize) >= MINIMUM) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

	/* Deleting block from free list */
        delete(bp);
        bp = NEXT_BLKP(bp);

	/*Splitting the block */
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));

	/*Coalescing the newly freed block */
        coalesce(bp);
    }
    /* If the remaining space is not enough for a free block, don't split the block */
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        delete(bp);
    }
}

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Perform a minimal check of the heap for consistency. 
 */
static void checkheap(int verbose)
{
    	void *bp = heap_listp, *bp1; 
	int heap = 0, free = 0;
	void *curr = heap_listp;
	//printf("HI\n");

    if (verbose)
        printf("Heap (%p):\n", heap_listp);

    
    if ((GET_SIZE(HDRP(heap_listp)) != MINIMUM) ||
            !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp); 

	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (verbose){
			printblock(bp);}
		checkblock(bp);
	}

	if (verbose)
		printblock(bp);
	if (GET_SIZE(HDRP(bp)) != 0 || !GET_ALLOC(HDRP(bp)))
		printf("Bad epilogue header\n");



    /* Print the stats of every free block in the free list */
    for (bp = free_listp; GET_ALLOC(HDRP(bp))==0; bp = NEXT_FREEP(bp))
    {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }

	/*Checks if every block in free list is marked free */
	for (bp = free_listp; bp != NULL; bp = NEXT_FREEP(bp))
    {
        if (GET_ALLOC(HDRP(bp))==1)
            printf("Allocation Status of free block is wrong\n");
    }

	/*Checks if all free blocks are in free list*/
	

	for (bp = free_listp; GET_ALLOC(HDRP(bp))==0; bp = NEXT_FREEP(bp))
    {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }

		
	/* CHecks for overlapping allocated blocks*/
	
	for (bp = heap_listp; NEXT_BLKP(bp) != NULL; bp = NEXT_BLKP(bp))
    {
	if(GET_ALLOC(HDRP(bp)) == 1 && GET_ALLOC(HDRP(curr)) == 1){     
		if(HDRP(curr) < (void *)(FTRP(bp) + WSIZE))			//If both allocated and not overlapping
			curr = bp;
		else 
			printf("Allocated blocks are overlapping\n");  //If both allocated and overlapping
	}	        
    }

	/* Checks if free block pointers point to valid free blocks */
	for (bp = free_listp; bp != NULL; bp = NEXT_FREEP(bp))
    {
        if((void *)bp < HDRP(bp) && (void *)bp > FTRP(bp))
		printf("Pointer points to invalid block\n");
    }

		

	/* CHecks if adjacent free blocks have not been coalesced */
	for (bp = free_listp; bp != NULL; bp = NEXT_FREEP(bp) ) free++;
	for (bp1 = heap_listp; bp1 != NULL; bp1 = NEXT_BLKP(bp1) ){
    
        if (GET_ALLOC(HDRP(bp1)) == 0)
		 heap++;            
    }
	if(heap != free) 
		printf("Not all free blocks are in the free list\n");

	
   
}

/*
 * printblock - Prints the details of a block in the list.
 * This function displays previous and next pointers if the block
 * is marked as free.
 *
 * This function takes a block pointer (to a block for examination) as a
 * parameter.
 */
static void printblock(void *bp)
{
    int hsize, halloc, fsize, falloc;
	size_t h,f;
    /* Basic header and footer information */
    checkheap(false);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));

    if (hsize == 0)
    {
        printf("%p: end of heap\n", bp);
        return;
    }
   
    /* Prints out header and footer info if it's an allocated block.
     * Prints out header and footer info and next and prev info
     * if it's a free block.
    */
	h=hsize;
	f=fsize;
	printf("%p: header:[%zu:%c] footer:[%zu:%c]\n",bp,h,(halloc ? 'a' : 'f'),f,(falloc ? 'a' : 'f'));
}

static void checkblock(void *bp)
{
    /* CHecks if the pointers in heap point to valid addresses */
    if (NEXT_FREEP(bp)< mem_heap_lo() || NEXT_FREEP(bp) > mem_heap_hi())
        printf("Error: next pointer %p is not within heap bounds, points to invalid address \n"
                , NEXT_FREEP(bp));
    if (PREV_FREEP(bp)< mem_heap_lo() || PREV_FREEP(bp) > mem_heap_hi())
        printf("Error: prev pointer %p is not within heap bounds, points to invalid address \n"
                , PREV_FREEP(bp));

    /* Reports if there isn't DSIZE alignment by checking if the block pointer
     * is divisible by DSIZE.
    */
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);

    /* Reports if the header does not match the footer for a free block*/
    if (!GET_ALLOC(bp) && GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}


/*
 * Inserts a block at the front of the explicit free list
 */
static void add(void *bp)
{
    NEXT_FREEP(bp) = free_listp;        //Sets next ptr to start of free list
    PREV_FREEP(free_listp) = bp;        //Sets current's prev to new block
    PREV_FREEP(bp) = NULL;              // Sets prev pointer to NULL
    free_listp = bp;                    // Sets start of free list as new block
}

/*
 * Removes a block from the free list
 * If there's a prev block, set the next pointer of this block 
 * to the next pointer of the current block.
 * If not, set the next block's previous pointer to the prev 
 * of the current block.
 */

static void delete(void *bp)
{
    
    if (PREV_FREEP(bp))
        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);
    else
        free_listp = NEXT_FREEP(bp);
    PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
}

/*
 * The last lines of this file configures the behavior of the "Tab" key in
 * emacs.  Emacs has a rudimentary understanding of C syntax and style.  In
 * particular, depressing the "Tab" key once at the start of a new line will
 * insert as many tabs and/or spaces as are needed for proper indentation.
 */

/* Local Variables: */
/* mode: c */
/* c-default-style: "bsd" */
/* c-basic-offset: 8 */
/* c-continued-statement-offset: 4 */
/* indent-tabs-mode: t */
/* End: */
