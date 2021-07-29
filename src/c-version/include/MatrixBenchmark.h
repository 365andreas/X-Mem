/**
 * @file
 *
 * @brief Header file for the MatrixBenchmark class.
 */

#ifndef MATRIX_BENCHMARK_H
#define MATRIX_BENCHMARK_H

//Headers
#include <Thread.h>
#include <common.h>

//Libraries
#include <stdint.h>


/**
 * @brief A type of benchmark that measures memory latency or throughput and aggregates the results in a matrix form.
 */
typedef struct {
    void* mem_array;
    size_t len;
    uint32_t iterations;
    uint32_t num_worker_threads;
    uint32_t mem_node;
    uint32_t mem_region;
    uint32_t cpu_node;
    uint32_t cpu;
    bool use_cpu_nodes;
    pattern_mode_t pattern_mode;
    rw_mode_t rw_mode;
    chunk_size_t chunk_size;
    int32_t stride_size;
    char metric_units[20];
    char name[50];

    // std::vector<double> load_metric_on_iter_; /**< Load metrics for each iteration of the benchmark. This is in MB/s. */
    // double mean_load_metric_; /**< The average load throughput in MB/sec that was imposed on the latency measurement. */
    // std::ofstream &logfile_; /**< The logfile to be used for logging each iteration of the experiments. */

} MatrixBenchmark;

// /**
//  * @brief Destructor.
//  */
// virtual ~MatrixBenchmark();

// /**
//  * @brief Get the average load throughput in MB/sec that was imposed on the latency measurement during the given iteration.
//  * @brief iter The iteration of interest.
//  * @returns The average throughput in MB/sec.
//  */
// double getLoadMetricOnIter(MatrixBenchmark *bench, uint32_t iter);

// /**
//  * @brief Get the overall arithmetic mean load throughput in MB/sec that was imposed on the latency measurement.
//  * @returns The mean throughput in MB/sec.
//  */
// double getMeanLoadMetric(MatrixBenchmark *bench);

/**
 * @brief Gets the CPU id node used in this benchmark.
 * @returns The CPU used in this benchmark.
 */
uint32_t getCPUId(MatrixBenchmark *bench);

// /**
//  * @brief Prints a header piece of information describing the benchmark to the console.
//  */
// void printBenchmarkHeader(MatrixBenchmark *bench);

// /**
//  * @brief Reports benchmark configuration details to the console.
//  */
// void reportBenchmarkInfo(MatrixBenchmark *bench);

// /**
//  * @brief Reports results to the console.
//  */
// void reportResults(MatrixBenchmark *bench);

// /**
//  * @brief Computes the metrics across iterations.
//  */
// void computeMetrics(MatrixBenchmark *bench);

#endif
