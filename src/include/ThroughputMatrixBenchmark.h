/**
 * @file
 *
 * @brief Header file for the ThroughputMatrixBenchmark class.
 */

#ifndef THROUGHPUT_MATRIX_BENCHMARK_H
#define THROUGHPUT_MATRIX_BENCHMARK_H

//Headers
#include <MatrixBenchmark.h>
#include <common.h>

//Libraries
#include <cstdint>
#include <string>

namespace xmem {

    /**
     * @brief A type of benchmark that measures memory throughput. Loading may be provided with separate threads which access memory as quickly as possible using given access patterns.
     */
    class ThroughputMatrixBenchmark : public MatrixBenchmark {
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
            std::string name,
            std::ofstream &logfile
        );

        /**
         * @brief Destructor.
         */
        virtual ~ThroughputMatrixBenchmark() {}

    protected:
        virtual bool runCore();

    };
};

#endif
