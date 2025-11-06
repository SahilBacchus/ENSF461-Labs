#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "myalloc.h"

int statusno = 0;

// Global variables to track arena state
static void* arena_start = NULL;
static size_t arena_size = 0;
static node_t* head = NULL;




int myinit(size_t size) {
    printf("Initializing arena:\n");
    printf("...requested size %lu bytes\n", size);

    if (size <= 0 || size > (size_t)MAX_ARENA_SIZE) {
        statusno = ERR_BAD_ARGUMENTS;
        return ERR_BAD_ARGUMENTS;
    }

    int page_size = getpagesize();
    printf("...pagesize is %d bytes\n", page_size);
    
    // Adjust size to be multiple of page size
    size_t adjusted_size = ((size + page_size - 1) / page_size) * page_size;
    printf("...adjusting size with page boundaries\n");
    printf("...adjusted size is %lu bytes\n", adjusted_size);


    printf("...mapping arena with mmap()\n");
    arena_start = mmap(NULL, adjusted_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    

    if (arena_start == MAP_FAILED) { // If mmap fails it returns -1
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }

    arena_size = adjusted_size;
    
    // Initialize the first chunk header
    head = (node_t*)arena_start;
    head->size = adjusted_size - sizeof(node_t);
    head->is_free = 1;
    head->fwd = NULL;
    head->bwd = NULL;

    printf("...arena starts at %p\n", arena_start);
    printf("...arena ends at %p\n", (char*)arena_start + adjusted_size);


    return adjusted_size;


}





int mydestroy() {
    printf("Destroying Arena:\n");
    
    if (arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return ERR_UNINITIALIZED;
    }

    printf("...unmapping arena with munmap()\n");
    int result = munmap(arena_start, arena_size);
    
    // Reset global state
    arena_start = NULL;
    arena_size = 0;
    head = NULL;
    statusno = 0;

    if (result != 0) {
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }


    return 0;

}




void* myalloc(size_t size) {
    if (arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }
    if (size <= 0) {
        statusno = ERR_BAD_ARGUMENTS;
        return NULL;
    }

    printf("Allocating memory:\n");
    printf("...looking for free chunk of >= %lu bytes\n", size);

    // Search for first fit --> looks for first free chunk that is large enough
    node_t* temp = head;
    node_t* found = NULL;
    while (temp != NULL) {
        if (temp->is_free && temp->size >= size) {
            found = temp;
            break;
        }
        temp = temp->fwd;
    }

    if (found == NULL) {
        printf("...no suitable free chunk found\n");
        statusno = ERR_OUT_OF_MEMORY;
        return NULL;
    }


    printf("...found free chunk of %lu bytes with header at %p\n", found->size, found);
    printf("...free chunk->fwd currently points to %p\n", found->fwd);
    printf("...free chunk->bwd currently points to %p\n", found->bwd);


    // Check if we need to split  --> occurs when chunk is larger than needed
    printf("...checking if splitting is required\n");
    size_t remaining_size = found->size - size;
    if (remaining_size > sizeof(node_t)) {
        printf("...splitting is required\n");

        // Create new chunk from remaining space
        node_t* new_chunk = (node_t*) ((char*)(found + 1) + size);
        new_chunk->size = remaining_size - sizeof(node_t);
        new_chunk->is_free = 1;
        
        // Update links
        new_chunk->fwd = found->fwd;
        new_chunk->bwd = found;
        
        if (found->fwd != NULL) {
            found->fwd->bwd = new_chunk;
        }
        
        found->fwd = new_chunk;
        found->size = size;
    }else {
        printf("...splitting not required\n");
    }


    // Mark as allocated
    found->is_free = 0;
    printf("...updating chunk header at %p\n", found);
    printf("...being careful with my pointer arithmetic and void pointer casting\n");

    // Return pointer to data area (after header)
    void* alloc_ptr = (void*) (found + 1);
    printf("...allocation starts at %p\n", alloc_ptr);

    return alloc_ptr;


}






void myfree(void* ptr) {
    if (arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return;
    }
    if (ptr == NULL) {
        statusno = ERR_BAD_ARGUMENTS;
        return;
    }

    printf("Freeing allocated memory:\n");
    printf("...supplied pointer %p:\n", ptr);
    printf("...being careful with my pointer arithmetic and void pointer casting\n");

    // Get header from data pointer
    node_t* header = (node_t*) ptr - 1;
    printf("...accessing chunk header at %p\n", header);
    printf("...chunk of size %lu\n", header->size);

    // Mark as free
    header->is_free = 1;


    //
    // Coalescing merges adjacent free chunks

    // Coalesce with next chunk if free and adjacent
    printf("...checking if coalescing is needed\n");
    if (header->fwd != NULL && header->fwd->is_free) {
        node_t* next = header->fwd;
        
        // Check if adjacent
        char* current_chunk_end = (char*) header + sizeof(node_t) + header->size;
        char* next_chunk_start = (char*) next;
        if (current_chunk_end == next_chunk_start) {
            printf("...coalescing with next chunk\n");
            header->size += sizeof(node_t) + next->size;
            header->fwd = next->fwd;
            
            if (next->fwd != NULL) {
                next->fwd->bwd = header;
            }
        }
    }


    // Coalesce with previous chunk if free and adjacent
    if (header->bwd != NULL && header->bwd->is_free) {
        node_t* prev = header->bwd;
        
        // Check if adjacent
        char* prev_chunk_end = (char*)prev + sizeof(node_t) + prev->size;
        char* current_chunk_start = (char*) header;
        if (prev_chunk_end == current_chunk_start) {
            printf("...coalescing with previous chunk\n");
            prev->size += sizeof(node_t) + header->size;
            prev->fwd = header->fwd;
            
            if (header->fwd != NULL) {
                header->fwd->bwd = prev;
            }
        }
    } 
    else {
        printf("...coalescing not needed\n");
    }



}