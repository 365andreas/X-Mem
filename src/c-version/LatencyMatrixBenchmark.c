/**
 * @file
 *
 * @brief Implementation file for the LatencyMatrixBenchmark struct.
 */

//Headers
#include <LatencyMatrixBenchmark.h>
#include <common.h>
#include <benchmark_kernels.h>
#include <MemoryWorker.h>
#include <LatencyWorker.h>
// #include <LoadWorker.h>

//Libraries
#include <assert.h>
#include <stdio.h>
// #include <time.h>
// #include <util.h>


LatencyMatrixBenchmark *initLatencyMatrixBenchmark(void *mem_array, size_t mem_array_len, uint32_t iters,
                                                   uint32_t num_worker_threads, uint32_t mem_node, uint32_t mem_region,
                                                   uint32_t cpu_node, uint32_t cpu, bool use_cpu_nodes,
                                                   pattern_mode_t pattern_mode, rw_mode_t rw_mode,
                                                   chunk_size_t chunk_size, int32_t stride_size, char *benchmark_name) {

    LatencyMatrixBenchmark *lat_mat_bench = (LatencyMatrixBenchmark *) malloc(sizeof(LatencyMatrixBenchmark));

    lat_mat_bench->mat_bench = newMatrixBenchmark(mem_array, mem_array_len, iters, num_worker_threads, mem_node,
                                                  mem_region, cpu_node, cpu, use_cpu_nodes, pattern_mode, rw_mode,
                                                  chunk_size, stride_size, benchmark_name);

    return lat_mat_bench;
}

bool run(LatencyMatrixBenchmark *lat_mat_bench) {

    // if (lat_mat_bench->mat_bench->has_run_) //A benchmark should only be run once per object
    //     return false;

    // printBenchmarkHeader(lat_mat_bench->mat_bench);
    // reportBenchmarkInfo(lat_mat_bench->mat_bench);

    //Write to all of the memory region of interest to make sure
    //pages are resident in physical memory and are not shared
    forwSequentialWrite_Word32(lat_mat_bench->mat_bench->mem_array,
                               (void *) ((uint8_t *) (lat_mat_bench->mat_bench->mem_array) + lat_mat_bench->mat_bench->len));

    bool success = runCore(lat_mat_bench);
    if (success) {
        return true;
    } else {
        fprintf(stderr, "WARNING: Benchmark %s failed!\n", lat_mat_bench->mat_bench->name);
        return false;
    }

}

bool runCore(LatencyMatrixBenchmark *lat_mat_bench) {
    size_t len_per_thread = lat_mat_bench->mat_bench->len / lat_mat_bench->mat_bench->num_worker_threads; //Carve up memory space so each worker has its own area to play in

    //Set up latency measurement kernel function pointers
    RandomFunction lat_kernel_fptr = &chasePointers;
    RandomFunction lat_kernel_dummy_fptr = &dummy_chasePointers;

    //Initialize memory regions for all threads by writing to them, causing the memory to be physically resident.
    forwSequentialWrite_Word32(lat_mat_bench->mat_bench->mem_array,
                               (void *) ((uint8_t *) (lat_mat_bench->mat_bench->mem_array) + lat_mat_bench->mat_bench->len)); //static casts to silence compiler warnings

    //Build pointer indices for random-access latency thread. We assume that latency thread is the first one, so we use beginning of memory region.
    if (! build_random_pointer_permutation(lat_mat_bench->mat_bench->mem_array,
                                           (void *) ((uint8_t *) (lat_mat_bench->mat_bench->mem_array) + len_per_thread), //static casts to silence compiler warnings
                                           CHUNK_64b)) {
        fprintf(stderr, "ERROR: Failed to build a random pointer permutation for the latency measurement thread!\n");
        return false;
    }

    //Set up load generation kernel function pointers
    SequentialFunction load_kernel_fptr_seq = NULL;
    SequentialFunction load_kernel_dummy_fptr_seq = NULL;
    RandomFunction load_kernel_fptr_ran = NULL;
    RandomFunction load_kernel_dummy_fptr_ran = NULL;

    uint32_t iterations         = lat_mat_bench->mat_bench->iterations;
    uint32_t num_worker_threads = lat_mat_bench->mat_bench->num_worker_threads;

    if (num_worker_threads > 1) {
        fprintf(stderr, "ERROR: Worker threads are more than 1 in latency matrix benchmark.\n");
        return false;
    }

    //Set up some stuff for worker threads
    LatencyWorker **workers = (LatencyWorker **) malloc(iterations * num_worker_threads * sizeof(LatencyWorker *));
    Thread **worker_threads = (Thread **) malloc(iterations * num_worker_threads * sizeof(Thread *));

    //Run benchmark
    if (g_verbose)
        printf("Running benchmark.\n\n");

    //Do a bunch of iterations of the core benchmark routine
    for (uint32_t i = 0; i < iterations; i++) {

        //Create load workers and load worker threads
        for (uint32_t t = 0; t < num_worker_threads; t++) {
            void* thread_mem_array = (void *) ((uint8_t *) lat_mat_bench->mat_bench->mem_array + t * len_per_thread);
            int32_t cpu_id = getCPUId(lat_mat_bench->mat_bench);
            if (cpu_id < 0) {
                fprintf(stderr, "WARNING: Failed to find logical CPU %d\n", t);
            }
            if (t == 0) { //special case: thread 0 is always latency thread
                workers[i * num_worker_threads + t] = newLatencyWorker(thread_mem_array,
                                                                       len_per_thread,
                                                                       lat_kernel_fptr,
                                                                       lat_kernel_dummy_fptr,
                                                                       cpu_id);
            } else {
                fprintf(stderr, "ERROR: Worker threads are more than 1 in latency matrix benchmark.\n");
                return false;
            }
            worker_threads[i * num_worker_threads + t] = newThread(workers[t]);
        }

        //Start worker threads! gogogo
        for (uint32_t t = 0; t < num_worker_threads; t++)
            create_and_start(worker_threads[t]);

        //Wait for all threads to complete
        for (uint32_t t = 0; t < num_worker_threads; t++)
            if (! join(worker_threads[t]))
                fprintf(stderr, "WARNING: A worker thread failed to complete correctly!\n");

        //Compute metrics for this iteration
        bool iterwarning = false;

        //Compute latency metric
        uint32_t lat_passes = getPasses(workers[0]->mem_worker);
        tick_t lat_adjusted_ticks = getAdjustedTicks(workers[0]->mem_worker);
        tick_t lat_elapsed_dummy_ticks = getElapsedDummyTicks(workers[0]->mem_worker);
        uint32_t lat_bytes_per_pass = getBytesPerPass(workers[0]->mem_worker);
        uint32_t lat_accesses_per_pass = lat_bytes_per_pass / 8;
        iterwarning |= hadWarning(workers[0]->mem_worker);

        if (iterwarning)
            lat_mat_bench->mat_bench->warning_ = true;

        if (g_verbose) { //Report metrics for this iteration
            //Latency thread
            printf("Iter %d had %d latency measurement passes, with %d accesses per pass:", i + 1, lat_passes, lat_accesses_per_pass);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");

            printf("...lat clock ticks == %ld (adjusted by -%ld)", lat_adjusted_ticks, lat_elapsed_dummy_ticks);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");

            printf("...lat ns == %f (adjusted by -%f)", lat_adjusted_ticks * g_ns_per_tick, lat_elapsed_dummy_ticks * g_ns_per_tick);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");

            printf("...lat sec == %f (adjusted by -%f)", lat_adjusted_ticks * g_ns_per_tick / 1e9, lat_elapsed_dummy_ticks * g_ns_per_tick / 1e9);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");

        }

        //Compute overall metrics for this iteration
        lat_mat_bench->mat_bench->enumerator_metric_on_iter_[i]  = (double) (lat_adjusted_ticks * g_ns_per_tick);
        lat_mat_bench->mat_bench->denominator_metric_on_iter_[i] = (double) (lat_accesses_per_pass * lat_passes);
        lat_mat_bench->mat_bench->metric_on_iter_[i] = (double) (lat_adjusted_ticks * g_ns_per_tick)  /  (double) (lat_accesses_per_pass * lat_passes);
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
