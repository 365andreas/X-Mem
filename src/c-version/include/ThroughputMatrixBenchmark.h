/**
 * @file
 *
 * @brief Header file for the ThroughputMatrixBenchmark struct.
 */

#ifndef THROUGHPUT_MATRIX_BENCHMARK_H
#define THROUGHPUT_MATRIX_BENCHMARK_H

//Headers
#include <MatrixBenchmark.h>
#include <common.h>

//Libraries
#include <stdint.h>

/**
 * @brief A type of benchmark that measures memory latency via random pointer chasing. Loading may be provided with separate threads which access memory as quickly as possible using given access patterns.
 */
typedef struct {
    MatrixBenchmark *mat_bench;
} ThroughputMatrixBenchmark;


ThroughputMatrixBenchmark *initThroughputMatrixBenchmark(void *mem_array,
                                                         size_t mem_array_len,
                                                         uint32_t iters,
                                                         uint32_t num_worker_threads,
                                                         uint32_t mem_node,
                                                         uint32_t mem_region,
                                                         uint32_t cpu_node,
                                                         uint32_t cpu,
                                                         bool use_cpu_nodes,
                                                         pattern_mode_t pattern_mode,
                                                         rw_mode_t rw_mode,
                                                         chunk_size_t chunk_size,
                                                         int32_t stride_size,
                                                         char *benchmark_name);

// /**
//  * @brief Destructor.
//  */
// virtual ~ThroughputMatrixBenchmark() {}

bool runThroughput(ThroughputMatrixBenchmark *thr_mat_bench);

#endif
