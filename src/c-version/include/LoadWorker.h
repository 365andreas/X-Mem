/**
 * @file
 *
 * @brief Header file for the LoadWorker class.
 */

#ifndef LOAD_WORKER_H
#define LOAD_WORKER_H

//Headers
#include <MemoryWorker.h>
#include <benchmark_kernels.h>

/**
 * @brief Multithreading-friendly class to do memory loading.
 */
typedef struct {

    MemoryWorker *mem_worker;
    bool use_sequential_kernel_fptr_; /**< If true, use the SequentialFunction, otherwise use the RandomFunction. */
    SequentialFunction kernel_fptr_seq_; /**< Points to the memory test core routine to use of the "sequential" type. */
    SequentialFunction kernel_dummy_fptr_seq_; /**< Points to a dummy version of the memory test core routine to use of the "sequential" type. */
    RandomFunction kernel_fptr_ran_; /**< Points to the memory test core routine to use of the "random" type. */
    RandomFunction kernel_dummy_fptr_ran_; /**< Points to a dummy version of the memory test core routine to use of the "random" type. */
    int32_t cpu_affinity;
} LoadWorker;

LoadWorker *newLoadWorkerSeq(void* mem_array, size_t len, SequentialFunction kernel_fptr, SequentialFunction kernel_dummy_fptr, int32_t cpu_affinity);

LoadWorker *newLoadWorkerRan(void* mem_array, size_t len, RandomFunction kernel_fptr, RandomFunction kernel_dummy_fptr, int32_t cpu_affinity);


/**
 * @brief Destructor.
 */
void deleteLoadWorker(LoadWorker *load_worker);

/**
 * @brief Thread-safe worker method.
 */
void runLoadWorker(LoadWorker *load_worker);

#endif
