/**
 * @file
 *
 * @brief Header file for the ThroughputMatrixBenchmark class.
 */

#ifndef THROUGHPUT_MATRIX_BENCHMARK_H
#define THROUGHPUT_MATRIX_BENCHMARK_H

//Headers
#include <Benchmark.h>
#include <common.h>

//Libraries
#include <cstdint>
#include <string>

namespace xmem {

    /**
     * @brief A type of benchmark that measures memory throughput. Loading may be provided with separate threads which access memory as quickly as possible using given access patterns.
     */
    class ThroughputMatrixBenchmark : public Benchmark {
    public:

        /**
         * @brief Constructor. Parameters are passed directly to the Benchmark constructor. See Benchmark class documentation for parameter semantics.
         */
        ThroughputMatrixBenchmark(
            void* mem_array,
            size_t len,
            uint32_t iterations,
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
            std::vector<PowerReader*> dram_power_readers,
            std::string name
        );

        /**
         * @brief Destructor.
         */
        virtual ~ThroughputMatrixBenchmark() {}

        /**
         * @brief Gets the CPU id node used in this benchmark.
         * @returns The CPU used in this benchmark.
         */
        uint32_t getCPUId() const;

        /**
         * @brief Prints a header piece of information describing the benchmark to the console.
         */
        virtual void printBenchmarkHeader() const;

        /**
         * @brief Reports benchmark configuration details to the console.
         */
        virtual void reportBenchmarkInfo() const;

        /**
         * @brief Reports results to the console.
         */
        virtual void reportResults() const;

        /**
         * @brief Computes the metrics across iterations.
         */
        virtual void computeMetrics();

    protected:
        virtual bool runCore();

        uint32_t cpu_;
        bool use_cpu_nodes_;
        uint32_t iterations_needed_;

    };
};

#endif