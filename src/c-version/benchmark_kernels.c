/**
 * @file
 *
 * @brief Implementation file for benchmark kernel functions for doing the actual work we care about. :)
 *
 * Optimization tricks include:
 *   - UNROLL macros to manual loop unrolling. This reduces the relative branch overhead of the loop.
 *     We don't want to benchmark loops, we want to benchmark memory! But unrolling too much can hurt
 *     code size and instruction locality, potentially decreasing I-cache utilization and causing extra overheads.
 *     This is why we allow multiple unroll lengths at compile-time.
 *   - volatile keyword to prevent compiler from optimizing the code and removing instructions that we need.
 *     The compiler is too smart for its own good!
 */

//Headers
#include <benchmark_kernels.h>
#include <common.h>

//Libraries
#include <stdio.h>
#include <time.h>
#include "mtwist.h"

void shuffle(Word64_t *first, Word64_t *last) {

    uint64_t size = ((uint64_t) last - (uint64_t) first) / sizeof(Word64_t);

    for (uint64_t i = size - 1; i > 0; --i) {
        uint64_t r = mt_llrand();
        uint64_t ind = r % (i + 1);
        // std::uniform_int_distribution<decltype(i)> d(0,i);
        Word64_t a = first[i];
        first[i] = first[ind];
        first[ind] = a;
    }
}

bool build_random_pointer_permutation(void* start_address, void* end_address, chunk_size_t chunk_size, bool only_offset) {
    if (g_verbose)
        printf("Preparing a memory region under test. This might take a while...");

    size_t length = (uint8_t *) end_address - (uint8_t *) start_address; //length of region in bytes
    size_t num_pointers = 0; //Number of pointers that fit into the memory region of interest
    switch (chunk_size) {
        //special case on 32-bit architectures only.
        case CHUNK_64b:
            num_pointers = length / sizeof(Word64_t);
            break;
        default:
            fprintf(stderr, "ERROR: Chunk size %d not available\n", chunk_size);
            return false;
    }

    mt_seed();
    // std::mt19937_614 gen(time(NULL)); //Mersenne Twister random number generator, seeded at current time

    //Do a random shuffle of memory pointers.
    //I had originally used a random Hamiltonian Cycle generator, but this was much slower and aside from
    //rare instances, did not make any difference in random-access performance measurement.
    Word64_t* mem_region_base = (Word64_t *) (start_address);

    switch (chunk_size) {
        case CHUNK_64b:
            for (size_t i = 0; i < num_pointers; i++) { //Initialize pointers to point at themselves (identity mapping)
                if (only_offset) {
                    mem_region_base[i] = (Word64_t) i;
                } else {
                    mem_region_base[i] = (Word64_t) (mem_region_base + i);
                }
            }
            shuffle(mem_region_base, mem_region_base + num_pointers);
            break;
        default:
            fprintf(stderr, "ERROR: Got an invalid chunk size. This should not have happened.\n");
            return false;
    }

    if (g_verbose) {
        printf("done\n\n");
    }

    return true;
}

/***********************************************************************
 ***********************************************************************
 ********************** LATENCY-RELATED BENCHMARK KERNELS **************
 ***********************************************************************
 ***********************************************************************/

/* --------------------- DUMMY BENCHMARK ROUTINES --------------------------- */

int32_t dummy_chasePointers(uintptr_t *base_address, uintptr_t **last_touched_offset, size_t len) {
    volatile uintptr_t placeholder = 0; //Try to defeat compiler optimizations removing this method
    return 0;
}

/* -------------------- CORE BENCHMARK ROUTINES -------------------------- */

int32_t chasePointers(uintptr_t *base_address, uintptr_t **last_touched_offset, size_t len) {
    volatile uintptr_t *p = (uintptr_t *) ((uint64_t) *last_touched_offset + base_address);
    UNROLL512(p = (uintptr_t *) (*p + base_address);)
    *last_touched_offset = (uintptr_t *) (p - base_address);
    return 0;
}


// /***********************************************************************
//  ***********************************************************************
//  ******************* THROUGHPUT-RELATED BENCHMARK KERNELS **************
//  ***********************************************************************
//  ***********************************************************************/

#ifdef HAS_WORD_64
int32_t dummy_forwSequentialLoop_Word64(void* start_address, void* end_address) {
    volatile int32_t placeholder = 0; //Try our best to defeat compiler optimizations
    for (volatile Word64_t* wordptr = (Word64_t *) start_address, *endptr = (Word64_t *) end_address; wordptr < endptr;) {
        UNROLL512(wordptr++;)
        placeholder = 0;
    }
    return placeholder;
}
#endif

// /* ------------ RANDOM LOOP --------------*/

#ifdef HAS_WORD_64
int32_t dummy_randomLoop_Word64(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
    volatile uintptr_t* placeholder = NULL; //Try to defeat compiler optimizations removing this method
    return 0;
}
#endif

// /* -------------------- CORE BENCHMARK ROUTINES --------------------------
//  *
//  * These routines access the memory in different ways for each benchmark type.
//  * Optimization tricks include:
//  *   - register keyword to hint to compiler values that should not be used in memory access
//  *   - UNROLL macros to manual loop unrolling. This reduces the relative branch overhead of the loop.
//  *     We don't want to benchmark loops, we want to benchmark memory!
//  *   - volatile keyword to prevent compiler from optimizing the code and removing instructions that we need.
//  *     The compiler is too smart for its own good!
//  *   - Hardcoding stride and chunk sizes in each benchmark snippet, so that way they do not have to waste cycles being computed at runtime.
//  *     This makes code harder to maintain and to read but ensures more accurate memory benchmarking.
//  *
//  * ----------------------------------------------------------------------- */

// /* ------------ SEQUENTIAL READ --------------*/

int32_t forwSequentialRead_Word32(void* start_address, void* end_address) {
    register Word32_t val;
    for (volatile Word32_t* wordptr = (Word32_t *) start_address, *endptr = (Word32_t *) end_address; wordptr < endptr;) {
        UNROLL1024(val = *wordptr++;)
    }
    return 0;
}

#ifdef HAS_WORD_64
int32_t forwSequentialRead_Word64(void* start_address, void* end_address) {
    register Word64_t val;
    for (volatile Word64_t* wordptr = (Word64_t *) start_address, *endptr = (Word64_t *) end_address; wordptr < endptr;) {
        UNROLL512(val = *wordptr++;)
    }
    return 0;
}
#endif

// /* ------------ SEQUENTIAL WRITE --------------*/

int32_t forwSequentialWrite_Word32(void* start_address, void* end_address) {
    register Word32_t val = 0xFFFFFFFF;
    for (volatile Word32_t* wordptr = (Word32_t*) start_address, *endptr = (Word32_t *) end_address; wordptr < endptr;) {
        UNROLL1024(*wordptr++ = val;)
    }
    return 0;
}

#ifdef HAS_WORD_64
int32_t forwSequentialWrite_Word64(void* start_address, void* end_address) {
    register Word64_t val = 0xFFFFFFFFFFFFFFFF;
    for (volatile Word64_t* wordptr = (Word64_t *) start_address, *endptr = (Word64_t *) end_address; wordptr < endptr;) {
        UNROLL512(*wordptr++ = val;)
    }
    return 0;
}
#endif

#ifdef HAS_WORD_64
int32_t randomRead_Word64(uintptr_t* first_address, uintptr_t** last_touched_address, size_t len) {
    volatile uintptr_t* p = first_address;

    UNROLL512(p = (uintptr_t *) *p;)
    *last_touched_address = (uintptr_t *) p;
    return 0;
}
#endif
