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
    bool assume_existing_pointers;
    uint32_t num_worker_threads;
    uint32_t mem_region;
    uint32_t cpu;
    pattern_mode_t pattern_mode;
    rw_mode_t rw_mode;
    chunk_size_t chunk_size;
    int32_t stride_size;
    char metric_units[20];
    char name[50];

    bool has_run_; /**< Indicates whether the benchmark has run. */
    bool warning_; /**< Indicates whether the benchmarks results might be clearly questionable/inaccurate/incorrect due to a variety of factors. */
    double *metric_on_iter_; /**< Metrics for each iteration of the benchmark. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double *enumerator_metric_on_iter_; /**< Metrics for each iteration of the benchmark. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double *denominator_metric_on_iter_; /**< Denominator metric for ratio metrics for each iteration of the benchmark. */
    double mean_metric_; /**< Average metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double median_metric_; /**< Median metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double lower_95_CI_median_; /**< Lower bound value of the 95% CI of the median. */
    double upper_95_CI_median_; /**< Upper bound value of the 95% CI of the median. */
} MatrixBenchmark;

/**
 * @brief Constructor.
 */
MatrixBenchmark *newMatrixBenchmark(void* mem_array, size_t mem_array_len, uint32_t iters,
                                    bool assume_existing_pointers, uint32_t num_worker_threads, uint32_t mem_region,
                                    uint32_t cpu, pattern_mode_t pattern_mode, rw_mode_t rw_mode,
                                    chunk_size_t chunk_size, int32_t stride_size, char *benchmark_name,
                                    char *metric_units);

// /**
//  * @brief Destructor.
//  */
// virtual ~MatrixBenchmark();

/**
 * @brief Computes the median metric across `n` iterations.
 */
void computeMedian(MatrixBenchmark *mat_bench, uint32_t n);

/**
 * @brief Gets the median benchmark metric across all iterations.
 * @returns The median metric.
 */
double getMedianMetric(MatrixBenchmark *mat_bench);

/**
 * @brief Gets the CPU id node used in this benchmark.
 * @returns The CPU used in this benchmark.
 */
uint32_t getCPUId(MatrixBenchmark *mat_bench);

/**
 * @brief Gets the units of the metric for this benchmark.
 * @returns A string representing the units for printing to console and file.
 */
char *getMetricUnits(MatrixBenchmark *mat_bench);

/**
 * @brief Gets the region id of the memory used in this benchmark.
 * @returns The region id of memory area used in this benchmark.
 */
uint32_t getMemRegion(MatrixBenchmark *mat_bench);

/**
 * @brief Reports results to the console.
 */
void reportResults(MatrixBenchmark *mat_bench);

/**
 * @brief Computes the metrics across iterations.
 */
void computeMetrics(MatrixBenchmark *mat_bench);

#endif
