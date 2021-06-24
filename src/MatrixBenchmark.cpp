/**
 * @file
 *
 * @brief Implementation file for the MatrixBenchmark class.
 */

//Headers
#include <MatrixBenchmark.h>
#include <common.h>
#include <benchmark_kernels.h>
#include <MemoryWorker.h>
#include <LatencyWorker.h>
#include <LoadWorker.h>

//Libraries
#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <time.h>
#include <util.h>

#ifdef _WIN32
#include <windows.h>
#include <processthreadsapi.h>
#endif

using namespace xmem;

MatrixBenchmark::MatrixBenchmark(
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
        std::string metric_units,
        std::string name,
        std::ofstream &logfile
    ) :
        Benchmark(
            mem_array,
            len,
            iterations,
            num_worker_threads,
            mem_node,
            mem_region,
            cpu_node,
            pattern_mode,
            rw_mode,
            chunk_size,
            stride_size,
            dram_power_readers,
            metric_units,
            name
        ),
        cpu_(cpu),
        use_cpu_nodes_(use_cpu_nodes),
        load_metric_on_iter_(),
        mean_load_metric_(0),
        logfile_(logfile)
    {

    for (uint32_t i = 0; i < iterations_; i++)
        load_metric_on_iter_.push_back(0);
}

MatrixBenchmark::~MatrixBenchmark() {

}

uint32_t MatrixBenchmark::getCPUId() const {
    return  use_cpu_nodes_ ? cpu_id_in_numa_node(getCPUNode(), 0) : cpu_;
}

void MatrixBenchmark::printBenchmarkHeader() const {
}

void MatrixBenchmark::reportBenchmarkInfo() const {

    if (num_worker_threads_ > 1 && g_verbose) {
        std::cout << "Load Chunk Size: ";
        switch (chunk_size_) {
            case CHUNK_32b:
                std::cout << "32-bit";
                break;
#ifdef HAS_WORD_64
            case CHUNK_64b:
                std::cout << "64-bit";
                break;
#endif
#ifdef HAS_WORD_128
            case CHUNK_128b:
                std::cout << "128-bit";
                break;
#endif
#ifdef HAS_WORD_256
            case CHUNK_256b:
                std::cout << "256-bit";
                break;
#endif
            default:
                std::cout << "UNKNOWN";
                break;
        }
        std::cout << std::endl;

        std::cout << "Load Access Pattern: ";
        switch (pattern_mode_) {
            case SEQUENTIAL:
                if (stride_size_ > 0)
                    std::cout << "forward ";
                else if (stride_size_ < 0)
                    std::cout << "reverse ";
                else
                    std::cout << "UNKNOWN ";

                if (stride_size_ == 1 || stride_size_ == -1)
                    std::cout << "sequential";
                else
                    std::cout << "strides of " << stride_size_ << " chunks";
                break;
            case RANDOM:
                std::cout << "random";
                break;
            default:
                std::cout << "UNKNOWN";
                break;
        }
        std::cout << std::endl;


        std::cout << "Load Read/Write Mode: ";
        switch (rw_mode_) {
            case READ:
                std::cout << "read";
                break;
            case WRITE:
                std::cout << "write";
                break;
            default:
                std::cout << "UNKNOWN";
                break;
        }
        std::cout << std::endl;

        std::cout << "Load number of worker threads: " << num_worker_threads_-1;
        std::cout << std::endl;
    }
}


void MatrixBenchmark::reportResults() const {

    if (use_cpu_nodes_) {
        std::cout << "CPU NUMA Node: " << cpu_node_ << " ";
    } else {
        std::cout << "CPU: " << cpu_ << " ";
    }
    std::cout << "Memory NUMA Node: " << mem_node_ << " ";
    std::cout << "Region: " << mem_region_ << " ";

    if (has_run_) {
        std::cout << "Mean: "       << mean_metric_        << " " << metric_units_ << " "; // << " and " << mean_load_metric_ << " MB/s mean imposed load (not necessarily matched)";
        std::cout << "Median: "     << median_metric_      << " " << metric_units_ << " ";
        std::cout << "LB 95% CI: "  << lower_95_CI_median_ << " " << metric_units_ << " ";
        std::cout << "UB 95% CI: "  << upper_95_CI_median_ << " " << metric_units_ << " ";
        std::cout << "iterations: " << iterations_;
        if (warning_)
            std::cout << " (WARNING)";
        std::cout << std::endl;
    }
    else
        std::cerr << "WARNING: Benchmark has not run yet. No reported results." << std::endl;
}

double MatrixBenchmark::getLoadMetricOnIter(uint32_t iter) const {
    if (has_run_ && iter - 1 <= iterations_)
        return load_metric_on_iter_[iter - 1];
    else //bad call
        return -1;
}

double MatrixBenchmark::getMeanLoadMetric() const {
    if (has_run_)
        return mean_load_metric_;
    else //bad call
        return -1;
}

void MatrixBenchmark::computeMetrics() {
    if (has_run_) {

        // Resize vector according to iterations executed.
        metric_on_iter_.resize(iterations_);

        //Compute mean
        mean_metric_ = 0;
        for (uint32_t i = 0; i < iterations_; i++)
            mean_metric_ += metric_on_iter_[i];
        mean_metric_ /= iterations_;

        //Build sorted array of metrics from each iteration
        std::vector<double> sortedMetrics = metric_on_iter_;
        std::sort(sortedMetrics.begin(), sortedMetrics.end());

        //Compute percentiles
        min_metric_ = sortedMetrics.front();
        percentile_25_metric_ = sortedMetrics[sortedMetrics.size()/4];
        percentile_75_metric_ = sortedMetrics[sortedMetrics.size()*3/4];
        median_metric_ = compute_median(metric_on_iter_);
        percentile_95_metric_ = sortedMetrics[sortedMetrics.size()*95/100];
        percentile_99_metric_ = sortedMetrics[sortedMetrics.size()*99/100];
        max_metric_ = sortedMetrics.back();

        //Compute mode
        std::map<double,uint32_t> metricCounts;
        for (uint32_t i = 0; i < iterations_; i++)
            metricCounts[metric_on_iter_[i]]++;
        mode_metric_ = 0;
        uint32_t greatest_count = 0;
        for (auto it = metricCounts.cbegin(); it != metricCounts.cend(); it++) {
            if (it->second > greatest_count)
                mode_metric_ = it->first;
        }
    }
}
