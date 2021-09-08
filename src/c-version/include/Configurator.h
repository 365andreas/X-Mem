/**
 * @file
 *
 * @brief Header file for the Configurator struct and some helper data structures.
 */

#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

//Headers
#include <common.h>

//Libraries
#include <stdbool.h>
#include <stdint.h>


typedef struct {
    bool configured_; /**< If true, this object has been configured. configureFromInput() will only work if this is false. */
    uint32_t iterations_; /**< Number of iterations to run for each benchmark test. */
    bool mem_regions_in_phys_addr_; /**< True if physical addresses of memory regions are passed as arguments for matrix becnhmarks. */
    uint64_t *mem_regions_phys_addr_; /**< Array of physical addresses of memory regions to be used for matrix benchmark experiments. */
    uint32_t mem_regions_phys_addr_size; /**< Number of memory regions to be used for matrix benchmark experiments. */
    uint32_t core_id_; /**< Core id of the core that will be used for the benchmarks. */
    bool run_all_cores_; /**< True if matrix benchmarks should run for all cores. */
    uint32_t use_num_cores_; /**< Specifies how many cores should be used for the benchmarks. The cores selected will
                                  be consecutive strating from the specified, or 0 if not specified. */
    bool run_latency_matrix_; /**< True if latency matrix tests should be run. */
    bool run_throughput_matrix_; /**< True if throughput matrix tests should be run. */
    bool verbose_; /**< If true, then console reporting should be more detailed. */
    size_t working_set_size_per_thread_; /**< Working set size in bytes for each thread, if applicable. */
    bool sync_mem_; /**< True if accesses to memory should happen synchronously. If true every access will be uncached. */
    bool use_dec_net_file_; /**< If true, generate a decoding net friendly output file for results. */
    char *dec_net_filename_; /**< The decoding network compatible output filename if applicable. */

    // bool use_random_access_pattern_; /**< If true, run throughput benchmarks with random access pattern. */
    // bool use_sequential_access_pattern_; /**< If true, run throughput benchmarks with sequential access pattern. */
    // bool use_reads_; /**< If true, throughput benchmarks should use reads. */
    // bool use_writes_; /**< If true, throughput benchmarks should use writes. */

} Configurator;

Configurator config;


/**
 * @brief Configures the tool based on user's command-line inputs.
 * @param argc The argc from main().
 * @param argv The argv from main().
 * @returns 0 on success.
 */
int32_t configureFromInput(Configurator *conf, int argc, char *argv[]);

/**
 * @brief Indicates if all cores have been selected for the matrix tests.
 * @returns True if all cores have been selected.
 */
bool runForAllCoresSelected(Configurator *conf);

/**
 * @brief Indicates which core will be used for the matrix tests.
 * @returns The core which will be used for the matrix tests.
 */
uint32_t getCoreId(Configurator *conf);

/**
 * @brief Indicates the number of cores that will be used for the throughput matrix tests.
 * @returns The number of cores that will be used for the throughput matrix tests.
 */
uint32_t useNumCores(Configurator *conf);

/**
 * @brief Indicates if the latency matrix test has been selected.
 * @returns True if the latency matrix test has been selected to run.
 */
bool latencyMatrixTestSelected(Configurator *conf);

/**
 * @brief Indicates if the throughput matrix test has been selected.
 * @returns True if the throughput matrix test has been selected to run.
 */
bool throughputMatrixTestSelected(Configurator *conf);

/**
 * @brief Indicates if the accesses to memory for matrix tests will be processed synchronously.
 * @returns True if the accesses to memory for matrix tests will be processed synchronously.
 */
bool syncMemory(Configurator *conf);

/**
 * @brief Gets the working set size in bytes for each worker thread, if applicable.
 * @returns The working set size in bytes.
 */
size_t getWorkingSetSizePerThread(Configurator *conf);

/**
 * @brief Determines if the matrix benchmarks uses physical addresses to specify the regions to be tested.
 * @returns True if physical addresses are given for matrix benchmarks.
 */
bool memoryRegionsInPhysAddr(Configurator *conf);

/**
 * @brief Gets the number of memory regions' physical addresses for the matrix benchmarks.
 * @returns The number.
 */

uint32_t numberOfMemoryRegionsPhysAddresses(Configurator *conf);

/**
 * @brief Gets the memory regions' physical addresses for the matrix benchmarks.
 * @returns The vector of physical addresses.
 */
uint64_t *getMemoryRegionsPhysAddresses(Configurator *conf);

/**
 * @brief Gets the number of iterations that should be run of each benchmark.
 * @returns The iterations for each test.
 */
uint32_t getIterationsPerTest(Configurator *conf);

//         /**
//          * @brief Determines if throughput benchmarks should use a random access pattern.
//          * @returns True if random access should be used.
//          */
//         bool useRandomAccessPattern() const { return use_random_access_pattern_; }

//         /**
//          * @brief Determines if throughput benchmarks should use a sequential access pattern.
//          * @returns True if sequential access should be used.
//          */
//         bool useSequentialAccessPattern() const { return use_sequential_access_pattern_; }

/**
 * @brief Gets the output filename suitable for decoding network to use, if applicable.
 * @returns The output filename to use if useDecNetFile() returns true. Otherwise return value is "".
 */
char *getDecNetFilename(Configurator *conf);

/**
 * @brief Determines whether to generate an output file with a format suitbale for decoding networks.
 * @returns True if a decoding network file should be used.
 */
bool useDecNetFile(Configurator *conf);

/**
 * @brief Changes whether an output file written for decoding networks should be used.
 * @param use If true, then use the output file.
 */
void setUseDecNetFile(Configurator *conf, bool use);

//         /**
//          * @brief Determines whether reads should be used in throughput benchmarks.
//          * @returns True if reads should be used.
//          */
//         bool useReads() const { return use_reads_; }

//         /**
//          * @brief Determines whether writes should be used in throughput benchmarks.
//          * @returns True if writes should be used.
//          */
//         bool useWrites() const { return use_writes_; }
#endif
