/**
 * @file
 *
 * @brief Implementation file for the LatencyWorker struct.
 */

//Headers
#include <LatencyWorker.h>
#include <benchmark_kernels.h>
#include <common.h>

//Libraries
#include <stdio.h>
#include <unistd.h>

LatencyWorker *newLatencyWorker(void* mem_array, size_t len, RandomFunction kernel_fptr,
                                RandomFunction kernel_dummy_fptr, int32_t cpu_affinity) {

    LatencyWorker *lat_worker = (LatencyWorker *) malloc(sizeof(LatencyWorker));

    lat_worker->mem_worker = newMemoryWorker(mem_array, len, cpu_affinity);

    lat_worker->kernel_fptr       = kernel_fptr;
    lat_worker->kernel_dummy_fptr = kernel_dummy_fptr;

    return lat_worker;
}

void deleteLatencyWorker(LatencyWorker *lat_worker) {
    deleteMemoryWorker(lat_worker->mem_worker);
    free(lat_worker);
}

void runLatWorker(LatencyWorker *lat_worker) {
    //Set up relevant state -- localized to this thread's stack
    int32_t cpu_affinity = 0;
    RandomFunction kernel_fptr = NULL;
    RandomFunction kernel_dummy_fptr = NULL;
    uintptr_t* next_address = NULL;
    uint32_t bytes_per_pass = 0;
    uint32_t passes = 0;
    uint32_t p = 0;
    tick_t start_tick = 0;
    tick_t stop_tick = 0;
    tick_t elapsed_ticks = 0;
    tick_t elapsed_dummy_ticks = 0;
    tick_t adjusted_ticks = 0;
    bool warning = false;
    void* mem_array = NULL;
    size_t len = 0;
    tick_t target_ticks = g_ticks_per_ms * BENCHMARK_DURATION_MS; //Rough target run duration in ticks

    //Grab relevant setup state thread-safely and keep it local
    if (acquireLock(lat_worker->mem_worker->runnable, -1)) {
        mem_array = lat_worker->mem_worker->mem_array_;
        len = lat_worker->mem_worker->len_;
        bytes_per_pass = LATENCY_BENCHMARK_UNROLL_LENGTH * 8;
        cpu_affinity = lat_worker->mem_worker->cpu_affinity_;
        kernel_fptr = lat_worker->kernel_fptr;
        kernel_dummy_fptr = lat_worker->kernel_dummy_fptr;
        releaseLock(lat_worker->mem_worker->runnable);
    }

    // Set processor affinity
    bool locked = lock_thread_to_cpu(cpu_affinity);
    if (! locked)
        fprintf(stderr, "WARNING: Failed to lock thread to logical CPU %d! Results may not be correct.\n", cpu_affinity);

    // Increase scheduling priority
    if (! boost_scheduling_priority())
        fprintf(stderr, "WARNING: Failed to boost scheduling priority. Perhaps running in Administrator mode would help.\n");

    //Prime memory
    for (uint32_t i = 0; i < 4; i++) {
        void* prime_start_address = mem_array;
        void* prime_end_address = (void *) ((uint8_t *) mem_array + len);
        forwSequentialRead_Word32(prime_start_address, prime_end_address); //dependent reads on the memory, make sure caches are ready, coherence, etc...
    }

    //Run benchmark
    //Run actual version of function and loop overhead
    next_address = (uintptr_t *) mem_array;
    while (elapsed_ticks < target_ticks) {
        start_tick = start_timer();
        UNROLL256((*kernel_fptr)(next_address, &next_address, 0);)
        stop_tick = stop_timer();
        elapsed_ticks += (stop_tick - start_tick);
        passes+=256;
    }

    //Run dummy version of function and loop overhead
    next_address = (uintptr_t *) mem_array;
    while (p < passes) {
        start_tick = start_timer();
        UNROLL256((*kernel_dummy_fptr)(next_address, &next_address, 0);)
        stop_tick = stop_timer();
        elapsed_dummy_ticks += (stop_tick - start_tick);
        p+=256;
    }

    adjusted_ticks = elapsed_ticks - elapsed_dummy_ticks;

    //Warn if something looks fishy
    if (elapsed_dummy_ticks >= elapsed_ticks || elapsed_ticks < MIN_ELAPSED_TICKS || adjusted_ticks < elapsed_ticks / 2)
        warning = true;

    // Unset processor affinity
    if (locked) {
        bool unlocked;
        unlocked = unlock_thread_to_numa_node();
        if (! unlocked)
            fprintf(stderr, "WARNING: Failed to unlock threads!\n");
    }

    // Revert thread priority
    if (! revert_scheduling_priority())
        fprintf(stderr, "WARNING: Failed to revert scheduling priority. Perhaps running in Administrator mode would help.\n");

    //Update the object state thread-safely
    if (acquireLock(lat_worker->mem_worker->runnable, -1)) {
        lat_worker->mem_worker->adjusted_ticks_ = adjusted_ticks;
        lat_worker->mem_worker->elapsed_ticks_ = elapsed_ticks;
        lat_worker->mem_worker->elapsed_dummy_ticks_ = elapsed_dummy_ticks;
        lat_worker->mem_worker->warning_ = warning;
        lat_worker->mem_worker->bytes_per_pass_ = bytes_per_pass;
        lat_worker->mem_worker->completed_ = true;
        lat_worker->mem_worker->passes_ = passes;
        releaseLock(lat_worker->mem_worker->runnable);
    }
}
