/**
 * @file
 *
 * @brief Implementation file for the LoadWorker class.
 */

//Headers
#include <LoadWorker.h>
#include <benchmark_kernels.h>
#include <common.h>

//Libraries
#include <stdio.h>
#include <unistd.h>

LoadWorker *newLoadWorkerSeq(void* mem_array, size_t len, SequentialFunction kernel_fptr, SequentialFunction kernel_dummy_fptr, int32_t cpu_affinity) {

    LoadWorker *load_worker = (LoadWorker *) malloc(sizeof(LoadWorker));

    load_worker->mem_worker = newMemoryWorker(mem_array, len, cpu_affinity);

    load_worker->use_sequential_kernel_fptr_ = true;
    load_worker->kernel_fptr_seq_            = kernel_fptr;
    load_worker->kernel_dummy_fptr_seq_      = kernel_dummy_fptr;
    load_worker->kernel_fptr_ran_            = NULL;
    load_worker->kernel_dummy_fptr_ran_      = NULL;

    return load_worker;
}

LoadWorker *newLoadWorkerRan(void* mem_array, size_t len, RandomFunction kernel_fptr, RandomFunction kernel_dummy_fptr, int32_t cpu_affinity) {

    LoadWorker *load_worker = (LoadWorker *) malloc(sizeof(LoadWorker));

    load_worker->mem_worker = newMemoryWorker(mem_array, len, cpu_affinity);

    load_worker->use_sequential_kernel_fptr_ = false;
    load_worker->kernel_fptr_seq_            = NULL;
    load_worker->kernel_dummy_fptr_seq_      = NULL;
    load_worker->kernel_fptr_ran_            = kernel_fptr;
    load_worker->kernel_dummy_fptr_ran_      = kernel_dummy_fptr;

    return load_worker;
}

void deleteLoadWorker(LoadWorker *load_worker) {
    deleteMemoryWorker(load_worker->mem_worker);
    free(load_worker);
}

void runLoadWorker(LoadWorker *load_worker) {
    //Set up relevant state -- localized to this thread's stack
    int32_t cpu_affinity = 0;
    bool use_sequential_kernel_fptr = false;
    SequentialFunction kernel_fptr_seq = NULL;
    SequentialFunction kernel_dummy_fptr_seq = NULL;
    RandomFunction kernel_fptr_ran = NULL;
    RandomFunction kernel_dummy_fptr_ran = NULL;
    void* start_address = NULL;
    void* end_address = NULL;
    void* prime_start_address = NULL;
    void* prime_end_address = NULL;
    uint32_t bytes_per_pass = 0;
    uint32_t passes = 0;
    tick_t start_tick = 0;
    tick_t stop_tick = 0;
    tick_t elapsed_ticks = 0;
    tick_t elapsed_dummy_ticks = 0;
    tick_t adjusted_ticks = 0;
    bool warning = false;
    void* mem_array = NULL;
    size_t len = 0;
    tick_t target_ticks = g_ticks_per_ms * BENCHMARK_DURATION_MS; //Rough target run duration in ticks
    uint32_t p = 0;
    bytes_per_pass = THROUGHPUT_BENCHMARK_BYTES_PER_PASS;

    //Grab relevant setup state thread-safely and keep it local
    if (acquireLock(load_worker->mem_worker->runnable, -1)) {
        mem_array = load_worker->mem_worker->mem_array_;
        len = load_worker->mem_worker->len_;
        cpu_affinity = load_worker->mem_worker->cpu_affinity_;
        use_sequential_kernel_fptr = load_worker->use_sequential_kernel_fptr_;
        kernel_fptr_seq = load_worker->kernel_fptr_seq_;
        kernel_dummy_fptr_seq = load_worker->kernel_dummy_fptr_seq_;
        kernel_fptr_ran = load_worker->kernel_fptr_ran_;
        kernel_dummy_fptr_ran = load_worker->kernel_dummy_fptr_ran_;
        start_address = mem_array;
        end_address = (void *) ((uint8_t *) mem_array + bytes_per_pass);
        prime_start_address = mem_array;
        prime_end_address = (void *) ((uint8_t *) mem_array + len);
        if (! releaseLock(load_worker->mem_worker->runnable)) {
            fprintf(stderr, "ERROR: in releaseLock()\n");
            exit(EXIT_FAILURE);
        }
    }  else {
        fprintf(stderr, "ERROR: in acquireLock()\n");
        exit(EXIT_FAILURE);
    }

    // Set processor affinity
    bool locked = lock_thread_to_cpu(cpu_affinity);
    if (! locked)
        fprintf(stderr, "WARNING: Failed to lock thread to logical CPU %d! Results may not be correct.\n", cpu_affinity);

    //Increase scheduling priority
    if (! boost_scheduling_priority())
        fprintf(stderr, "WARNING: Failed to boost scheduling priority. Perhaps running in Administrator mode would help.\n");

    //Prime memory
    for (uint32_t i = 0; i < 4; i++) {
        forwSequentialRead_Word32(prime_start_address, prime_end_address); //dependent reads on the memory, make sure caches are ready, coherence, etc...
    }

    //Run the benchmark!
    uintptr_t* next_address = (uintptr_t *) mem_array;
    //Run actual version of function and loop overhead
    while (elapsed_ticks < target_ticks) {
        if (use_sequential_kernel_fptr) { //sequential function semantics
            start_tick = start_timer();
            UNROLL1024(
                (*kernel_fptr_seq)(start_address, end_address);
                start_address = (void *) ((uint8_t *) mem_array + ((uintptr_t) start_address + bytes_per_pass) % len);
                end_address = (void *) ((uint8_t *) start_address  + bytes_per_pass);
            )
            stop_tick = stop_timer();
            passes+=1024;
        } else { //random function semantics
            start_tick = start_timer();
            UNROLL1024((*kernel_fptr_ran)(next_address, &next_address, bytes_per_pass);)
            stop_tick = stop_timer();
            passes+=1024;
        }
        elapsed_ticks += (stop_tick - start_tick);
    }

    //Run dummy version of function and loop overhead
    p = 0;
    start_address = mem_array;
    end_address = (void *) ((uint8_t *) mem_array + bytes_per_pass);
    next_address = (uintptr_t *) mem_array;
    while (p < passes) {
        if (use_sequential_kernel_fptr) { //sequential function semantics
            start_tick = start_timer();
            UNROLL1024(
                (*kernel_dummy_fptr_seq)(start_address, end_address);
                start_address = (void *) ((uint8_t *) mem_array + ((uintptr_t) start_address + bytes_per_pass) % len);
                end_address = (void *) ((uint8_t *) start_address  + bytes_per_pass);
            )
            stop_tick = stop_timer();
            p+=1024;
        } else { //random function semantics
            start_tick = start_timer();
            UNROLL1024((*kernel_dummy_fptr_ran)(next_address, &next_address, bytes_per_pass);)
            stop_tick = stop_timer();
            p+=1024;
        }

        elapsed_dummy_ticks += (stop_tick - start_tick);
    }

    // Unset processor affinity
    if (locked) {
        bool unlocked;
        unlocked = unlock_thread_to_cpu();
        if (! unlocked)
            fprintf(stderr, "WARNING: Failed to unlock threads!\n");
    }

    // Revert thread priority
    if (! revert_scheduling_priority())
        fprintf(stderr, "WARNING: Failed to revert scheduling priority. Perhaps running in Administrator mode would help.\n");

    adjusted_ticks = elapsed_ticks - elapsed_dummy_ticks;

    //Warn if something looks fishy
    if (elapsed_dummy_ticks >= elapsed_ticks || elapsed_ticks < MIN_ELAPSED_TICKS || adjusted_ticks < elapsed_ticks / 2)
        warning = true;

    //Update the object state thread-safely
    if (acquireLock(load_worker->mem_worker->runnable, -1)) {
        load_worker->mem_worker->adjusted_ticks_ = adjusted_ticks;
        load_worker->mem_worker->elapsed_ticks_ = elapsed_ticks;
        load_worker->mem_worker->elapsed_dummy_ticks_ = elapsed_dummy_ticks;
        load_worker->mem_worker->warning_ = warning;
        load_worker->mem_worker->bytes_per_pass_ = bytes_per_pass;
        load_worker->mem_worker->completed_ = true;
        load_worker->mem_worker->passes_ = passes;
        releaseLock(load_worker->mem_worker->runnable);
    }
}
