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

    bool has_run_; /**< Indicates whether the benchmark has run. */
    bool warning_; /**< Indicates whether the benchmarks results might be clearly questionable/inaccurate/incorrect due to a variety of factors. */
    double *metric_on_iter_; /**< Metrics for each iteration of the benchmark. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double *enumerator_metric_on_iter_; /**< Metrics for each iteration of the benchmark. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double *denominator_metric_on_iter_; /**< Denominator metric for ratio metrics for each iteration of the benchmark. */
    double mean_metric_; /**< Average metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    // double min_metric_; /**< Minimum metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    // double percentile_25_metric_; /**< 25th percentile metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double median_metric_; /**< Median metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    // double percentile_75_metric_; /**< 75th percentile metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    // double percentile_95_metric_; /**< 95th percentile metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    // double percentile_99_metric_; /**< 99th percentile metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    // double max_metric_; /**< Maximum metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    // double mode_metric_; /**< Mode metric over all iterations. Unit-less because any benchmark can set this metric as needed. It is up to the descendant class to interpret units. */
    double lower_95_CI_median_; /**< Lower bound value of the 95% CI of the median. */
    double upper_95_CI_median_; /**< Upper bound value of the 95% CI of the median. */
    // std::string metric_units_; /**< String representing the units of measurement for the metric. */
    // std::string enumerator_metric_units_; /**< String representing the units of measurement for the enumerator of ratio metrics. */
    // std::string denominator_metric_units_; /**< String representing the units of measurement for the denominator of ratio metrics. */
    // std::vector<double> load_metric_on_iter_; /**< Load metrics for each iteration of the benchmark. This is in MB/s. */
    // double mean_load_metric_; /**< The average load throughput in MB/sec that was imposed on the latency measurement. */
    // std::ofstream &logfile_; /**< The logfile to be used for logging each iteration of the experiments. */

} MatrixBenchmark;

/**
 * @brief Constructor.
 */
MatrixBenchmark *newMatrixBenchmark(void* mem_array, size_t mem_array_len, uint32_t iters, uint32_t num_worker_threads,
                                    uint32_t mem_node, uint32_t mem_region, uint32_t cpu_node, uint32_t cpu,
                                    bool use_cpu_nodes, pattern_mode_t pattern_mode, rw_mode_t rw_mode,
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

// /**
//  * @brief Get the average load throughput in MB/sec that was imposed on the latency measurement during the given iteration.
//  * @brief iter The iteration of interest.
//  * @returns The average throughput in MB/sec.
//  */
// double getLoadMetricOnIter(MatrixBenchmark *mat_bench, uint32_t iter);

// /**
//  * @brief Get the overall arithmetic mean load throughput in MB/sec that was imposed on the latency measurement.
//  * @returns The mean throughput in MB/sec.
//  */
// double getMeanLoadMetric(MatrixBenchmark *mat_bench);

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

// /**
//  * @brief Prints a header piece of information describing the benchmark to the console.
//  */
// void printBenchmarkHeader(MatrixBenchmark *mat_bench);

// /**
//  * @brief Reports benchmark configuration details to the console.
//  */
// void reportBenchmarkInfo(MatrixBenchmark *mat_bench);

/**
 * @brief Reports results to the console.
 */
void reportResults(MatrixBenchmark *mat_bench);

/**
 * @brief Computes the metrics across iterations.
 */
void computeMetrics(MatrixBenchmark *mat_bench);

#endif
