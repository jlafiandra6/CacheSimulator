#include "cachesim.hpp"
#include <map>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <iostream>

#define TRUE 1
#define FALSE 0
/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c1 The total number of bytes for data storage in L1 is 2^c
 * @b1 The size of L1's blocks in bytes: 2^b-byte blocks.
 * @s1 The number of blocks in each set of L1: 2^s blocks per set.
 */
 typedef struct block {
    uint64_t tag;  // The tag stored in that block
    uint8_t valid; // Valid bit
    uint8_t dirty; // Dirty bit
    uint64_t LRUage; //most recently used counter

} block_t;

 typedef struct prediction {
    uint64_t address;  // The address of a block
    uint64_t likelihood; //times seen that it follows

} prediction_t;

 typedef struct markovrow {
    uint64_t address;  // The address of a block
    uint64_t last; // holds the last seen miss always stoored at markovrow[0]
    uint8_t valid; //valid bit
    prediction_t predictions[4]; //most recently used counter
    uint64_t LRUage; //LRU age
} markovrow_t;

typedef struct config {
    uint64_t C;
    uint64_t B;
    uint64_t S;
    uint64_t P;
    uint64_t pt;
} config_t;

config_t* prefetcherconfig1;
block_t* prefetcher1;
markovrow_t* markov_table;
int date = 0; //lru timestamp for certain things
config_t* cacheconfig1;
block_t* cache1;

void setup_cache(uint64_t c1, uint64_t b1, uint64_t s1, uint64_t p1, uint64_t prefetcher_type) {

    uint64_t num_block =(uint64_t) (((uint64_t)1)<<c1)/(((uint64_t)1)<<b1);
    
    block_t* cache = (block_t*) calloc(num_block, sizeof(block_t));
    
    block_t* prefetcher = (block_t*) calloc(32, sizeof(block_t));
     
    for(uint64_t i = 0; i < num_block; i++) {
        cache[i].tag = 0;
        cache[i].dirty = 0;
        cache[i].valid = 0;
        cache[i].LRUage = 0;
    }
    
    
    config_t* prefetcherconfig = (config_t*) calloc(1,sizeof(config_t));
	prefetcherconfig->C = 5 + b1;
	prefetcherconfig->B = b1;
	prefetcherconfig->S = prefetcherconfig->C-prefetcherconfig->B;
	prefetcherconfig->P = 0;
	prefetcherconfig->pt = 0;
	
	
	
	
	
	
    config_t* cacheconfig = (config_t*) calloc(1,sizeof(config_t));
	cacheconfig->C = c1;
	cacheconfig->B = b1;
	cacheconfig->S = s1;
    cacheconfig->P = p1;	
	cacheconfig->pt = prefetcher_type;
	
	if(prefetcher_type == 1 || prefetcher_type == 3){
	    markov_table = (markovrow_t*) calloc(p1,sizeof(markovrow_t));
	
	
	}
	
	
	
	prefetcherconfig1 = prefetcherconfig;
    prefetcher1 = prefetcher;
    
	cacheconfig1 = cacheconfig;
	cache1 = cache;



}

/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @type The type of event, can be READ or WRITE.
 * @arg  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(char type, uint64_t arg, cache_stats_t* p_stats) {

    p_stats -> accesses = p_stats -> accesses + 1;
    uint8_t is_hit = FALSE;
    uint64_t tag =  get_tag(arg, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
    uint64_t index =  get_index(arg, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
    index = index * (((uint64_t)1)<<cacheconfig1->S);
    date++;
    
    for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
        if(cache1[index + i].valid == 1){
           cache1[index+ i].LRUage = cache1[index+ i].LRUage + 1;
        }
    }
    
    
    
    for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
    
        if(cache1[index + i].tag == tag && cache1[index + i].valid == 1){
            if(type == 'w'){
                cache1[index + i].dirty = 1;  
            }
            is_hit = TRUE;   
            cache1[index+i].LRUage = 0;
        } 
        
    }
    
    
    
    //READ
    if(type == 'r'){
       p_stats -> reads = p_stats -> reads + 1;
       if(is_hit == TRUE) {
            printf("H\n");
            p_stats -> read_hits_l1 = p_stats -> read_hits_l1 + 1;
            p_stats -> total_hits_l1 = p_stats -> total_hits_l1 + 1;
           return;
       }
       
       if(cacheconfig1->pt == 0){
       printf("M\n");
       }
       
       p_stats -> read_misses_l1 = p_stats -> read_misses_l1 + 1;
       p_stats -> total_misses_l1 = p_stats -> total_misses_l1 + 1;
       
       
       if(cacheconfig1->pt != 0){
            
            uint64_t tag2 =  get_tag(arg, prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
            
            for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
    
                if(prefetcher1[i].tag == tag2 && prefetcher1[i].valid == 1){ 
                    printf("PH\n");
                    is_hit = TRUE;   
                    prefetcher1[i].LRUage = 0;
                    prefetcher1[i].tag = 0;
                    prefetcher1[i].valid = 0;
                    prefetcher1[i].dirty = 0;
                    p_stats -> prefetch_hits = p_stats -> prefetch_hits + 1;
                } 
        
            }         
            if(is_hit == FALSE){
                printf("M\n");
                p_stats -> prefetch_misses = p_stats -> prefetch_misses + 1;
            }
            
            
             if(cacheconfig1->pt == 3 && is_hit == FALSE){
                prediction_t pred;
                pred.likelihood = 0;
                
                for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                    if(markov_table[i].address == (arg >> cacheconfig1->B << cacheconfig1->B) && markov_table[i].valid == 1){
                        uint64_t max = markov_table[i].predictions[0].likelihood;
                        pred = markov_table[i].predictions[0];
                        for(int x = 1; x < 4; x++){
                            if(max < markov_table[i].predictions[x].likelihood){
                                max = markov_table[i].predictions[x].likelihood;
                                pred = markov_table[i].predictions[x];
                            } else if(max == markov_table[i].predictions[x].likelihood){
                                if(markov_table[i].predictions[x].address > pred.address){
                                    max = markov_table[i].predictions[x].likelihood;
                                    pred = markov_table[i].predictions[x];
                                }    
                            }
                        }
               
                    
                    }// now that u have found the prediction
              
                } 
                
                uint8_t pre1 = TRUE;
                uint8_t n1 = FALSE;
                
                if (pred.likelihood == 0){
                    n1 = TRUE;
                }
                
                if(pred.address == arg && n1 == FALSE){
                    pre1 = FALSE;
                }//c=x
            
                uint64_t taggy;
                if(n1 == TRUE){
                taggy =  get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                
                }else{            
                taggy =  get_tag(pred.address, prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                }
                
                for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                    //already in prefetcher
                    if(prefetcher1[i].tag == taggy && prefetcher1[i].valid == 1){
                        pre1 = FALSE;
                    }
                } 
            
                uint64_t tagg4;
                if(n1 == TRUE){
                tagg4 = get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                }else{            
                tagg4 = get_tag(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                }
                uint64_t indexx2;
                if(n1 == TRUE){
                indexx2 = get_index(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                }else{            
                indexx2 =get_index(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                }
                 
                
                indexx2 = indexx2 * (((uint64_t)1)<<cacheconfig1->S);

                for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
                    //already in cache
                    if(cache1[indexx2 + i].tag == tagg4 && cache1[indexx2 + i].valid == 1){
                        pre1 = FALSE;
                    }
        
                }
            
                if(pre1 == TRUE){
                    p_stats -> prefetch_issued = p_stats -> prefetch_issued + 1;
                    uint8_t done = FALSE;
                     for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                        if(prefetcher1[i].valid == 1){
                            prefetcher1[i].LRUage += 1;
                        }
                        if(prefetcher1[i].valid == 0 && done ==FALSE){
                            prefetcher1[i].tag = taggy;
                            prefetcher1[i].valid = 1;
                            prefetcher1[i].dirty = 0;
                            prefetcher1[i].LRUage = 0;
                            done = TRUE;
                        }
                    } 
                    
                    if(done == FALSE){
                        uint64_t counter1 = prefetcher1[0].LRUage;
                        uint64_t spot1 = 0;
                        for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                            if(prefetcher1[i].LRUage > counter1){
                               counter1 = prefetcher1[i].LRUage;
                               spot1 = i;
                             }
                        }       
                        prefetcher1[spot1].tag = taggy;
                        prefetcher1[spot1].LRUage = 0;
                        prefetcher1[spot1].valid = 1;
                        prefetcher1[spot1].dirty = 0;
                    }
                }
                
                
            //next portion of code after prefetching, updating markov stuff
                if(p_stats -> prefetch_misses == 1){
                   markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;
                }else{ 
                    
                    uint8_t inserted = FALSE;
                    for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                        if(markov_table[i].address == markov_table[0].last && markov_table[i].valid == 1){
                        
                             for(int x = 0; x < 4; x++){
                                if(markov_table[i].predictions[x].address == arg >> cacheconfig1->B << cacheconfig1->B && inserted == FALSE){
                                 
                                    markov_table[i].predictions[x].likelihood = markov_table[i].predictions[x].likelihood+1;
                                    markov_table[i].LRUage = date;
                                    inserted = TRUE;
                                }
                             }
                             if(inserted == FALSE){
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood == 0 && inserted == FALSE){
                                        markov_table[i].predictions[x].likelihood = 1;
                                        markov_table[i].predictions[x].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                    }
                                }
                                if(inserted == FALSE){
                                uint64_t min = markov_table[i].predictions[0].likelihood;
                                uint64_t inde = 0;
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood < min && inserted == FALSE){
                                        inde = x;
                                        min = markov_table[i].predictions[x].likelihood;
                                    } else if(markov_table[i].predictions[x].likelihood == min && inserted == FALSE){
                                        if(markov_table[i].predictions[x].address < markov_table[i].predictions[inde].address){
                                             inde = x;
                                             min = markov_table[i].predictions[x].likelihood;
                                        }
                                    
                                    }
                                }
                                
                                        markov_table[i].predictions[inde].likelihood = 1;
                                        markov_table[i].predictions[inde].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                }
                             
                             }
                             
                        
                        } 
                    }
                    if(inserted == FALSE){
                        for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                            if(markov_table[i].valid == 0 && inserted != TRUE){
                                markov_table[i].valid = 1;
                                markov_table[i].predictions[0].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                markov_table[i].predictions[0].likelihood = 1;
                                markov_table[i].address = markov_table[0].last;
                                markov_table[i].LRUage = date;
                                inserted = TRUE;
                            }
                        }
                        
                        if(inserted == FALSE){
                            markovrow_t replacer= markov_table[0];
                            uint64_t rep = 0;
                            for(uint64_t i = 1; i < cacheconfig1->P; i++) {
                                if(markov_table[i].LRUage < replacer.LRUage){
                                    replacer = markov_table[i];
                                    rep = i;
                                }
                            }
                            
                            markov_table[rep].address = markov_table[0].last;
                            markov_table[rep].valid = 1;
                            markov_table[rep].LRUage = date;
                            for(int x = 0; x < 4; x++){
                                markov_table[rep].predictions[x].address = 0;
                                markov_table[rep].predictions[x].likelihood = 0;
                                    
                             }
                             
                            markov_table[rep].predictions[0].address = arg >>cacheconfig1->B << cacheconfig1->B;
                            markov_table[rep].predictions[0].likelihood = 1;
                            
                        
                        }
                    }
                    
                  markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;  
            
                }
            
            }
            
            
            if(cacheconfig1->pt == 1 && is_hit == FALSE){
                prediction_t pred;
                pred.likelihood = 0;
                
                for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                    if(markov_table[i].address == (arg >> cacheconfig1->B << cacheconfig1->B) && markov_table[i].valid == 1){
                        uint64_t max = markov_table[i].predictions[0].likelihood;
                        pred = markov_table[i].predictions[0];
                        for(int x = 1; x < 4; x++){
                            if(max < markov_table[i].predictions[x].likelihood){
                                max = markov_table[i].predictions[x].likelihood;
                                pred = markov_table[i].predictions[x];
                            } else if(max == markov_table[i].predictions[x].likelihood){
                                if(markov_table[i].predictions[x].address > pred.address){
                                    max = markov_table[i].predictions[x].likelihood;
                                    pred = markov_table[i].predictions[x];
                                }    
                            }
                        }
               
                    
                    }// now that u have found the prediction
              
                } 
                
                uint8_t pre1 = TRUE;
                
                if (pred.likelihood == 0){
                    pre1 = FALSE;
                }
                
                if(pred.address == arg){
                    pre1 = FALSE;
                }//c=x
            
                uint64_t taggy =  get_tag(pred.address, prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                
                for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                    //already in prefetcher
                    if(prefetcher1[i].tag == taggy && prefetcher1[i].valid == 1){
                        pre1 = FALSE;
                    }
                } 
            
                uint64_t tagg4 =  get_tag(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                uint64_t indexx2 =  get_index(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                indexx2 = indexx2 * (((uint64_t)1)<<cacheconfig1->S);

                for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
                    //already in cache
                    if(cache1[indexx2 + i].tag == tagg4 && cache1[indexx2 + i].valid == 1){
                        pre1 = FALSE;
                    }
        
                }
            
                if(pre1 == TRUE){
                
                    p_stats -> prefetch_issued = p_stats -> prefetch_issued + 1;
                    uint8_t done = FALSE;
                     for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                        if(prefetcher1[i].valid == 1){
                            prefetcher1[i].LRUage += 1;
                        }
                        if(prefetcher1[i].valid == 0 && done ==FALSE){
                            prefetcher1[i].tag = taggy;
                            prefetcher1[i].valid = 1;
                            prefetcher1[i].dirty = 0;
                            prefetcher1[i].LRUage = 0;
                            done = TRUE;
                        }
                    } 
                    
                    if(done == FALSE){
                        uint64_t counter1 = prefetcher1[0].LRUage;
                        uint64_t spot1 = 0;
                        for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                            if(prefetcher1[i].LRUage > counter1){
                               counter1 = prefetcher1[i].LRUage;
                               spot1 = i;
                             }
                        }       
                        prefetcher1[spot1].tag = taggy;
                        prefetcher1[spot1].LRUage = 0;
                        prefetcher1[spot1].valid = 1;
                        prefetcher1[spot1].dirty = 0;
                    }
                }
                
                
            //next portion of code after prefetching, updating markov stuff
                if(p_stats -> prefetch_misses == 1){
                   markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;
                }else{ 
                    
                    uint8_t inserted = FALSE;
                    for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                        if(markov_table[i].address == markov_table[0].last && markov_table[i].valid == 1){
                        
                             for(int x = 0; x < 4; x++){
                                if(markov_table[i].predictions[x].address == arg >> cacheconfig1->B << cacheconfig1->B && inserted == FALSE){
                                    
                                    markov_table[i].predictions[x].likelihood = markov_table[i].predictions[x].likelihood+1;
                                    markov_table[i].LRUage = date;
                                    inserted = TRUE;
                                }
                             }
                             if(inserted == FALSE){
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood == 0 && inserted == FALSE){
                                        markov_table[i].predictions[x].likelihood = 1;
                                        markov_table[i].predictions[x].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                    }
                                }
                                if(inserted == FALSE){
                                uint64_t min = markov_table[i].predictions[0].likelihood;
                                uint64_t inde = 0;
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood < min && inserted == FALSE){
                                        inde = x;
                                        min = markov_table[i].predictions[x].likelihood;
                                    } else if(markov_table[i].predictions[x].likelihood == min && inserted == FALSE){
                                        if(markov_table[i].predictions[x].address < markov_table[i].predictions[inde].address){
                                             inde = x;
                                             min = markov_table[i].predictions[x].likelihood;
                                        }
                                    
                                    }
                                }
                                
                                        markov_table[i].predictions[inde].likelihood = 1;
                                        markov_table[i].predictions[inde].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                }
                             
                             }
                             
                        
                        } 
                    }
                    if(inserted == FALSE){
                        for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                            if(markov_table[i].valid == 0 && inserted != TRUE){
                                markov_table[i].valid = 1;
                                markov_table[i].predictions[0].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                markov_table[i].predictions[0].likelihood = 1;
                                markov_table[i].address = markov_table[0].last;
                                markov_table[i].LRUage = date;
                                inserted = TRUE;
                            }
                        }
                        
                        if(inserted == FALSE){
                            markovrow_t replacer= markov_table[0];
                            uint64_t rep = 0;
                            for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                                if(markov_table[i].LRUage < replacer.LRUage){
                                    replacer = markov_table[i];
                                    rep = i;
                                }
                            }
                            
                            markov_table[rep].address = markov_table[0].last;
                            markov_table[rep].valid = 1;
                            markov_table[rep].LRUage = date;
                            for(int x = 0; x < 4; x++){
                                markov_table[rep].predictions[x].address = 0;
                                markov_table[rep].predictions[x].likelihood = 0;
                                    
                             }
                             
                            markov_table[rep].predictions[0].address = arg >>cacheconfig1->B << cacheconfig1->B;
                            markov_table[rep].predictions[0].likelihood = 1;
                            
                        
                        }
                    }
                    
                  markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;  
            
                }
            
            }
            
            if(cacheconfig1->pt == 2 && is_hit == FALSE){
                uint8_t pre = TRUE; 
                uint64_t tag3 =  get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                
                for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                    //already in prefetcher
                    if(prefetcher1[i].tag == tag3 && prefetcher1[i].valid == 1){
                        pre = FALSE;
                    }
                } 
                
                uint64_t tag4 =  get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                uint64_t index2 =  get_index(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                index2 = index2 * (((uint64_t)1)<<cacheconfig1->S);

                for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
                    //already in cache
                    if(cache1[index2 + i].tag == tag4 && cache1[index2 + i].valid == 1){
                        pre = FALSE;
                    }
        
                }
                
                if(pre ==TRUE){
                p_stats -> prefetch_issued = p_stats -> prefetch_issued + 1;
                     uint8_t done = FALSE;
                     for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                        if(prefetcher1[i].valid == 1){
                            prefetcher1[i].LRUage += 1;
                        }
                        if(prefetcher1[i].valid == 0 && done ==FALSE){
                            prefetcher1[i].tag = tag3;
                            prefetcher1[i].valid = 1;
                            prefetcher1[i].dirty = 0;
                            prefetcher1[i].LRUage = 0;
                            done = TRUE;
                        }
                    } 
                    
                    if(done == FALSE){
                        uint64_t counter1 = prefetcher1[0].LRUage;
                        uint64_t spot1 = 0;
                        for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                            if(prefetcher1[i].LRUage > counter1){
                               counter1 = prefetcher1[i].LRUage;
                               spot1 = i;
                             }
                        }       
                        prefetcher1[spot1].tag = tag3;
                        prefetcher1[spot1].LRUage = 0;
                        prefetcher1[spot1].valid = 1;
                        prefetcher1[spot1].dirty = 0;
                    }
        
                
                
                
                }
                
                
                
            }
            
       }
       
       
       for(uint64_t i = 0; i < (((uint64_t)1)<<(cacheconfig1->S)); i++) {
            if(cache1[index + i].valid == 0){
                cache1[index+ i].tag = tag;
                cache1[index+ i].valid = 1;
                cache1[index+ i].dirty = 0;
                cache1[index+ i].LRUage = 0;
                return;
            }
       }     
       //first check if anything is empty. If empty, just take that slot.
       
       //if nothing is empty, time to look at replacement
       
       uint64_t counter = cache1[index].LRUage;
        uint64_t spot = 0;
        for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
            if(cache1[index + i].LRUage > counter){
                counter = cache1[index + i].LRUage;
                spot = i;
            }
        }
        //find the least rarely used block
        
        if(cache1[index + spot].dirty == 1){
        //if that block was dirty, write it back to mem
            p_stats -> write_back_l1 = p_stats -> write_back_l1 + 1;
            cache1[index + spot].dirty = 0;
        }
        

        
        //replace that block
        cache1[index + spot].tag = tag;
        cache1[index + spot].LRUage = 0;
        cache1[index + spot].valid = 1;
        return;   
        
      //WRITE
    } else{
        p_stats -> writes = p_stats -> writes + 1;
        if(is_hit == TRUE) {
           printf("H\n");
           p_stats -> write_hits_l1 = p_stats -> write_hits_l1 + 1;
           p_stats -> total_hits_l1 = p_stats -> total_hits_l1 + 1;
           return;
       }
       
       if(cacheconfig1->pt == 0){
       printf("M\n");
       }
       p_stats -> write_misses_l1 = p_stats -> write_misses_l1 + 1;
       p_stats -> total_misses_l1 = p_stats -> total_misses_l1 + 1;
       //miss
      if(cacheconfig1->pt != 0){
            
            uint64_t tag2 =  get_tag(arg, prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
            
            for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
    
                if(prefetcher1[i].tag == tag2 && prefetcher1[i].valid == 1){
                    printf("PH\n"); 
                    is_hit = TRUE;   
                    prefetcher1[i].LRUage = 0;
                    prefetcher1[i].tag = 0;
                    prefetcher1[i].valid = 0;
                    prefetcher1[i].dirty = 0;
                    p_stats -> prefetch_hits = p_stats -> prefetch_hits + 1;
                } 
        
            }         
            
            if(is_hit == FALSE){
                printf("M\n");
                
                p_stats -> prefetch_misses = p_stats -> prefetch_misses + 1;
            }
            
            if(cacheconfig1->pt == 3 && is_hit == FALSE){
                prediction_t pred;
                pred.likelihood = 0;
                
                for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                    if(markov_table[i].address == (arg >> cacheconfig1->B << cacheconfig1->B) && markov_table[i].valid == 1){
                        uint64_t max = markov_table[i].predictions[0].likelihood;
                        pred = markov_table[i].predictions[0];
                        for(int x = 1; x < 4; x++){
                            if(max < markov_table[i].predictions[x].likelihood){
                                max = markov_table[i].predictions[x].likelihood;
                                pred = markov_table[i].predictions[x];
                            } else if(max == markov_table[i].predictions[x].likelihood){
                                if(markov_table[i].predictions[x].address > pred.address){
                                    max = markov_table[i].predictions[x].likelihood;
                                    pred = markov_table[i].predictions[x];
                                }    
                            }
                        }
               
                    
                    }// now that u have found the prediction
              
                } 
                
                uint8_t pre1 = TRUE;
                uint8_t n1 = FALSE;
                
                if (pred.likelihood == 0){
                    n1 = TRUE;
                }
                
                if(pred.address == arg && n1 == FALSE){
                    pre1 = FALSE;
                }//c=x
            
                uint64_t taggy;
                if(n1 == TRUE){
                taggy =  get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                
                }else{            
                taggy =  get_tag(pred.address, prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                }
                
                for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                    //already in prefetcher
                    if(prefetcher1[i].tag == taggy && prefetcher1[i].valid == 1){
                        pre1 = FALSE;
                    }
                } 
            
                uint64_t tagg4;
                if(n1 == TRUE){
                tagg4 = get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                }else{            
                tagg4 = get_tag(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                }
                uint64_t indexx2;
                if(n1 == TRUE){
                indexx2 = get_index(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                }else{            
                indexx2 =get_index(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                }
                 
                
                indexx2 = indexx2 * (((uint64_t)1)<<cacheconfig1->S);

                for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
                    //already in cache
                    if(cache1[indexx2 + i].tag == tagg4 && cache1[indexx2 + i].valid == 1){
                        pre1 = FALSE;
                    }
        
                }
            
                if(pre1 == TRUE){
                   
                    p_stats -> prefetch_issued = p_stats -> prefetch_issued + 1;
                    uint8_t done = FALSE;
                     for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                        if(prefetcher1[i].valid == 1){
                            prefetcher1[i].LRUage += 1;
                        }
                        if(prefetcher1[i].valid == 0 && done ==FALSE){
                            prefetcher1[i].tag = taggy;
                            prefetcher1[i].valid = 1;
                            prefetcher1[i].dirty = 0;
                            prefetcher1[i].LRUage = 0;
                            done = TRUE;
                        }
                    } 
                    
                    if(done == FALSE){
                        uint64_t counter1 = prefetcher1[0].LRUage;
                        uint64_t spot1 = 0;
                        for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                            if(prefetcher1[i].LRUage > counter1){
                               counter1 = prefetcher1[i].LRUage;
                               spot1 = i;
                             }
                        }       
                        prefetcher1[spot1].tag = taggy;
                        prefetcher1[spot1].LRUage = 0;
                        prefetcher1[spot1].valid = 1;
                        prefetcher1[spot1].dirty = 0;
                    }
                }
                
                
            //next portion of code after prefetching, updating markov stuff
                if(p_stats -> prefetch_misses == 1){
                   markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;
                }else{ 
                    
                    uint8_t inserted = FALSE;
                    for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                        if(markov_table[i].address == markov_table[0].last && markov_table[i].valid == 1){
                        
                             for(int x = 0; x < 4; x++){
                                if(markov_table[i].predictions[x].address == arg >> cacheconfig1->B << cacheconfig1->B && inserted == FALSE){
                           
                                    markov_table[i].predictions[x].likelihood = markov_table[i].predictions[x].likelihood+1;
                                    markov_table[i].LRUage = date;
                                    inserted = TRUE;
                                }
                             }
                             if(inserted == FALSE){
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood == 0 && inserted == FALSE){
                                        markov_table[i].predictions[x].likelihood = 1;
                                        markov_table[i].predictions[x].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                    }
                                }
                                if(inserted == FALSE){
                                uint64_t min = markov_table[i].predictions[0].likelihood;
                                uint64_t inde = 0;
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood < min && inserted == FALSE){
                                        inde = x;
                                        min = markov_table[i].predictions[x].likelihood;
                                    } else if(markov_table[i].predictions[x].likelihood == min && inserted == FALSE){
                                        if(markov_table[i].predictions[x].address < markov_table[i].predictions[inde].address){
                                             inde = x;
                                             min = markov_table[i].predictions[x].likelihood;
                                        }
                                    
                                    }
                                }
                                
                                        markov_table[i].predictions[inde].likelihood = 1;
                                        markov_table[i].predictions[inde].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                }
                             
                             }
                             
                        
                        } 
                    }
                    if(inserted == FALSE){
                        for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                            if(markov_table[i].valid == 0 && inserted != TRUE){
                                markov_table[i].valid = 1;
                                markov_table[i].predictions[0].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                markov_table[i].predictions[0].likelihood = 1;
                                markov_table[i].address = markov_table[0].last;
                                markov_table[i].LRUage = date;
                                inserted = TRUE;
                            }
                        }
                        
                        if(inserted == FALSE){
                            markovrow_t replacer= markov_table[0];
                            uint64_t rep = 0;
                            for(uint64_t i = 1; i < cacheconfig1->P; i++) {
                                if(markov_table[i].LRUage < replacer.LRUage){
                                    replacer = markov_table[i];
                                    rep = i;
                                }
                            }
                            
                            markov_table[rep].address = markov_table[0].last;
                            markov_table[rep].valid = 1;
                            markov_table[rep].LRUage = date;
                            for(int x = 0; x < 4; x++){
                                markov_table[rep].predictions[x].address = 0;
                                markov_table[rep].predictions[x].likelihood = 0;
                                    
                             }
                             
                            markov_table[rep].predictions[0].address = arg >>cacheconfig1->B << cacheconfig1->B;
                            markov_table[rep].predictions[0].likelihood = 1;
                            
                        
                        }
                    }
                    
                  markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;  
            
                }
            
            }
            
            
            
            
            if(cacheconfig1->pt == 1 && is_hit == FALSE){
                prediction_t pred;
                pred.likelihood = 0;
                
                for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                    if(markov_table[i].address == (arg >> cacheconfig1->B << cacheconfig1->B) && markov_table[i].valid == 1){
                        uint64_t max = markov_table[i].predictions[0].likelihood;
                        pred = markov_table[i].predictions[0];
                        for(int x = 1; x < 4; x++){
                            if(max < markov_table[i].predictions[x].likelihood){
                                max = markov_table[i].predictions[x].likelihood;
                                pred = markov_table[i].predictions[x];
                            } else if(max == markov_table[i].predictions[x].likelihood){
                                if(markov_table[i].predictions[x].address > pred.address){
                                    max = markov_table[i].predictions[x].likelihood;
                                    pred = markov_table[i].predictions[x];
                                }    
                            }
                        }
               
                    
                    }// now that u have found the prediction
              
                } 
                
                uint8_t pre1 = TRUE;
                
                if (pred.likelihood == 0){
                    pre1 = FALSE;
                }
                
                if(pred.address == arg){
                    pre1 = FALSE;
                }//c=x
            
                uint64_t taggy =  get_tag(pred.address, prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                
                for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                    //already in prefetcher
                    if(prefetcher1[i].tag == taggy && prefetcher1[i].valid == 1){
                        pre1 = FALSE;
                    }
                } 
            
                uint64_t tagg4 =  get_tag(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                uint64_t indexx2 =  get_index(pred.address, cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                indexx2 = indexx2 * (((uint64_t)1)<<cacheconfig1->S);

                for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
                    //already in cache
                    if(cache1[indexx2 + i].tag == tagg4 && cache1[indexx2 + i].valid == 1){
                        pre1 = FALSE;
                    }
        
                }
            
                if(pre1 == TRUE){
                
                    p_stats -> prefetch_issued = p_stats -> prefetch_issued + 1;
                    uint8_t done = FALSE;
                     for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                        if(prefetcher1[i].valid == 1){
                            prefetcher1[i].LRUage += 1;
                        }
                        if(prefetcher1[i].valid == 0 && done ==FALSE){
                            prefetcher1[i].tag = taggy;
                            prefetcher1[i].valid = 1;
                            prefetcher1[i].dirty = 0;
                            prefetcher1[i].LRUage = 0;
                            done = TRUE;
                        }
                    } 
                    
                    if(done == FALSE){
                        uint64_t counter1 = prefetcher1[0].LRUage;
                        uint64_t spot1 = 0;
                        for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                            if(prefetcher1[i].LRUage > counter1){
                               counter1 = prefetcher1[i].LRUage;
                               spot1 = i;
                             }
                        }       
                        prefetcher1[spot1].tag = taggy;
                        prefetcher1[spot1].LRUage = 0;
                        prefetcher1[spot1].valid = 1;
                        prefetcher1[spot1].dirty = 0;
                    }
                }
                
                
            //next portion of code after prefetching, updating markov stuff
                if(p_stats -> prefetch_misses == 1){
                   markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;
                }else{ 
                    uint8_t inserted = FALSE;
                    for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                        if(markov_table[i].address == markov_table[0].last && markov_table[i].valid == 1){
                        
                             for(int x = 0; x < 4; x++){
                                if(markov_table[i].predictions[x].address == arg >> cacheconfig1->B << cacheconfig1->B && inserted == FALSE){
                                  
                                    markov_table[i].predictions[x].likelihood = markov_table[i].predictions[x].likelihood+1;
                                    markov_table[i].LRUage = date;
                                    inserted = TRUE;
                                }
                             }
                             if(inserted == FALSE){
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood == 0 && inserted == FALSE){
                                        markov_table[i].predictions[x].likelihood = 1;
                                        markov_table[i].predictions[x].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                    }
                                }
                                if(inserted == FALSE){
                                uint64_t min = markov_table[i].predictions[0].likelihood;
                                uint64_t inde = 0;
                                for(int x = 0; x < 4; x++){
                                    if(markov_table[i].predictions[x].likelihood < min && inserted == FALSE){
                                        inde = x;
                                        min = markov_table[i].predictions[x].likelihood;
                                    } else if(markov_table[i].predictions[x].likelihood == min && inserted == FALSE){
                                        if(markov_table[i].predictions[x].address < markov_table[i].predictions[inde].address){
                                             inde = x;
                                             min = markov_table[i].predictions[x].likelihood;
                                        }
                                    
                                    }
                                }
                                
                                        markov_table[i].predictions[inde].likelihood = 1;
                                        markov_table[i].predictions[inde].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                        markov_table[i].LRUage = date;
                                        inserted = TRUE;
                                }
                             
                             }
                             
                        
                        } 
                    }
                    if(inserted == FALSE){
                        for(uint64_t i = 0; i < cacheconfig1->P; i++) {
                    
                            if(markov_table[i].valid == 0 && inserted != TRUE){
                                markov_table[i].valid = 1;
                                markov_table[i].predictions[0].address = arg >> cacheconfig1->B << cacheconfig1->B;
                                markov_table[i].predictions[0].likelihood = 1;
                                markov_table[i].address = markov_table[0].last;
                                markov_table[i].LRUage = date;
                                inserted = TRUE;
                            }
                        }
                        
                        if(inserted == FALSE){
                            markovrow_t replacer= markov_table[0];
                            uint64_t rep = 0;
                            for(uint64_t i = 1; i < cacheconfig1->P; i++) {
                                if(markov_table[i].LRUage < replacer.LRUage){
                                    replacer = markov_table[i];
                                    rep = i;
                                }
                            }
                            
                            markov_table[rep].address = markov_table[0].last;
                            markov_table[rep].valid = 1;
                            markov_table[rep].LRUage = date;
                            for(int x = 0; x < 4; x++){
                                markov_table[rep].predictions[x].address = 0;
                                markov_table[rep].predictions[x].likelihood = 0;
                                    
                             }
                             
                            markov_table[rep].predictions[0].address = arg >>cacheconfig1->B << cacheconfig1->B;
                            markov_table[rep].predictions[0].likelihood = 1;
                            
                        
                        }
                    }
                    
                  markov_table[0].last = arg >> cacheconfig1->B << cacheconfig1->B;  
            
                }
            
            }
            
            
             if(cacheconfig1->pt == 2 && is_hit == FALSE){
                uint8_t pre = TRUE; 
                uint64_t tag3 =  get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), prefetcherconfig1->C, prefetcherconfig1->B, prefetcherconfig1->S);
                
                for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                    //already in prefetcher
                    if(prefetcher1[i].tag == tag3 && prefetcher1[i].valid == 1){
                        pre = FALSE;
                    }
                } 
                
                uint64_t tag4 =  get_tag(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                uint64_t index2 =  get_index(arg + (((uint64_t)1)<<prefetcherconfig1->B), cacheconfig1->C, cacheconfig1->B, cacheconfig1->S);
                
                index2 = index2 * (((uint64_t)1)<<cacheconfig1->S);

                for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
                    //already in cache
                    if(cache1[index2 + i].tag == tag4 && cache1[index2 + i].valid == 1){
                        pre = FALSE;
                    }
        
                }
                
                if(pre ==TRUE){
                p_stats -> prefetch_issued = p_stats -> prefetch_issued + 1;
                     uint8_t done = FALSE;
                     for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                        if(prefetcher1[i].valid == 1){
                            prefetcher1[i].LRUage += 1;
                        }
                        if(prefetcher1[i].valid == 0 && done ==FALSE){
                            prefetcher1[i].tag = tag3;
                            prefetcher1[i].valid = 1;
                            prefetcher1[i].dirty = 0;
                            prefetcher1[i].LRUage = 0;
                            done = TRUE;
                        }
                    } 
                    
                    if(done == FALSE){
                        uint64_t counter1 = prefetcher1[0].LRUage;
                        uint64_t spot1 = 0;
                        for(uint64_t i = 0; i < (((uint64_t)1)<<prefetcherconfig1->S); i++) {
                            if(prefetcher1[i].LRUage > counter1){
                               counter1 = prefetcher1[i].LRUage;
                               spot1 = i;
                             }
                        }       
                        prefetcher1[spot1].tag = tag3;
                        prefetcher1[spot1].LRUage = 0;
                        prefetcher1[spot1].valid = 1;
                        prefetcher1[spot1].dirty = 0;
                    }
        
                
                
                
                }
                
                
                
            }
            
       }
    
       for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
            if(cache1[index + i].valid == 0){
                cache1[index+ i].tag = tag;
                cache1[index+ i].dirty = 1;
                cache1[index+ i].LRUage = 0;
                cache1[index+ i].valid = 1;
                return;
            }
       }  
       //empty slot?take it
       
       //if no empty slot, use lru
       uint64_t counter = cache1[index].LRUage;
            uint64_t spot = 0;
            for(uint64_t i = 0; i < (((uint64_t)1)<<cacheconfig1->S); i++) {
                if(cache1[index + i].LRUage > counter){
                    counter = cache1[index + i].LRUage;
                    spot = i;
                }
            }
            //find the least rarely used block
            
            if(cache1[index + spot].dirty == 1){
            //if that block was dirty, rite it back to mem
                p_stats -> write_back_l1 = p_stats -> write_back_l1 + 1;
            }
         
            //replace that block
            cache1[index + spot].tag = tag;
            cache1[index + spot].dirty = 1;
            cache1[index + spot].LRUage = 0;
            cache1[index + spot].valid = 1;
            return;    
            
    
    }
    return;
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(cache_stats_t *p_stats) {

    if(cacheconfig1->pt == 0){
      p_stats->prefetch_hits = 0;
      p_stats->prefetch_misses = 0;
      p_stats->prefetch_issued = 0;
    }
    
    double hittime = 2 + cacheconfig1->S * 0.2;



    p_stats->l1_hit_ratio =  1.0* p_stats->total_hits_l1 / p_stats->accesses;
    p_stats->l1_miss_ratio = 1.0* p_stats->total_misses_l1 / p_stats->accesses;
    
    p_stats->overall_miss_ratio = (1.0*p_stats->total_misses_l1) / p_stats->accesses;
    
    if(cacheconfig1->pt != 0){
        p_stats->overall_miss_ratio = (1.0*p_stats->prefetch_misses) / p_stats->accesses;
        p_stats->prefetch_hit_ratio = (1.0*p_stats->prefetch_hits) / p_stats->prefetch_issued;
    }
    
    
    p_stats->read_hit_ratio = 1.0* p_stats->read_hits_l1 / p_stats->reads;
    p_stats->read_miss_ratio = 1.0* p_stats->read_misses_l1 / p_stats->reads;
    
    p_stats->write_hit_ratio = 1.0* p_stats->write_hits_l1 / p_stats->writes;
    p_stats->write_miss_ratio = 1.0* p_stats->write_misses_l1 / p_stats->writes;
    
    p_stats->avg_access_time_l1 = hittime + p_stats->overall_miss_ratio * 20;
}



uint64_t get_tag(uint64_t address, uint64_t C, uint64_t B, uint64_t S)
{
     uint64_t tag = address >> ( (C-B-S) + B);
    
    return tag;
}


uint64_t get_index(uint64_t address, uint64_t C, uint64_t B, uint64_t S)
{
    uint64_t shifter = (64 - ((C-B-S) + B));

    uint64_t index= ((address << shifter) >> shifter) >> B;
    
    return index;
}




