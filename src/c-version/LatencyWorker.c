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

    lat_worker->mem_worker = (MemoryWorker *) malloc(sizeof(MemoryWorker));

    lat_worker->mem_worker->len_          = len;
    lat_worker->mem_worker->mem_array_    = mem_array;
    lat_worker->mem_worker->cpu_affinity_ = cpu_affinity;

    lat_worker->kernel_fptr       = kernel_fptr;
    lat_worker->kernel_dummy_fptr = kernel_dummy_fptr;

    return lat_worker;
}

// LatencyWorker::~LatencyWorker() {
// }

// void LatencyWorker::run() {
//     //Set up relevant state -- localized to this thread's stack
//     int32_t cpu_affinity = 0;
//     RandomFunction kernel_fptr = NULL;
//     RandomFunction kernel_dummy_fptr = NULL;
//     uintptr_t* next_address = NULL;
//     uint32_t bytes_per_pass = 0;
//     uint32_t passes = 0;
//     uint32_t p = 0;
//     tick_t start_tick = 0;
//     tick_t stop_tick = 0;
//     tick_t elapsed_ticks = 0;
//     tick_t elapsed_dummy_ticks = 0;
//     tick_t adjusted_ticks = 0;
//     bool warning = false;
//     void* mem_array = NULL;
//     size_t len = 0;
//     tick_t target_ticks = g_ticks_per_ms * BENCHMARK_DURATION_MS; //Rough target run duration in ticks

//     //Grab relevant setup state thread-safely and keep it local
//     if (acquireLock(-1)) {
//         mem_array = mem_array_;
//         len = len_;
//         bytes_per_pass = LATENCY_BENCHMARK_UNROLL_LENGTH * 8;
//         cpu_affinity = cpu_affinity_;
//         kernel_fptr = kernel_fptr_;
//         kernel_dummy_fptr = kernel_dummy_fptr_;
//         releaseLock();
//     }

//     //Set processor affinity
//     bool locked = lock_thread_to_cpu(cpu_affinity);
//     if (!locked)
//         std::cerr << "WARNING: Failed to lock thread to logical CPU " << cpu_affinity << "! Results may not be correct." << std::endl;

//     //Increase scheduling priority
// #ifdef _WIN32
//     DWORD original_priority_class;
//     DWORD original_priority;
//     if (!boost_scheduling_priority(original_priority_class, original_priority))
// #endif
// #ifdef __gnu_linux__
//     if (!boost_scheduling_priority())
// #endif
//         std::cerr << "WARNING: Failed to boost scheduling priority. Perhaps running in Administrator mode would help." << std::endl;

//     //Prime memory
//     for (uint32_t i = 0; i < 4; i++) {
//         void* prime_start_address = mem_array;
//         void* prime_end_address = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(mem_array) + len);
//         forwSequentialRead_Word32(prime_start_address, prime_end_address); //dependent reads on the memory, make sure caches are ready, coherence, etc...
//     }

//     //Run benchmark
//     //Run actual version of function and loop overhead
//     next_address = static_cast<uintptr_t*>(mem_array);
//     while (elapsed_ticks < target_ticks) {
//         start_tick = start_timer();
//         UNROLL256((*kernel_fptr)(next_address, &next_address, 0);)
//         stop_tick = stop_timer();
//         elapsed_ticks += (stop_tick - start_tick);
//         passes+=256;
//     }

//     //Run dummy version of function and loop overhead
//     next_address = static_cast<uintptr_t*>(mem_array);
//     while (p < passes) {
//         start_tick = start_timer();
//         UNROLL256((*kernel_dummy_fptr)(next_address, &next_address, 0);)
//         stop_tick = stop_timer();
//         elapsed_dummy_ticks += (stop_tick - start_tick);
//         p+=256;
//     }

//     adjusted_ticks = elapsed_ticks - elapsed_dummy_ticks;

//     //Warn if something looks fishy
//     if (elapsed_dummy_ticks >= elapsed_ticks || elapsed_ticks < MIN_ELAPSED_TICKS || adjusted_ticks < 0.5 * elapsed_ticks)
//         warning = true;

//     //Unset processor affinity
//     if (locked)
//         unlock_thread_to_numa_node();

//     //Revert thread priority
// #ifdef _WIN32
//     if (!revert_scheduling_priority(original_priority_class, original_priority))
// #endif
// #ifdef __gnu_linux__
//     if (!revert_scheduling_priority())
// #endif
//         std::cerr << "WARNING: Failed to revert scheduling priority. Perhaps running in Administrator mode would help." << std::endl;

//     //Update the object state thread-safely
//     if (acquireLock(-1)) {
//         adjusted_ticks_ = adjusted_ticks;
//         elapsed_ticks_ = elapsed_ticks;
//         elapsed_dummy_ticks_ = elapsed_dummy_ticks;
//         warning_ = warning;
//         bytes_per_pass_ = bytes_per_pass;
//         completed_ = true;
//         passes_ = passes;
//         releaseLock();
//     }
// }
