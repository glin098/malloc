#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

struct node{
	struct node *next;
	int size;
	int free;
	void *ptr;
};

struct node *head = NULL;

// check if there is a free block that is large enough
struct node *find_free_mem(struct node **last, int size){
	
	struct node* current = head;
	
	if(!(current->free) || current->size < size){
		*last = current;	
		current = current->next;
	}
	
	else if(current->free){
		if(current->size >= size){
			// divide mem
			int newsize = current->size - size - sizeof(struct node);
				if (newsize > 8){
					struct node *newNode = (struct node*)(current->ptr + size);
					newNode->next = current->next;
					newNode->free = 0;
					newNode->size = newsize;
					newNode->ptr = current->ptr + size + sizeof(struct node);
					
					current->next = newNode;
					current->size = size;
				}
		}
	}
	
	return current;
}

// if no free block, request space with sbrk
struct node *request_mem(struct node *last, int size){
	struct node* newNode;
	
	int pageSize = sysconf(_SC_PAGESIZE);
	
	newNode = sbrk(0);
	
	void *ret = sbrk(size + sizeof(struct node));	
	
	if(ret == (void*) - 1){ // check if sbrk works
		errno = ENOMEM;

		return NULL;
	}
	
	if(last){
		last->next = newNode;
	}	
	
	newNode->size = size;
	newNode->next = NULL;
	newNode->free = 0;
	// newNode->ptr = ptr + sizeof(struct node);
	
	
	if(pageSize > size + sizeof(struct node)){
		int newsize = newNode->size - size - sizeof(struct node);
                                if (newsize > 8){
                                        struct node *new = (struct node*)(newNode->ptr + size);
                                        new->next = newNode->next;
                                        new->free = 0;
                                        new->size = newsize;
                                        new->ptr = newNode->ptr + size +sizeof(struct node);
                                        newNode->next = new;
                                        newNode->size = size;
                                }                                       
	
	}

	return newNode;
}


void *malloc(size_t size){
	struct node* newNode;
	
	if (size <= 0){
                return NULL;
        }

	if(!head){ // first time set-up
		newNode = request_mem(NULL, size);
		
		if(!newNode){
			return NULL;
		}
		
		head = newNode;
	}
	else{
		struct node *last = head;
		newNode = find_free_mem(&last, size);

		if(!newNode){ // if free space not found
			newNode = request_mem(last, size);
			if(!newNode){
				return NULL;
			}
		}
		else{ //if find free space
			newNode->free = 0;
		}

	}

	return (newNode+1);
}

void free(void *ptr){
	struct node *current = head;

	if(current != NULL){
		if(current->ptr == ptr){
			current->free = 1;
			return;
		}
		current = current->next;
	}
	return;
}

// get address of node ptr
struct node *getptr(void* ptr){
	return (struct node*)ptr - 1;
}

// calloc function should call malloc and then call memset to clear the 
// newly allocated block
// clears mem before returning ptr
void *calloc(size_t num_of_elts, size_t elt_size){
	int size = num_of_elts * elt_size;
	void *ptr = malloc(size);		
	
	if(ptr != NULL){	
		memset(ptr, 0, size);
		return ptr;
	}
	return NULL;
}


// call malloc to make a new block, memcpy to move old information into new 
// block and free the old 
// block can be resized larger or smaller 
void *realloc( void *ptr, size_t size){
	if(ptr == NULL){
		return malloc(size);
	}

	struct node* nodeptr = getptr(ptr);

	if(nodeptr->size >= size){
		return ptr;
	}
	else if(nodeptr->size < size) { //malloc new space
		void* newptr;
		newptr = malloc(size);
		if(newptr == NULL){
			errno = ENOMEM;
			return NULL;
		}

		memcpy(newptr, ptr, nodeptr->size);
		//free old
		free(ptr);
	
		return newptr;

	}

	return NULL;
	
}

// to compile gcc simple.c bad_malloc.c -o simple_bad

        // in class notes
        //
        // malloc(1000)
        // 
        // ret = sbrk(page_size)
        // struct node* newNode = ret;
        // newNode ->size = 1000
        // newNode ->free = 0
        // newNode -> start = newNode + 1; (start of the memory region)
        // OR               = ((void*) newNode) + sizeof(struct Node)
        // newNode -> next = (void* newNode + 1000 + sizeof(struct Node)
        // struct node* next = newNode -> next;
        // next -> free = 1
        // next -> start = next + 1
        // next -> size  = page_size - 2*sizeof(struct Node) - 1000
        // next-> next = NULL;
