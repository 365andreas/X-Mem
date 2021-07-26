/**
 * @file
 *
 * @brief Header file for the BenchmarkManager struct.
 */

#ifndef BENCHMARK_MANAGER_H
#define BENCHMARK_MANAGER_H

//Headers
#include <common.h>
#include <Timer.h>
// #include <Benchmark.h>
// #include <LatencyMatrixBenchmark.h>
// #include <ThroughputMatrixBenchmark.h>
#include <Configurator.h>

//Libraries
#include <stdint.h>

/**
 * @brief Manages running all benchmarks at a high level.
 */
typedef struct {

    Configurator *config_;

    uint32_t *cpu_numa_node_affinities_; /**< List of CPU nodes to affinitize for benchmark experiments. */
    uint32_t *memory_numa_node_affinities_; /**< List of memory nodes to affinitize for benchmark experiments. */
    uint32_t num_mem_regions_;
    void* *mem_arrays_; /**< Memory regions to use in benchmarks. One for each benchmarked NUMA node. */
#ifndef HAS_NUMA
    void* orig_malloc_addr_; /**< Points to the original address returned by the malloc() for __mem_arrays on non-NUMA machines. Special case. FIXME: do we need this? seems awkward */
#endif
    size_t *mem_array_lens_; /**< Length of each memory region to use in benchmarks. */
    size_t *mem_array_node_; /**< NUMA node of each memory region to use in benchmarks. */
    uint32_t num_lat_mat_benchmarks_;
    uint32_t num_thr_mat_benchmarks_;
    // LatencyMatrixBenchmark* *lat_mat_benchmarks_; /**< Set of latency matrix benchmarks. */
    // ThroughputMatrixBenchmark* *thr_mat_benchmarks_; /**< Set of throughput matrix benchmarks. */
    // std::fstream results_file_; /**< The results CSV file. */
    // std::fstream dec_net_results_file_; /**< The results file for use by a decoding network. */
    // std::ofstream lat_mat_logfile_; /**< Logfile for latency matrix measurements. */
    // std::ofstream thr_mat_logfile_; /**< Logfile for throughput matrix measurements. */
    bool built_benchmarks_; /**< If true, finished building all benchmarks. */
} BenchmarkManager;

/**
 * @brief Constructor.
 * @param config The configuration object containing run-time options for this X-Mem execution instance.
 */
BenchmarkManager *initBenchMgr(Configurator *config);

// /**
//  * @brief Destructor.
//  */
// destructBenchmarkManager();

// /**
//  * @brief Runs all benchmark configurations (does not include extensions).
//  * @returns True on success.
//  */
// bool runAll(BenchmarkManager *benchmgr);

// /**
//  * @brief Runs the latency matrix benchmark.
//  * @returns True on benchmarking success.
//  */
// bool runLatencyMatrixBenchmarks(BenchmarkManager *benchmgr);

// /**
//  * @brief Runs the throughput matrix benchmark.
//  * @returns True on benchmarking success.
//  */
// bool runThroughputMatrixBenchmarks(BenchmarkManager *benchmgr);

/**
 * @brief Allocates memory for all working sets.
 * @param working_set_size Memory size in bytes, per enabled NUMA node.
 */
void setupWorkingSets(BenchmarkManager *benchmgr, size_t working_set_size);

// /**
//  * @brief Constructs and initializes all configured benchmarks.
//  * @returns True on success.
//  */
// bool buildBenchmarks(BenchmarkManager *benchmgr);

// /**
//  * @brief Prints the results aggregated in a matrix form (for latencyMatrix and throughputMatrix benchmarks).
//  */
// void printMatrix(std::vector<MatrixBenchmark *> mat_benchmarks_, std::string what);

#endif
