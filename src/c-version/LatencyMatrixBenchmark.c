/**
 * @file
 *
 * @brief Implementation file for the LatencyMatrixBenchmark struct.
 */

//Headers
#include <LatencyMatrixBenchmark.h>
#include <LatencyWorker.h>
#include <MemoryWorker.h>
#include <benchmark_kernels.h>
#include <common.h>

//Libraries
#include <assert.h>
#include <stdio.h>


LatencyMatrixBenchmark *initLatencyMatrixBenchmark(void *mem_array, size_t mem_array_len, uint32_t iters,
                                                   uint32_t num_worker_threads, uint32_t mem_region, uint32_t cpu,
                                                   pattern_mode_t pattern_mode, rw_mode_t rw_mode,
                                                   chunk_size_t chunk_size, int32_t stride_size, char *benchmark_name) {

    LatencyMatrixBenchmark *lat_mat_bench = (LatencyMatrixBenchmark *) malloc(sizeof(LatencyMatrixBenchmark));

    lat_mat_bench->mat_bench = newMatrixBenchmark(mem_array, mem_array_len, iters, num_worker_threads, mem_region, cpu,
                                                  pattern_mode, rw_mode, chunk_size, stride_size, benchmark_name,
                                                  "ns/access");

    return lat_mat_bench;
}

bool runLatencyCore(LatencyMatrixBenchmark *lat_mat_bench) {
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
    LatencyWorker **workers = (LatencyWorker **) malloc(num_worker_threads * sizeof(LatencyWorker *));
    Thread **worker_threads = (Thread **) malloc(num_worker_threads * sizeof(Thread *));

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
                workers[t] = newLatencyWorker(thread_mem_array,
                                              len_per_thread,
                                              lat_kernel_fptr,
                                              lat_kernel_dummy_fptr,
                                              cpu_id);
            } else {
                fprintf(stderr, "ERROR: Worker threads are more than 1 in latency matrix benchmark.\n");
                return false;
            }
            worker_threads[t] = newThread(workers[t]);
        }

        //Start worker threads! gogogo
        for (uint32_t t = 0; t < num_worker_threads; t++)
            create_and_start(worker_threads[t], &runLaunchpadLatency);

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
        if (g_verbose)
            printf("latency_matrix: iter %d -> %f %s\n", i + 1, lat_mat_bench->mat_bench->metric_on_iter_[i], lat_mat_bench->mat_bench->metric_units);

        //Clean up workers and threads for this iteration
        for (uint32_t t = 0; t < num_worker_threads; t++) {
            deleteLatencyWorker(workers[t]);
            free(worker_threads[t]);
        }

        if (i >= 5) {
            // 95% CI must not be computed for lower than 6 iterations of the experiment
            computeMedian(lat_mat_bench->mat_bench, i + 1);

            double lowest_bound  = (1 - DEV_FROM_MEDIAN) * lat_mat_bench->mat_bench->median_metric_;
            double highest_bound = (1 + DEV_FROM_MEDIAN) * lat_mat_bench->mat_bench->median_metric_;
            bool lower_CI_in = (int) (lat_mat_bench->mat_bench->lower_95_CI_median_ - lowest_bound) >= 0; // float comparison (fucomip) is not supported on k1om
            bool upper_CI_in = (int) (lat_mat_bench->mat_bench->upper_95_CI_median_ - highest_bound) <= 0;
            if (lower_CI_in && upper_CI_in) {
                // 95% CI is within DEV_FROM_MEDIAN % of the median
                if (! g_log_extended) {
                    // stop if extended measurements are not enabled
                    uint32_t iterations_needed_ = i + 1;
                    // Resizing vector for keeping the results of the measurements since they are fewer than the max.
                    lat_mat_bench->mat_bench->iterations = iterations_needed_;
                    break;
                }
            } else if (i == lat_mat_bench->mat_bench->iterations - 1) {
                fprintf(stderr, "WARNING: 95%% CI did not converge within %f%% of median value!\n", DEV_FROM_MEDIAN * 100);
            }
        } else if (i == iterations - 1) {
            fprintf(stderr, "WARNING: 95%% CI cannot be computed for fewer than six iterations!\n");
        }
    }

    free(worker_threads);
    free(workers);

    //Run metadata
    lat_mat_bench->mat_bench->has_run_ = true;
    computeMetrics(lat_mat_bench->mat_bench);

    return true;
}

bool runLatency(LatencyMatrixBenchmark *lat_mat_bench) {

    // if (lat_mat_bench->mat_bench->has_run_) //A benchmark should only be run once per object
    //     return false;

    // printBenchmarkHeader(lat_mat_bench->mat_bench);
    // reportBenchmarkInfo(lat_mat_bench->mat_bench);

    //Write to all of the memory region of interest to make sure
    //pages are resident in physical memory and are not shared
    forwSequentialWrite_Word32(lat_mat_bench->mat_bench->mem_array,
                               (void *) ((uint8_t *) (lat_mat_bench->mat_bench->mem_array) + lat_mat_bench->mat_bench->len));

    bool success = runLatencyCore(lat_mat_bench);
    if (success) {
        return true;
    } else {
        fprintf(stderr, "WARNING: Benchmark %s failed!\n", lat_mat_bench->mat_bench->name);
        return false;
    }

}
