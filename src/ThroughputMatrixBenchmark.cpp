/**
 * @file
 *
 * @brief Implementation file for the ThroughputMatrixBenchmark class.
 */

//Headers
#include <LoadWorker.h>
#include <ThroughputMatrixBenchmark.h>
#include <common.h>
#include <benchmark_kernels.h>
#include <util.h>

//Libraries
#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <map>
#include <time.h>

using namespace xmem;

ThroughputMatrixBenchmark::ThroughputMatrixBenchmark(
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
    ) :
        MatrixBenchmark(
            mem_array,
            len,
            iterations,
            num_worker_threads,
            mem_node,
            mem_region,
            cpu_node,
            cpu,
            use_cpu_nodes,
            pattern_mode,
            rw_mode,
            chunk_size,
            stride_size,
            dram_power_readers,
            "MB/s",
            name,
            logfile
        )
    {}


bool ThroughputMatrixBenchmark::runCore() {
    size_t len_per_thread = len_ / num_worker_threads_; //Carve up memory space so each worker has its own area to play in

    //Set up kernel function pointers
    SequentialFunction kernel_fptr_seq = NULL;
    SequentialFunction kernel_dummy_fptr_seq = NULL;
    RandomFunction kernel_fptr_ran = NULL;
    RandomFunction kernel_dummy_fptr_ran = NULL;

    if (pattern_mode_ == SEQUENTIAL) {
        if (!determine_sequential_kernel(rw_mode_, chunk_size_, stride_size_, &kernel_fptr_seq, &kernel_dummy_fptr_seq)) {
            std::cerr << "ERROR: Failed to find appropriate benchmark kernel." << std::endl;
            return false;
        }
    } else if (pattern_mode_ == RANDOM) {
        if (!determine_random_kernel(rw_mode_, chunk_size_, &kernel_fptr_ran, &kernel_dummy_fptr_ran)) {
            std::cerr << "ERROR: Failed to find appropriate benchmark kernel." << std::endl;
            return false;
        }

        //Build pointer indices. Note that the pointers for each thread must stay within its respective region, otherwise sharing may occur.
        for (uint32_t i = 0; i < num_worker_threads_; i++) {
            if (!build_random_pointer_permutation(reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(mem_array_) + i*len_per_thread), //static casts to silence compiler warnings
                                                  reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(mem_array_) + (i+1)*len_per_thread), //static casts to silence compiler warnings
                                                  chunk_size_)) {
                std::cerr << "ERROR: Failed to build a random pointer permutation for a load generation thread!" << std::endl;
                return false;
            }
        }
    } else {
        std::cerr << "ERROR: Got an invalid pattern mode." << std::endl;
        return false;
    }

    //Set up some stuff for worker threads
    std::vector<LoadWorker*> workers;
    std::vector<Thread*> worker_threads;

    //Start power measurement
    if (g_verbose)
        std::cout << "Starting power measurement threads...";

    if (!startPowerThreads()) {
        if (g_verbose)
            std::cout << "FAIL" << std::endl;
        std::cerr << "WARNING: Failed to start power threads." << std::endl;
    } else if (g_verbose)
        std::cout << "done" << std::endl;

    //Run benchmark
    if (g_verbose)
        std::cout << "Running benchmark." << std::endl << std::endl;

    //Do a bunch of iterations of the core benchmark routine
    for (uint32_t i = 0; i < iterations_; i++) {
        //Create workers and worker threads
        for (uint32_t t = 0; t < num_worker_threads_; t++) {
            void* thread_mem_array = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(mem_array_) + t*len_per_thread);
            int32_t cpu_id = use_cpu_nodes_ ? cpu_id_in_numa_node(cpu_node_, t) : cpu_;
            if (cpu_id < 0) {
                std::cerr << "WARNING: Failed to find logical CPU " << t << " in NUMA node " << cpu_node_ << std::endl;
            }
            if (pattern_mode_ == SEQUENTIAL)
                workers.push_back(new LoadWorker(thread_mem_array,
                                                 len_per_thread,
                                                 kernel_fptr_seq,
                                                 kernel_dummy_fptr_seq,
                                                 cpu_id));
            else if (pattern_mode_ == RANDOM)
                workers.push_back(new LoadWorker(thread_mem_array,
                                                 len_per_thread,
                                                 kernel_fptr_ran,
                                                 kernel_dummy_fptr_ran,
                                                 cpu_id));
            else
                std::cerr << "WARNING: Invalid benchmark pattern mode." << std::endl;
            worker_threads.push_back(new Thread(workers[t]));
        }

        //Start worker threads! gogogo
        for (uint32_t t = 0; t < num_worker_threads_; t++)
            worker_threads[t]->create_and_start();

        //Wait for all threads to complete
        for (uint32_t t = 0; t < num_worker_threads_; t++)
            if (!worker_threads[t]->join())
                std::cerr << "WARNING: A worker thread failed to complete correctly!" << std::endl;

        //Compute throughput achieved with all workers
        uint32_t total_passes = 0;
        tick_t total_adjusted_ticks = 0;
        double avg_adjusted_ticks = 0;
        tick_t total_elapsed_dummy_ticks = 0;
        uint32_t bytes_per_pass = workers[0]->getBytesPerPass(); //all should be the same.
        bool iter_warning = false;
        for (uint32_t t = 0; t < num_worker_threads_; t++) {
            total_passes += workers[t]->getPasses();
            total_adjusted_ticks += workers[t]->getAdjustedTicks();
            total_elapsed_dummy_ticks += workers[t]->getElapsedDummyTicks();
            if (bytes_per_pass != workers[t]->getBytesPerPass()) {
                std::cerr << "WARNING: A worker thread failed to complete correctly!" << std::endl;
            }
            iter_warning |= workers[t]->hadWarning();
        }

        avg_adjusted_ticks = static_cast<double>(total_adjusted_ticks) / num_worker_threads_;

        if (iter_warning)
            warning_ = true;

        if (g_verbose) { //Report metrics for this iteration
            std::cout << "Iter " << i+1 << " had " << total_passes << " passes in total across " << num_worker_threads_ << " threads, with " << bytes_per_pass << " bytes touched per pass:";
            if (iter_warning) std::cout << " -- WARNING";
            std::cout << std::endl;

            std::cout << "...clock ticks in total across " << num_worker_threads_ << " threads == " << total_adjusted_ticks << " (adjusted by -" << total_elapsed_dummy_ticks << ")";
            if (iter_warning) std::cout << " -- WARNING";
            std::cout << std::endl;

            std::cout << "...ns in total across " << num_worker_threads_ << " threads == " << total_adjusted_ticks * g_ns_per_tick << " (adjusted by -" << total_elapsed_dummy_ticks * g_ns_per_tick << ")";
            if (iter_warning) std::cout << " -- WARNING";
            std::cout << std::endl;

            std::cout << "...sec in total across " << num_worker_threads_ << " threads == " << total_adjusted_ticks * g_ns_per_tick / 1e9 << " (adjusted by -" << total_elapsed_dummy_ticks * g_ns_per_tick / 1e9 << ")";
            if (iter_warning) std::cout << " -- WARNING";
            std::cout << std::endl;
        }

        //Compute metric for this iteration
        enumerator_metric_on_iter_[i] = (static_cast<double>(total_passes) * static_cast<double>(bytes_per_pass)) / static_cast<double>(MB);
        denominator_metric_on_iter_[i] = (static_cast<double>(avg_adjusted_ticks) * g_ns_per_tick) / 1e9;
        metric_on_iter_[i] = enumerator_metric_on_iter_[i] / denominator_metric_on_iter_[i];
        // std::cout << "latency_matrix: iter " << i << " " << enumerator_metric_on_iter_[i] << " " << enumerator_metric_units_
        //           << " per " << denominator_metric_on_iter_[i] << " " << denominator_metric_units_ << " -> " << metric_on_iter_[i]
        //           << " " << metric_units_ << std::endl;

        //Clean up workers and threads for this iteration
        for (uint32_t t = 0; t < num_worker_threads_; t++) {
            delete worker_threads[t];
            delete workers[t];
        }
        worker_threads.clear();
        workers.clear();

        if (i >= 5) {
            // 95% CI must not be computed for lower than 6 iterations of the experiment
            computeMedian(metric_on_iter_, i + 1);

            if (   (lower_95_CI_median_ >= (1 - DEV_FROM_MEDIAN) * median_metric_)
                && (upper_95_CI_median_ <= (1 + DEV_FROM_MEDIAN) * median_metric_)) {
                if (! g_log_extended) {
                    // 95% CI is within DEV_FROM_MEDIAN % of the median
                    uint32_t iterations_needed_ = i + 1;
                    // Resizing vector for keeping the results of the measurements since they are fewer than the max.
                    iterations_ = iterations_needed_;
                    metric_on_iter_.resize(iterations_needed_);
                    enumerator_metric_on_iter_.resize(iterations_needed_);
                    denominator_metric_on_iter_.resize(iterations_needed_);
                    break;
                }
            } else if (i == iterations_ - 1) {
                std::cerr << "WARNING: 95% CI did not converge within " << DEV_FROM_MEDIAN * 100 << "% of median value!" << std::endl;
            }
        } else if (i == iterations_ - 1) {
            std::cerr << "WARNING: 95% CI cannot be computed for fewer than six iterations!" << std::endl;
        }
    }

    //Stop power measurement
    if (g_verbose) {
        std::cout << std::endl;
        std::cout << "Stopping power measurement threads...";
    }

    if (!stopPowerThreads()) {
        if (g_verbose)
            std::cout << "FAIL" << std::endl;
        std::cerr << "WARNING: Failed to stop power measurement threads." << std::endl;
    } else if (g_verbose)
        std::cout << "done" << std::endl;

    if (g_log_extended) {
        for (uint32_t i = 0; i < iterations_; i++) {
            logfile_ << (use_cpu_nodes_ ? cpu_node_ : cpu_) << ","
                     << mem_node_                           << ","
                     << mem_region_                         << ","
                     << i                                   << ","
                     << metric_on_iter_[i]                  << ","
                     << metric_units_                       << std::endl;
        }
    }

    //Run metadata
    has_run_ = true;
    computeMetrics();

    return true;
}
