/**
 * @file
 *
 * @brief Header file for the MemoryWorker struct.
 */

#ifndef MEMORY_WORKER_H
#define MEMORY_WORKER_H

//Headers
#include <common.h>
#include <Runnable.h>

//Libraries
// #include <cstdint>

/**
 * @brief Multithreading-friendly class to run memory access kernels.
 */
typedef struct {
    Runnable *runnable;

    // ONLY ACCESS OBJECT VARIABLES UNDER THE RUNNABLE OBJECT LOCK!!!!
    void* mem_array_; /**< The memory region for this worker. */
    size_t len_; /**< The length of the memory region for this worker. */
    int32_t cpu_affinity_; /**< The logical CPU affinity for this worker. */
    uint32_t bytes_per_pass_; /**< Number of bytes accessed in each kernel pass. */
    uint32_t passes_; /**< Number of passes. */
    tick_t elapsed_ticks_; /**< Total elapsed ticks on the kernel routine. */
    tick_t elapsed_dummy_ticks_; /**< Total elapsed ticks on the dummy kernel routine. */
    tick_t adjusted_ticks_; /**< Elapsed ticks minus dummy elapsed ticks. */
    bool warning_; /**< If true, results may be suspect. */
    bool completed_; /**< If true, worker completed. */
} MemoryWorker;

/**
 * @brief Constructor.
 * @param mem_array Pointer to the memory region to use by this worker.
 * @param len Length of the memory region to use by this worker.
 * @param cpu_affinity Logical CPU identifier to lock this worker's thread to.
 */
MemoryWorker *newMemoryWorker(void* mem_array, size_t len, int32_t cpu_affinity);

// /**
//  * @brief Destructor.
//  */
// virtual ~MemoryWorker();

// /**
//  * @brief Thread-safe worker method.
//  */
// virtual void run() = 0;

// /**
//  * @brief Gets the length of the memory region used by this worker.
//  * @returns Length of memory region in bytes.
//  */
// size_t getLen();

/**
 * @brief Gets the number of bytes used in each pass of the benchmark by this worker.
 * @returns Number of bytes in each pass.
 */
uint32_t getBytesPerPass(MemoryWorker *mem_worker);

/**
 * @brief Gets the number of passes for this worker.
 * @returns The number of passes.
 */
uint32_t getPasses(MemoryWorker *mem_worker);

/**
 * @brief Gets the elapsed ticks for this worker on the core benchmark kernel.
 * @returns The number of elapsed ticks.
 */
tick_t getElapsedTicks(MemoryWorker *mem_worker);

/**
 * @brief Gets the elapsed ticks for this worker on the dummy version of the core benchmark kernel.
 * @returns The number of elapsed dummy ticks.
 */
tick_t getElapsedDummyTicks(MemoryWorker *mem_worker);

/**
 * @brief Gets the adjusted ticks for this worker. This is elapsed ticks minus elapsed dummy ticks.
 * @returns The adjusted ticks for this worker.
 */
tick_t getAdjustedTicks(MemoryWorker *mem_worker);

/**
 * @brief Indicates whether worker's results may be questionable/inaccurate/invalid.
 * @returns True if the worker's results had a warning.
 */
bool hadWarning(MemoryWorker *mem_worker);

#endif
