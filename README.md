# classic-codes
DIY MALLOC ALLOCATOR
Team Name: April Coders
Members:
1. Malvika Singh      
2. Nandini Nerurkar    

Introduction

We have implemented the DIY malloc allocator using explicit free lists.  While using implicit free lists, the allocator traverses the entire heap, checking blocks regardless of whether they are allocated or free, in order to find a block of the size requested in the call to malloc.  This increases the throughput of the code, which is the average number of operations the code performs per second.  In explicit free lists, the allocator keeps track of only the free blocks and adds and deletes them as and when blocks are freed or allocated.  The allocator thus, only checks free blocks whenever malloc is called and increases throughput.
There are 3 find fit algorithms that we have studied in class - first fit, next fit and best fit.  In our implementation, we have used the first fit algorithm for searching blocks of at least size bytes, where size is the parameter passed to the malloc function.  On finding a suitable block, we calculated the difference between the size of the block found and the requested size.  If this difference was enough to define a free block then after allocation, the block was split and the newly formed free block was added to the free list.  We defined a minimum size for a free block as 6 times the word size and compared the above difference with this value.  The first fit algorithm creates less fragmentation than the next fit algorithm.  The best fit algorithm is slower than the former algorithms and takes more steps for implementation since it has to traverse the entire free list to  search a block.


Implementation:

Explicit free list is used by us in implementing the allocator. It is due to the advantages mentioned below.
Keeping track of free blocks becomes a cumbersome process in implementing a memory allocator. In order to deal with this, we have two models of implementation:
1. Implicit free list
2. Explicit free list
The  advantage of using implicit free list is that it is simple to implement.The disadvantage is that implicit keeps track of both the free as well as allocated blocks while the explicit free list keeps track of only the free blocks and not all the blocks.
One of the challenges was that the next free block could be located anywhere in the heap and was hence difficult to keep track of. So, we need to store the forward and backward pointers and not just the sizes. We need boundary tags for coalescing and we are tracking only the free blocks, so we can use the payload area.
Explicit free lists are logically doubly linked list, but in reality, the blocks can be located in any order.
So, we have achieved more efficiency as compared to implicit free lists due to the following advantages:
1. Allocation is in the linear order time, as it considers only the free blocks instead of all the blocks.
2. It works in a much  faster  way when  most  of  the  memory  space is  full.  
Also, we have used first fit strategy and not next fit, to increase our utilization, because even though the first fit may cause splinters at the beginning of the list, but it is still better than the next fit. Research proves that next fit has the worst issues of fragmentation.
The allocator efficiency is further expected to increase if we use best fit strategy as it will increase the storage utilization. But we have used the first fit as the best fit is the slowest algorithm amongst all the fits and will decrease the throughput. 


The summary of the implementation of our code is given as follows:


mm_init function:
The mem_sbrk function allocates 4 bytes for constructing the initial heap and returns the pointer to the beginning of the heap.  The commands that follow define the initial heap structure as:
| Prologue | Header | Previous |  Next |               |                | Footer  | Epilogue |
Then the free list pointer is set so that it points to the payload of the heap, that is the byte right after the “Next” pointer, since initially the entire heap is a free block.
The heap is then extended.
The structure of a free block is as follows:
| Header | Previous |  Next | ---- Variable number of free bytes ----| Footer  | 


coalesce function:
In the coalesce function we have checked for 3 cases:
1. Case wherein the previous block is free and next is allocated
2. The previous block is allocated and next is free
3. Both previous and next blocks are free
The old pointer is modified in this function.  If all of the above cases fail, that is adjacent blocks are allocated, then the old pointer remains unmodified and the function returns the old pointer.  We have not explicitly checked for this condition in order to reduce the number of steps required to execute the function.  
The adjacent free blocks are deleted from the free list, the old pointer is shifted to the previous block in cases 1 and 3, and header of this pointer is updated to the size of the newly formed block.  Then the footer is updated so that it matches the header of the free block.


mm_realloc function:
This function extends the block but does not shrink it. Thus, it preserves the old data in the block which was previously present. If the requested size is less than the original size, then we return the original pointer.
 If the requested size is greater than the original size, we check its previous and next blocks, and if free, we add the sizes of respective adjacent free blocks and return the pointer. This function takes a block pointer and the new size as parameters and returns  pointer to the payload of the newly allocated block.


mm_free:
This function takes a pointer of type void* , which is the pointer to the block to be freed. In this case, we make corresponding changes in the allocated bit of the header and footer of the freed block and also check if the adjacent block is free or not. If free, we coalesce the two free blocks and subsequently add them to the free list.


mm_malloc:
The mm_malloc function allocates a block at least “size” ( i.e requested) bytes of payload, unless size is zero. It allocates by incrementing the brk pointer. This function returns the address of the block if successful and NULL otherwise.
In addition to this, extra care has to be taken to always allocate a block whose size is a multiple of the alignment. The free list is searched in order to get the first free block using first fit method. If the requested space is present in the memory, then we set its corresponding allocation status, else we implement the functionality of extend heap. Finally, we return the payload pointer if the call to malloc is successful.


add function:
A newly freed block is added to the free list in this function.  The current block is inserted at the start of the list by assigning its next pointer to the free list head, previous of the head block to the current block and then shifting the free list head so that it points at the current block.
delete function:
The delete function removes a newly allocated block from the free list.  The function checks whether the current block is preceded by another block in the free list.  If it has a previous block then it changes the pointers of this previous block and the next block.  Otherwise, if it is the head of the list then the free list head is made to point at the next block and the previous of the next block is set to previous of the current block.


Modifications in the checker routines:

 We have also modified the checkblock function which checks whether the next and previous pointers are within the bounds of the heap. It also checks the alignment of the block and whether the contents of the header and footer of a free block are the same.

The checkheap routine checks if every block in free list is marked free by traversing the free list ensuring that the allocated bit in the header is 0. It also checks if all free blocks are in free list by comparing number of free blocks in the free list and the heap. In addition to this it checks for overlapping allocated blocks. We have used 2 pointers - one points to an allocated block and the other points to the next allocated block (not necessarily adjacent block) Then we check by comparing the addresses of the header of one pointer and footer of the other block.  Overlapping happens when if header of next allocated block lies before the footer of the previously allocated block. 
It also checks if adjacent free blocks have not been coalesced. This has been implemented by traversing the heap and identifying 2 consecutive free blocks. 




