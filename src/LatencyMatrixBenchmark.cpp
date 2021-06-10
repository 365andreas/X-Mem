/**
 * @file
 *
 * @brief Implementation file for the LatencyMatrixBenchmark class.
 */

//Headers
#include <LatencyMatrixBenchmark.h>
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

LatencyMatrixBenchmark::LatencyMatrixBenchmark(
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
            "ns/access",
            name,
            logfile
        )
    {}

bool LatencyMatrixBenchmark::runCore() {
    size_t len_per_thread = len_ / num_worker_threads_; //Carve up memory space so each worker has its own area to play in

    //Set up latency measurement kernel function pointers
    RandomFunction lat_kernel_fptr = &chasePointers;
    RandomFunction lat_kernel_dummy_fptr = &dummy_chasePointers;

    //Initialize memory regions for all threads by writing to them, causing the memory to be physically resident.
    forwSequentialWrite_Word32(mem_array_,
                               reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(mem_array_)+len_)); //static casts to silence compiler warnings

    //Build pointer indices for random-access latency thread. We assume that latency thread is the first one, so we use beginning of memory region.
    if (!build_random_pointer_permutation(mem_array_,
                                       reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(mem_array_)+len_per_thread), //static casts to silence compiler warnings
#ifndef HAS_WORD_64 //special case: 32-bit architectures
                                       CHUNK_32b)) {
#endif
#ifdef HAS_WORD_64
                                       CHUNK_64b)) {
#endif
        std::cerr << "ERROR: Failed to build a random pointer permutation for the latency measurement thread!" << std::endl;
        return false;
    }

    //Set up load generation kernel function pointers
    SequentialFunction load_kernel_fptr_seq = NULL;
    SequentialFunction load_kernel_dummy_fptr_seq = NULL;
    RandomFunction load_kernel_fptr_ran = NULL;
    RandomFunction load_kernel_dummy_fptr_ran = NULL;
    if (num_worker_threads_ > 1) { //If we only have one worker thread, it is used for latency measurement only, and no load threads will be used.
        if (pattern_mode_ == SEQUENTIAL) {
            if (!determine_sequential_kernel(rw_mode_, chunk_size_, stride_size_, &load_kernel_fptr_seq, &load_kernel_dummy_fptr_seq)) {
                std::cerr << "ERROR: Failed to find appropriate benchmark kernel." << std::endl;
                return false;
            }
        } else if (pattern_mode_ == RANDOM) {
            if (!determine_random_kernel(rw_mode_, chunk_size_, &load_kernel_fptr_ran, &load_kernel_dummy_fptr_ran)) {
                std::cerr << "ERROR: Failed to find appropriate benchmark kernel." << std::endl;
                return false;
            }

            //Build pointer indices for random-access load threads. Note that the pointers for each load thread must stay within its respective region, otherwise sharing may occur.
            for (uint32_t i = 1; i < num_worker_threads_; i++) {
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
    }

    //Set up some stuff for worker threads
    std::vector<MemoryWorker*> workers;
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

        //Create load workers and load worker threads
        for (uint32_t t = 0; t < num_worker_threads_; t++) {
            void* thread_mem_array = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(mem_array_) + t*len_per_thread);
            int32_t cpu_id = use_cpu_nodes_ ? cpu_id_in_numa_node(cpu_node_, t) : getCPUId();
            if (cpu_id < 0) {
                if (use_cpu_nodes_)
                    std::cerr << "WARNING: Failed to find logical CPU " << t << " in NUMA node " << cpu_node_ << std::endl;
                else
                    std::cerr << "WARNING: Failed to find logical CPU " << t << std::endl;
            }
            if (t == 0) { //special case: thread 0 is always latency thread
                workers.push_back(new LatencyWorker(thread_mem_array,
                                                    len_per_thread,
                                                    lat_kernel_fptr,
                                                    lat_kernel_dummy_fptr,
                                                    cpu_id));
            } else {
                if (pattern_mode_ == SEQUENTIAL)
                    workers.push_back(new LoadWorker(thread_mem_array,
                                                     len_per_thread,
                                                     load_kernel_fptr_seq,
                                                     load_kernel_dummy_fptr_seq,
                                                     cpu_id));
                else if (pattern_mode_ == RANDOM)
                    workers.push_back(new LoadWorker(thread_mem_array,
                                                     len_per_thread,
                                                     load_kernel_fptr_ran,
                                                     load_kernel_dummy_fptr_ran,
                                                     cpu_id));
                else
                    std::cerr << "WARNING: Invalid benchmark pattern mode." << std::endl;
            }
            worker_threads.push_back(new Thread(workers[t]));
        }

        //Start worker threads! gogogo
        for (uint32_t t = 0; t < num_worker_threads_; t++)
            worker_threads[t]->create_and_start();

        //Wait for all threads to complete
        for (uint32_t t = 0; t < num_worker_threads_; t++)
            if (!worker_threads[t]->join())
                std::cerr << "WARNING: A worker thread failed to complete correctly!" << std::endl;

        //Compute metrics for this iteration
        bool iterwarning = false;

        //Compute latency metric
        uint32_t lat_passes = workers[0]->getPasses();
        tick_t lat_adjusted_ticks = workers[0]->getAdjustedTicks();
        tick_t lat_elapsed_dummy_ticks = workers[0]->getElapsedDummyTicks();
        uint32_t lat_bytes_per_pass = workers[0]->getBytesPerPass();
        uint32_t lat_accesses_per_pass = lat_bytes_per_pass / 8;
        iterwarning |= workers[0]->hadWarning();

        //Compute throughput generated by load threads
        uint32_t load_total_passes = 0;
        tick_t load_total_adjusted_ticks = 0;
        tick_t load_total_elapsed_dummy_ticks = 0;
        uint32_t load_bytes_per_pass = 0;
        double load_avg_adjusted_ticks = 0;
        for (uint32_t t = 1; t < num_worker_threads_; t++) {
            load_total_passes += workers[t]->getPasses();
            load_total_adjusted_ticks += workers[t]->getAdjustedTicks();
            load_total_elapsed_dummy_ticks += workers[t]->getElapsedDummyTicks();
            load_bytes_per_pass = workers[t]->getBytesPerPass(); //all should be the same.
            iterwarning |= workers[t]->hadWarning();
        }

        //Compute load metrics for this iteration
        load_avg_adjusted_ticks = static_cast<double>(load_total_adjusted_ticks) / (num_worker_threads_-1);
        if (num_worker_threads_ > 1)
            load_metric_on_iter_[i] = (((static_cast<double>(load_total_passes) * static_cast<double>(load_bytes_per_pass)) / static_cast<double>(MB)))   /  ((load_avg_adjusted_ticks * g_ns_per_tick) / 1e9);

        if (iterwarning)
            warning_ = true;

        if (g_verbose) { //Report metrics for this iteration
            //Latency thread
            std::cout << "Iter " << i+1 << " had " << lat_passes << " latency measurement passes, with " << lat_accesses_per_pass << " accesses per pass:";
            if (iterwarning) std::cout << " -- WARNING";
            std::cout << std::endl;

            std::cout << "...lat clock ticks == " << lat_adjusted_ticks << " (adjusted by -" << lat_elapsed_dummy_ticks << ")";
            if (iterwarning) std::cout << " -- WARNING";
            std::cout << std::endl;

            std::cout << "...lat ns == " << lat_adjusted_ticks * g_ns_per_tick << " (adjusted by -" << lat_elapsed_dummy_ticks * g_ns_per_tick << ")";
            if (iterwarning) std::cout << " -- WARNING";
            std::cout << std::endl;

            std::cout << "...lat sec == " << lat_adjusted_ticks * g_ns_per_tick / 1e9 << " (adjusted by -" << lat_elapsed_dummy_ticks * g_ns_per_tick / 1e9 << ")";
            if (iterwarning) std::cout << " -- WARNING";
            std::cout << std::endl;

            //Load threads
            if (num_worker_threads_ > 1) {
                std::cout << "Iter " << i+1 << " had " << load_total_passes << " total load generation passes, with " << load_bytes_per_pass << " bytes per pass:";
                if (iterwarning) std::cout << " -- WARNING";
                std::cout << std::endl;

                std::cout << "...load total clock ticks across " << num_worker_threads_-1 << " threads == " << load_total_adjusted_ticks << " (adjusted by -" << load_total_elapsed_dummy_ticks << ")";
                if (iterwarning) std::cout << " -- WARNING";
                std::cout << std::endl;

                std::cout << "...load total ns across " << num_worker_threads_-1 << " threads == " << load_total_adjusted_ticks * g_ns_per_tick << " (adjusted by -" << load_total_elapsed_dummy_ticks * g_ns_per_tick << ")";
                if (iterwarning) std::cout << " -- WARNING";
                std::cout << std::endl;

                std::cout << "...load total sec across " << num_worker_threads_-1 << " threads == " << load_total_adjusted_ticks * g_ns_per_tick / 1e9 << " (adjusted by -" << load_total_elapsed_dummy_ticks * g_ns_per_tick / 1e9 << ")";
                if (iterwarning) std::cout << " -- WARNING";
                std::cout << std::endl;
            }
        }

        //Compute overall metrics for this iteration
        enumerator_metric_on_iter_[i]  = static_cast<double>(lat_adjusted_ticks * g_ns_per_tick);
        denominator_metric_on_iter_[i] = static_cast<double>(lat_accesses_per_pass * lat_passes);
        metric_on_iter_[i] = static_cast<double>(lat_adjusted_ticks * g_ns_per_tick)  /  static_cast<double>(lat_accesses_per_pass * lat_passes);
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

    //Get mean load metrics -- these aren't part of Benchmark class thus not covered by computeMetrics()
    for (uint32_t i = 0; i < iterations_; i++)
        mean_load_metric_ += load_metric_on_iter_[i];
    mean_load_metric_ /= static_cast<double>(iterations_);

    return true;
}
