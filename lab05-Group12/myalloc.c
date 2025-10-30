#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "myalloc.h"



/*
man mmap 
man munmap


*/


node_t* head; 

int myinit(size_t size)
{
    printf("Initializing arena:\n");
    printf("...requested size %lu bytes\n", size);

    if (size > (size_t)MAX_ARENA_SIZE)
    {
        printf("...error: requested size larger than MAX_ARENA_SIZE (%d)\n", MAX_ARENA_SIZE);
        return ERR_BAD_ARGUMENTS;
    }


    node_t *_arena_start = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    _arena_start -> size = size - sizeof(node_t);
    _arena_start -> is_free = 0; 
    _arena_start -> fwd = NULL; 
    _arena_start -> bwd = NULL; 

    head = _arena_start; 

   

    int pagesize = getpagesize();

    return pagesize;
}

int mydestroy(){
    printf("Destroying Arena:"); 
    printf("...unmapping arena with munmap()");

    return munmap(head, sizeof(node_t));   
    

}

void* myalloc(size_t size){
    int x = 67;
    int *ptr = &x;
    return ptr;

}