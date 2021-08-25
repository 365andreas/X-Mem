/**
 * @file
 *
 * @brief Implementation file for the ThroughputMatrixBenchmark class.
 */

//Headers
#include <LoadWorker.h>
#include <ThroughputMatrixBenchmark.h>
#include <benchmark_kernels.h>
#include <common.h>

//Libraries
#include <assert.h>
#include <stdio.h>


ThroughputMatrixBenchmark *initThroughputMatrixBenchmark(void *mem_array, size_t mem_array_len, uint32_t iters,
                                                         uint32_t num_worker_threads, uint32_t mem_node, uint32_t mem_region,
                                                         uint32_t cpu_node, uint32_t cpu, bool use_cpu_nodes,
                                                         pattern_mode_t pattern_mode, rw_mode_t rw_mode,
                                                         chunk_size_t chunk_size, int32_t stride_size, char *benchmark_name) {

    ThroughputMatrixBenchmark *thr_mat_bench = (ThroughputMatrixBenchmark *) malloc(sizeof(ThroughputMatrixBenchmark));

    thr_mat_bench->mat_bench = newMatrixBenchmark(mem_array, mem_array_len, iters, num_worker_threads, mem_node,
                                                  mem_region, cpu_node, cpu, use_cpu_nodes, pattern_mode, rw_mode,
                                                  chunk_size, stride_size, benchmark_name, "MB/s");

    return thr_mat_bench;
}

bool runThroughputCore(ThroughputMatrixBenchmark *thr_mat_bench) {
    size_t len_per_thread = thr_mat_bench->mat_bench->len / thr_mat_bench->mat_bench->num_worker_threads; //Carve up memory space so each worker has its own area to play in

    //Set up kernel function pointers
    SequentialFunction kernel_fptr_seq = NULL;
    SequentialFunction kernel_dummy_fptr_seq = NULL;
    RandomFunction kernel_fptr_ran = NULL;
    RandomFunction kernel_dummy_fptr_ran = NULL;

    if (thr_mat_bench->mat_bench->pattern_mode == SEQUENTIAL) {
        kernel_fptr_seq = &forwSequentialRead_Word64;
        kernel_dummy_fptr_seq = &dummy_forwSequentialLoop_Word64;
    } else if (thr_mat_bench->mat_bench->pattern_mode == RANDOM) {
        kernel_fptr_ran = &randomRead_Word64;
        kernel_dummy_fptr_ran = &dummy_randomLoop_Word64;

        //Build pointer indices. Note that the pointers for each thread must stay within its respective region, otherwise sharing may occur.
        for (uint32_t i = 0; i < thr_mat_bench->mat_bench->num_worker_threads; i++) {
            if (!build_random_pointer_permutation((void *) ((uint8_t *) (thr_mat_bench->mat_bench->mem_array) + i * len_per_thread), //static casts to silence compiler warnings
                                                  (void *) ((uint8_t *)(thr_mat_bench->mat_bench->mem_array) + (i + 1) * len_per_thread), //static casts to silence compiler warnings
                                                  CHUNK_64b)) {
                fprintf(stderr, "ERROR: Failed to build a random pointer permutation for the latency measurement thread!\n");
                return false;
            }
        }
    } else {
        fprintf(stderr, "ERROR: Got an invalid pattern mode.\n");
        return false;
    }

    uint32_t iterations         = thr_mat_bench->mat_bench->iterations;
    uint32_t num_worker_threads = thr_mat_bench->mat_bench->num_worker_threads;

    //Set up some stuff for worker threads
    LoadWorker **workers = (LoadWorker **) malloc(num_worker_threads * sizeof(LoadWorker *));
    Thread **worker_threads = (Thread **) malloc(num_worker_threads * sizeof(Thread *));


    //Run benchmark
    if (g_verbose)
        printf("Running benchmark.\n\n");

    //Do a bunch of iterations of the core benchmark routine
    for (uint32_t i = 0; i < iterations; i++) {
        //Create workers and worker threads
        for (uint32_t t = 0; t < num_worker_threads; t++) {
            void* thread_mem_array = (void *) ((uint8_t *) thr_mat_bench->mat_bench->mem_array + t * len_per_thread);
            int32_t cpu_id = getCPUId(thr_mat_bench->mat_bench);
            if (cpu_id < 0) {
                fprintf(stderr, "WARNING: Failed to find logical CPU %d\n", t);
            }
            if (thr_mat_bench->mat_bench->pattern_mode == SEQUENTIAL)
                workers[t] = newLoadWorkerSeq(thread_mem_array,
                                              len_per_thread,
                                              kernel_fptr_seq,
                                              kernel_dummy_fptr_seq,
                                              cpu_id);
            else if (thr_mat_bench->mat_bench->pattern_mode == RANDOM)
                workers[t] = newLoadWorkerRan(thread_mem_array,
                                              len_per_thread,
                                              kernel_fptr_ran,
                                              kernel_dummy_fptr_ran,
                                              cpu_id);
            else
                fprintf(stderr, "WARNING: Invalid benchmark pattern mode.\n");
            worker_threads[t] = newThread(workers[t]);
        }

        //Start worker threads! gogogo
        for (uint32_t t = 0; t < num_worker_threads; t++)
            create_and_start(worker_threads[t], &runLaunchpadLoad);

        //Wait for all threads to complete
        for (uint32_t t = 0; t < num_worker_threads; t++)
            if (! join(worker_threads[t]))
                fprintf(stderr, "WARNING: A worker thread failed to complete correctly!\n");

         //Compute metrics for this iteration
        bool iterwarning = false;

        //Compute throughput achieved with all workers
        uint32_t total_passes = 0;
        tick_t total_adjusted_ticks = 0;
        double avg_adjusted_ticks = 0;
        tick_t total_elapsed_dummy_ticks = 0;
        uint32_t bytes_per_pass = getBytesPerPass(workers[0]->mem_worker); //all should be the same.
        bool iter_warning = false;
        for (uint32_t t = 0; t < num_worker_threads; t++) {
            total_passes              += getPasses(workers[t]->mem_worker);
            total_adjusted_ticks      += getAdjustedTicks(workers[t]->mem_worker);
            total_elapsed_dummy_ticks += getElapsedDummyTicks(workers[t]->mem_worker);
            if (bytes_per_pass != getBytesPerPass(workers[t]->mem_worker)) {
                fprintf(stderr, "WARNING: A worker thread failed to complete correctly!\n");
            }
            iter_warning |= hadWarning(workers[t]->mem_worker);
        }

        avg_adjusted_ticks = ((double) total_adjusted_ticks) / num_worker_threads;

        if (iter_warning)
            thr_mat_bench->mat_bench->warning_ = true;

        if (g_verbose) { //Report metrics for this iteration
            printf("Iter %d had %d passes in total across %d threads, with %d bytes touched per pass:", i + 1, total_passes, num_worker_threads, bytes_per_pass);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");

            printf("...clock ticks in total across %d threads == %ld (adjusted by -%ld)", num_worker_threads, total_adjusted_ticks, total_elapsed_dummy_ticks);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");

            printf("...ns in total across %d threads == %f (adjusted by -%f)", num_worker_threads, total_adjusted_ticks * g_ns_per_tick , total_elapsed_dummy_ticks * g_ns_per_tick);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");

            printf("...sec in total across %d threads == %f (adjusted by -%f)", num_worker_threads, total_adjusted_ticks * g_ns_per_tick / 1e9, total_elapsed_dummy_ticks * g_ns_per_tick / 1e9);
            if (iterwarning) printf(" -- WARNING");
            printf("\n");
        }

        //Compute metric for this iteration
        thr_mat_bench->mat_bench->enumerator_metric_on_iter_[i] = ((double) total_passes * (double) bytes_per_pass) / (double) MB;
        thr_mat_bench->mat_bench->denominator_metric_on_iter_[i] = ((double) avg_adjusted_ticks * g_ns_per_tick) / 1e9;
        thr_mat_bench->mat_bench->metric_on_iter_[i] = thr_mat_bench->mat_bench->enumerator_metric_on_iter_[i] / thr_mat_bench->mat_bench->denominator_metric_on_iter_[i];
        if (g_verbose)
            printf("throughput_matrix: iter %d -> %f %s\n", i + 1, thr_mat_bench->mat_bench->metric_on_iter_[i], thr_mat_bench->mat_bench->metric_units);

        //Clean up workers and threads for this iteration
        for (uint32_t t = 0; t < num_worker_threads; t++) {
            deleteLoadWorker(workers[t]);
            free(worker_threads[t]);
        }

        if (i >= 5) {
            // 95% CI must not be computed for lower than 6 iterations of the experiment
            computeMedian(thr_mat_bench->mat_bench, i + 1);

            double lowest_bound  = (1 - DEV_FROM_MEDIAN) * thr_mat_bench->mat_bench->median_metric_;
            double highest_bound = (1 + DEV_FROM_MEDIAN) * thr_mat_bench->mat_bench->median_metric_;
            bool lower_CI_in = (int) (thr_mat_bench->mat_bench->lower_95_CI_median_ - lowest_bound) >= 0; // float comparison (fucomip) is not supported on k1om
            bool upper_CI_in = (int) (thr_mat_bench->mat_bench->upper_95_CI_median_ - highest_bound) <= 0;
            if (lower_CI_in && upper_CI_in) {
                // 95% CI is within DEV_FROM_MEDIAN % of the median
                if (! g_log_extended) {
                    // stop if extended measurements are not enabled
                    uint32_t iterations_needed_ = i + 1;
                    // Resizing vector for keeping the results of the measurements since they are fewer than the max.
                    thr_mat_bench->mat_bench->iterations = iterations_needed_;
                    break;
                }
            } else if (i ==  thr_mat_bench->mat_bench->iterations - 1) {
                fprintf(stderr, "WARNING: 95%% CI did not converge within %f%% of median value!\n", DEV_FROM_MEDIAN * 100);
            }
        } else if (i == iterations - 1) {
            fprintf(stderr, "WARNING: 95%% CI cannot be computed for fewer than six iterations!\n");
        }
    }

    // if (g_log_extended) {
    //     for (uint32_t i = 0; i < iterations_; i++) {
    //         logfile_ << (use_cpu_nodes_ ? cpu_node_ : cpu_) << ","
    //                  << mem_node_                           << ","
    //                  << mem_region_                         << ","
    //                  << i                                   << ","
    //                  << metric_on_iter_[i]                  << ","
    //                  << metric_units_                       << std::endl;
    //     }
    // }

    free(worker_threads);
    free(workers);

    //Run metadata
    thr_mat_bench->mat_bench->has_run_ = true;
    computeMetrics(thr_mat_bench->mat_bench);

    return true;
}

bool runThroughput(ThroughputMatrixBenchmark *thr_mat_bench) {

    // if (thr_mat_bench->mat_bench->has_run_) //A benchmark should only be run once per object
    //     return false;

    // printBenchmarkHeader(thr_mat_bench->mat_bench);
    // reportBenchmarkInfo(thr_mat_bench->mat_bench);

    //Write to all of the memory region of interest to make sure
    //pages are resident in physical memory and are not shared
    forwSequentialWrite_Word32(thr_mat_bench->mat_bench->mem_array,
                               (void *) ((uint8_t *) (thr_mat_bench->mat_bench->mem_array) +
                                         thr_mat_bench->mat_bench->len));

    bool success = runThroughputCore(thr_mat_bench);
    if (success) {
        return true;
    } else {
        fprintf(stderr, "WARNING: Benchmark %s failed!\n", thr_mat_bench->mat_bench->name);
        return false;
    }
}
