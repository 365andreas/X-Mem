/**
 * @file
 *
 * @brief Header file for the LatencyMatrixBenchmark class.
 */

#ifndef LATENCY_MATRIX_BENCHMARK_H
#define LATENCY_MATRIX_BENCHMARK_H

//Headers
#include <MatrixBenchmark.h>
#include <common.h>

//Libraries
#include <cstdint>
#include <fstream>
#include <string>

namespace xmem {

    /**
     * @brief A type of benchmark that measures memory latency via random pointer chasing. Loading may be provided with separate threads which access memory as quickly as possible using given access patterns.
     */
    class LatencyMatrixBenchmark : public MatrixBenchmark {
    public:

        /**
         * @brief Constructor. Parameters are passed directly to the Benchmark constructor. See Benchmark class documentation for parameter semantics.
         */
        LatencyMatrixBenchmark(
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
        virtual ~LatencyMatrixBenchmark() {}

    protected:
        virtual bool runCore();

        std::vector<double> load_metric_on_iter_; /**< Load metrics for each iteration of the benchmark. This is in MB/s. */
        double mean_load_metric_; /**< The average load throughput in MB/sec that was imposed on the latency measurement. */
    };
};

#endif
