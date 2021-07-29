/**
 * @file
 *
 * @brief Header file for the LatencyWorker struct.
 */

#ifndef LATENCY_WORKER_H
#define LATENCY_WORKER_H

//Headers
#include <MemoryWorker.h>
#include <benchmark_kernels.h>
#include <common.h>

/**
 * @brief Multithreading-friendly struct to do memory loading.
 */
typedef struct {

    MemoryWorker *mem_worker;
    RandomFunction kernel_fptr;
    RandomFunction kernel_dummy_fptr;
    int32_t cpu_affinity;
} LatencyWorker;

// /**
//  * @brief Destructor.
//  */
// ~LatencyWorker();

// /**
//  * @brief Thread-safe worker method.
//  */
// void run();


#endif
