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
    bool run_all_cores_; /**< True if matrix benchmarks should run for all cores. */
    bool run_latency_matrix_; /**< True if latency matrix tests should be run. */
    bool run_throughput_matrix_; /**< True if throughput matrix tests should be run. */
    bool register_regions_; /**< True if regions will be registered for remote memory access. */
    bool connect_before_run_; /**< True if the current (local) node must be connected to another (remote) node before running the benchmarks.
                                  Useful for systems containing Xeon PHIs. */
    bool verbose_; /**< If true, then console reporting should be more detailed. */
    size_t working_set_size_per_thread_; /**< Working set size in bytes for each thread, if applicable. */

//         bool run_extensions_; /**< If true, run extensions. */
//         bool run_latency_; /**< True if latency tests should be run. */
//         bool run_throughput_; /**< True if throughput tests should be run. */
//         bool sync_mem_; /**< True if accesses to memory should happen synchronously. If true every access will be uncached. */
//         uint32_t num_worker_threads_; /**< Number of load threads to use for throughput benchmarks, loaded latency benchmarks, and stress tests. */
//         bool numa_enabled_; /**< If false, only CPU/memory NUMA nodes 0 may be used. */
//         std::list<uint32_t> cpu_numa_node_affinities_; /**< List of CPU NUMA nodes to affinitize on all benchmark experiments. */
//         std::list<uint32_t> memory_numa_node_affinities_; /**< List of memory NUMA nodes to affinitize on all benchmark experiments. */
//         uint32_t mem_regions_; /**< Number of memory regions per NUMA node for the matrix benchmark tests. */
//         bool use_random_access_pattern_; /**< If true, run throughput benchmarks with random access pattern. */
//         bool use_sequential_access_pattern_; /**< If true, run throughput benchmarks with sequential access pattern. */
//         uint32_t starting_test_index_; /**< Numerical index to use for the first test. This is an aid for end-user interpreting and post-processing of result CSV file, if relevant. */
//         std::string filename_; /**< The output filename if applicable. */
//         bool use_output_file_; /**< If true, generate a CSV output file for results. */
//         bool use_large_pages_; /**< If true, then large pages should be used. */
//         bool use_reads_; /**< If true, throughput benchmarks should use reads. */
//         bool use_writes_; /**< If true, throughput benchmarks should use writes. */
//         std::string dec_net_filename_; /**< The decoding network friendly output filename if applicable. */
//         bool use_dec_net_file_; /**< If true, generate a decoding net friendly output file for results. */

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
bool allCoresSelected(Configurator *conf);

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
 * @brief Indicates if the register regions procedure has been selected.
 * @returns True if the regions' registration has been selected to run.
 */
bool registerRegionsSelected(Configurator *conf);

/**
 * @brief Indicates if the local node must be connected with a remote node.
 * @returns True if the local node must be connected with a remote node option has been selected.
 */
bool connectBeforeRun(Configurator *conf);

/**
 * @brief Gets the working set size in bytes for each worker thread, if applicable.
 * @returns The working set size in bytes.
 */
size_t getWorkingSetSizePerThread(Configurator *conf);

//         /**
//          * @brief Determines if the benchmarks should test for all CPU/memory NUMA combinations.
//          * @returns True if all NUMA nodes should be tested.
//          */
//         bool isNUMAEnabled() const { return numa_enabled_; }

//         /**
//          * @brief Gets a list of CPU NUMA nodes to affinitize for all benchmark experiments.
//          * @returns The list of NUMA node indices.
//          */
//         std::list<uint32_t> getCpuNumaNodeAffinities() const { return cpu_numa_node_affinities_; }

//         /**
//          * @brief Gets a list of memory NUMA nodes to affinitize for all benchmark experiments.
//          * @returns The list of NUMA node indices.
//          */
//         std::list<uint32_t> getMemoryNumaNodeAffinities() const { return memory_numa_node_affinities_; }

//         /**
//          * @brief Gets the number of memory regions per NUMA node for the matrix benchmarks.
//          * @returns The iterations for each test.
//          */
//         uint32_t getMemoryRegionsPerNUMANode() const { return mem_regions_; }

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

//         /**
//          * @brief Gets the number of worker threads to use.
//          * @returns The number of worker threads.
//          */
//         uint32_t getNumWorkerThreads() const { return num_worker_threads_; }

//         /**
//          * @brief Gets the numerical index of the first benchmark for CSV output purposes.
//          * @returns The starting benchmark index.
//          */
//         uint32_t getStartingTestIndex() const { return starting_test_index_; }

//         /**
//          * @brief Gets the output filename to use, if applicable.
//          * @returns The output filename to use if useOutputFile() returns true. Otherwise return value is "".
//          */
//         std::string getOutputFilename() const { return filename_; }

//         /**
//          * @brief Determines whether to generate an output CSV file.
//          * @returns True if an output file should be used.
//          */
//         bool useOutputFile() const { return use_output_file_; }

//         /**
//          * @brief Changes whether an output file should be used.
//          * @param use If true, then use the output file.
//          */
//         void setUseOutputFile(bool use) { use_output_file_ = use; }

//         /**
//          * @brief Gets the output filename suitable for decoding network to use, if applicable.
//          * @returns The output filename to use if useDecNetFile() returns true. Otherwise return value is "".
//          */
//         std::string getDecNetFilename() const { return dec_net_filename_; }

//         /**
//          * @brief Determines whether to generate an output file with a format suitbale for decoding networks.
//          * @returns True if a decoding network file should be used.
//          */
//         bool useDecNetFile() const { return use_dec_net_file_; }

//         /**
//          * @brief Changes whether an output file written for decoding networks should be used.
//          * @param use If true, then use the output file.
//          */
//         void setUseDecNetFile(bool use) { use_dec_net_file_ = use; }

//         /**
//          * @brief Determines whether X-Mem is in verbose mode.
//          * @returns True if verbose mode is enabled.
//          */
//         bool verboseMode() const { return verbose_; }

//         /**
//          * @brief Determines whether X-Mem should use large pages.
//          * @param True if large pages should be used.
//          */
//         bool useLargePages() const { return use_large_pages_; }

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

//         /**
//          * @brief Determines if a stride of +1 should be used in relevant benchmarks.
//          * @returns True if a stride of +1 should be used.
//          */
//         bool useStrideP1() const { return use_stride_p1_; }

//         /**
//          * @brief Determines if a stride of -1 should be used in relevant benchmarks.
//          * @returns True if a stride of -1 should be used.
//          */
//         bool useStrideN1() const { return use_stride_n1_; }

//         /**
//          * @brief Determines if a stride of +2 should be used in relevant benchmarks.
//          * @returns True if a stride of +2 should be used.
//          */
//         bool useStrideP2() const { return use_stride_p2_; }

//         /**
//          * @brief Determines if a stride of -2 should be used in relevant benchmarks.
//          * @returns True if a stride of -2 should be used.
//          */
//         bool useStrideN2() const { return use_stride_n2_; }

//         /**
//          * @brief Determines if a stride of +4 should be used in relevant benchmarks.
//          * @returns True if a stride of +4 should be used.
//          */
//         bool useStrideP4() const { return use_stride_p4_; }

//         /**
//          * @brief Determines if a stride of -4 should be used in relevant benchmarks.
//          * @returns True if a stride of -4 should be used.
//          */
//         bool useStrideN4() const { return use_stride_n4_; }

//         /**
//          * @brief Determines if a stride of +8 should be used in relevant benchmarks.
//          * @returns True if a stride of +8 should be used.
//          */
//         bool useStrideP8() const { return use_stride_p8_; }

//         /**
//          * @brief Determines if a stride of -8 should be used in relevant benchmarks.
//          * @returns True if a stride of -8 should be used.
//          */
//         bool useStrideN8() const { return use_stride_n8_; }

//         /**
//          * @brief Determines if a stride of +16 should be used in relevant benchmarks.
//          * @returns True if a stride of +16 should be used.
//          */
//         bool useStrideP16() const { return use_stride_p16_; }

//         /**
//          * @brief Determines if a stride of -16 should be used in relevant benchmarks.
//          * @returns True if a stride of -16 should be used.
//          */
//         bool useStrideN16() const { return use_stride_n16_; }

//     private:
//         /**
//          * @brief Inspects a command line option (switch) to see if it occurred more than once, and warns the user if this is the case. The program only uses the first occurrence of any switch.
//          * @param opt The option to inspect.
//          * @returns True if the option only occurred once.
//          */
//         bool check_single_option_occurrence(Option* opt) const;
//     };
// };

#endif
