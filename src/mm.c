#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#define WSIZE 8
#define DSIZE 16
/* When requesting memory from the OS using sbrk(), request it in
 * increments of CHUNK_SIZE. */


#define CHUNK_SIZE (1<<12)

static int largest = -1; 


 void heap_validator() ;           

struct free_node{
    size_t header;
    struct free_node *next;
}typedef free_node;

//static bool called_malloc;

static free_node *free_list[13];
/*
 * This function, defined in bulk.c, allocates a contiguous memory
 * region of at least size bytes.  It MAY NOT BE USED as the allocator
 * for pool-allocated regions.  Memory allocated using bulk_alloc()
 * must be freed by bulk_free().
 *
 * This function will return NULL on failure.
 */
extern void *bulk_alloc(size_t size);

/*
 * This function is also defined in bulk.c, and it frees an allocation
 * created with bulk_alloc().  Note that the pointer passed to this
 * function MUST have been returned by bulk_alloc(), and the size MUST
 * be the same as the size passed to bulk_alloc() when that memory was
 * allocated.  Any other usage is likely to fail, and may crash your
 * program.
 */
extern void bulk_free(void *ptr, size_t size);

void free(void *ptr);

/*
 * This function computes the log base 2 of the allocation block size
 * for a given allocation.  To find the allocation block size from the
 * result of this function, use 1 << block_size(x).
 *
 * Note that its results are NOT meaningful for any
 * size > 4088!
 *
 * You do NOT need to understand how this function works.  If you are
 * curious, see the gcc info page and search for __builtin_clz; it
 * basically counts the number of leading binary zeroes in the value
 * passed as its argument.
 */
static inline __attribute__((unused)) int block_index(size_t x) {
    if (x <= 8) {
        return 5;
    } else {
        return 32 - __builtin_clz((unsigned int)x + 7);
    }
}

/*
 * You must implement malloc().  Your implementation of malloc() must be
 * the multi-pool allocator described in the project handout.
 */
void *malloc(size_t size) {

    if (size == 0 ){
        return NULL;
    }
    // size of log2(size) (y in the pdf) 
//    fprintf(stderr,"alloc %zu\n",size );
  //
    if (size > (CHUNK_SIZE - sizeof(size_t))){
        //add initing memory  
        void *ptr = bulk_alloc(size + sizeof(size_t)); 
       // adding header  
        *(size_t*)ptr = size;

        ptr += sizeof(size_t);
       
    //    fprintf(stderr, "bulk malloc %zu block\n",size  );

        return ptr;
    }
    
    int counter = block_index(size);
    // does 2 to the counter power (x value in the pdf)  
    
    // amount of blocks needed to break up memory 
    
    

    if( free_list[largest] != NULL && largest > counter ){
        counter = largest;
        
    }
    else if (largest <counter ){
        largest = counter ; 

    }
    
 //   fprintf(stderr,"%i counter\n", counter  ) ;
    int block_size = 1<<counter; 
    
    int block_amount = CHUNK_SIZE / block_size; 

    if ( free_list[counter] == NULL  ){
                         
        // gives the amount blocks that the memory needs to be sets up into
        void *memory = sbrk(CHUNK_SIZE); 
        
// sets the head by casting the begining of the memory as a free_node
        free_list[counter] = memory;
        // keeps track of node being changed 
        free_node *node = free_list[counter];
        
 //     fprintf(stderr," free_list init %i \n", counter  );

        for(int i = 0; i < block_amount ; i++  ){
            //goes up block_size - 8 
            //inits nodes position on the stack based on the place         
//            free_node *new_node = (free_node * )   ;

            //set the next as the allocated memory chunk
            node->next = memory + (block_size * i) ; 
            // sets the allocated 
            node->header = block_size;
            
        //goes to next node 
            
            node = node->next; 

        }
        node->header = block_size;
        
        node->next = NULL;

    }
     
    free_node *allocated_node = free_list[counter];
    //marking node as allocated  
    allocated_node->header += 1; 
    
    //setting return value 
    
//  fprintf(stderr, "malloc %zu block\n",allocated_node->header  );
    //removing from free_list 
    free_list[counter] = free_list[counter]->next;

   // heap_validator();
    //returns previos node 
//    fprintf(stderr,"value(%s)\n",(char*)(allocated_node +sizeof(size_t)));
    return (void*)allocated_node +sizeof(size_t) ; 
}

/*
 * You must also implement calloc().  It should create allocations
 * compatible with those created by malloc().  In particular, any
 * allocations of a total size <= 4088 bytes must be pool allocated,
 * while larger allocations must use the bulk allocator.
 *
 * calloc() (see man 3 calloc) returns a cleared allocation large enough
 * to hold nmemb elements of size size.  It is cleared by setting every
 * byte of the allocation to 0.  You should use the function memset()
 * for this (see man 3 memset).
 */
void *calloc(size_t nmemb, size_t size) {

 // fprintf(stderr,"calloc %zu  block\n",size );
  //  fprintf(stderr,"C" );
    if (nmemb * size <= CHUNK_SIZE ){
        // mallocing size 
        void *ptr  = malloc(nmemb * size );
        //setting all bytes to size to 0
        memset(ptr, 0, nmemb * size);
     //   heap_validator();
        return ptr;
    }
    else {
        //bulk allocs size times nmem times size 
        void *ptr = bulk_alloc(nmemb * size);
        //sets all the values to zore
        memset(ptr, 0, nmemb * size);
        
        return ptr;
    }
}

/*
 * You must also implement realloc().  It should create allocations
 * compatible with those created by malloc(), honoring the pool
 * alocation and bulk allocation rules.  It must move data from the
 * previously-allocated block to the newly-allocated block if it cannot
 * resize the given block directly.  See man 3 realloc for more
 * information on what this means.
 *
 * It is not possible to implement realloc() using bulk_alloc() without
 * additional metadata, so the given code is NOT a working
 * implementation!
 */

void *realloc(void *ptr, size_t size) {
 //  fprintf(stderr,"RE");   
    
//    fprintf(stderr,"realloc %zu  block\n",size );

    if (ptr == NULL) { 
        return malloc(size); 
    }

    if (size == 0 ){
        free(ptr);
        return ptr;
    }

    //casting free_node from ptr 

    free_node *og_node = (free_node*)(ptr - sizeof(size_t));
    //    fprintf(stderr,"yo\n");
    //    fprintf(stderr,"yo(%zu)\n",og_node->header);
    //size of ptr 
    

    size_t og_size = og_node->header - 1;

    if ( size <= og_node->header ){
        //return if the size the exist pointer is smaller or equil to size param

        return ptr; 
        
    }
    else{

        if (size <= CHUNK_SIZE){

            //alloces new the memory of size 
            void* new_node = malloc(size);  

            //copies data from the ptr to the new node
            memcpy(new_node, ptr, og_node->header-1);
                
            free(ptr);
            return new_node;
        }
        else {

            //aboves a 
            void *new_node = bulk_alloc(size) ;
            memcpy(new_node, ptr,og_size);     
            free( ptr ) ;
            return new_node;
        }
    }
    //printf("poo\n" );
     //   fprintf(stderr, "Realloc is not implemented!\n");
    return NULL;
}

/*
 * You should implement a free() that can successfully free a region of
 * memory allocated by any of the above allocation routines, whether it
 * is a pool- or bulk-allocated region.
 *
 * The given implementation does nothing.
 */
void free(void *ptr) {
    //casting pointer on
//    fprintf(stderr ,"free()\n") ;

    if (ptr == NULL){
        return;
    }
     void *size_block = (ptr-sizeof(size_t));
      
 //   heap_validator();

  //  fprintf(stderr,"FREE%p--------------------------------- \n",(void*)size_block );
    
     //check if is allocated by check its divisable by 2  
     if  (  *(size_t*)size_block % 2 != 0 ){
        //remove allocator mark (+1) 
        
     //   fprintf(stderr,"%zu header\n", *size_block ) ;
        size_t  block_val = block_index(*(size_t*)size_block-1-sizeof(size_t));  

        //fprintf(stderr ,"%zu free block\n", block_val ) ;
        // if it was bulk allocated it will be bulk freed 
        if(block_val > CHUNK_SIZE){
 //           fprintf(stderr,"%zu bulk free amount \n",*(size_t*)size_block);
            *(size_t*)size_block -= 1;
 
            bulk_free(ptr,*(size_t*)size_block + sizeof(size_t) );
            return;
        }
        // adds the node give to the free list 
          
        free_node *value = (free_node*)(ptr-sizeof(size_t)); 

        value->header -= 1;
        
        value->next = free_list[block_val]; 
        free_list[block_val] = value ;   
                 
 //       heap_validator(); 
     } 
     
    return;
}
//step threw malloc to find the header val is 65
//
//
size_t test(void *ptr){
    free_node *ptr_node = (free_node*)ptr-sizeof(size_t);  
 //   printf("%zu",ptr_node->header);
    return ptr_node->header;
}


void heap_validator(){
    
    for (int j = 0; j <13; j++  ){
    free_node *cur = free_list[j];  
    int i = 0;
    size_t header_val  = 0;
    if ( free_list[j] != NULL ){
        header_val = free_list[j]->header;
    }
    for(  i = 0;  cur != NULL; i++ ){
         if (header_val != cur->header  ){
            fprintf(stderr,"wrong header %zu index: %i \n", cur->header,i );
        }
        cur = cur->next;
       
    }
     
    fprintf( stderr, "%iblocks  %iindex %zu header \n", i, j,header_val);
    }
}

