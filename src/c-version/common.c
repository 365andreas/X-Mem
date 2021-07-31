/**
 * @file
 *
 * @brief Implementation file for common preprocessor definitions, macros, functions, and global constants.
 */

//Headers
#include <Configurator.h>
#include <common.h>
#include <Timer.h>

//Libraries
#include <errno.h>
#include <stdio.h>
#include <string.h>


#ifdef __gnu_linux__
#include <unistd.h>
#include <pthread.h>
#ifdef HAS_NUMA
#include <numa.h>
#endif

#ifdef ARCH_INTEL
#include <immintrin.h> //for timer
#include <cpuid.h> //for timer
#include <x86intrin.h> //for timer
#endif

#ifdef USE_POSIX_TIMER
#include <time.h>
#endif

#ifdef HAS_LARGE_PAGES
#include <hugetlbfs.h> //for getting huge page size
#endif
#endif

#define MAX_PHYS_PACKAGE_IDS 10
#define MAX_CORE_IDS 200

bool g_verbose = false; /**< If true, be more verbose with console reporting. */
bool g_log_extended = false; /**< If true, disable early stopping (when CI does not deviate too much) and log measured values to enable statistical processing of the experiments. */
size_t g_page_size; /**< Default page size on the system, in bytes. */
size_t g_large_page_size; /**< Large page size on the system, in bytes. */
uint32_t g_num_numa_nodes; /**< Number of NUMA nodes in the system. */
uint32_t g_num_logical_cpus; /**< Number of logical CPU cores in the system. This may be different than physical CPUs, e.g. simultaneous multithreading. */
uint32_t g_num_physical_cpus; /**< Number of physical CPU cores in the system. */
uint32_t g_num_physical_packages; /**< Number of physical CPU packages in the system. Generally this is the same as number of NUMA nodes, unless UMA emulation is done in hardware. */
uint32_t *g_physical_package_of_cpu; /**< Mapping of logical CPU cores in the system to the according physical package that they belong. */
uint32_t g_starting_test_index; /**< Numeric identifier for the first benchmark test. */
uint32_t g_test_index; /**< Numeric identifier for the current benchmark test. */
tick_t g_ticks_per_ms; /**< Timer ticks per ms. */
float g_ns_per_tick; /**< Nanoseconds per timer tick. */

uint32_t num_core_ids = 0;

// void xmem::print_compile_time_options() {
//     std::cout << std::endl;
//     std::cout << "This binary was built for the following OS and architecture capabilities: " << std::endl;
// #ifdef _WIN32
//     std::cout << "Win32" << std::endl;
// #endif
// #ifdef _WIN64
//     std::cout << "Win64" << std::endl;
// #endif
// #ifdef __gnu_linux__
//     std::cout <<  "GNU/Linux" << std::endl;
// #endif
// #ifdef ARCH_INTEL
//     std::cout << "ARCH_INTEL" << std::endl;
// #endif
// #ifdef ARCH_INTEL_X86
//     std::cout << "ARCH_INTEL_X86" << std::endl;
// #endif
// #ifdef ARCH_INTEL_X86_64
//     std::cout << "ARCH_INTEL_X86_64" << std::endl;
// #endif
// #ifdef ARCH_INTEL_MIC
//     std::cout << "ARCH_INTEL_MIC" << std::endl;
// #endif
// #ifdef ARCH_INTEL_SSE
//     std::cout << "ARCH_INTEL_SSE" << std::endl;
// #endif
// #ifdef ARCH_INTEL_SSE2
//     std::cout << "ARCH_INTEL_SSE2" << std::endl;
// #endif
// #ifdef ARCH_INTEL_SSE3
//     std::cout << "ARCH_INTEL_SSE3" << std::endl;
// #endif
// #ifdef ARCH_INTEL_AVX
//     std::cout << "ARCH_INTEL_AVX" << std::endl;
// #endif
// #ifdef ARCH_INTEL_AVX2
//     std::cout << "ARCH_INTEL_AVX2" << std::endl;
// #endif
// #ifdef ARCH_INTEL_AVX512
//     std::cout << "ARCH_INTEL_AVX512" << std::endl;
// #endif
// #ifdef ARCH_AMD64
//     std::cout << "ARCH_AMD64" << std::endl;
// #endif
// #ifdef ARCH_ARM
//     std::cout << "ARCH_ARM" << std::endl;
// #endif
// #ifdef ARCH_ARM_64
//     std::cout << "ARCH_ARM_64" << std::endl;
// #endif
// #ifdef ARCH_ARM_V7
//     std::cout << "ARCH_ARM_V7" << std::endl;
// #endif
// #ifdef ARCH_ARM_V8
//     std::cout << "ARCH_ARM_V8" << std::endl;
// #endif
// #ifdef ARCH_ARM_VFP_V3
//     std::cout << "ARCH_ARM_VFP_V3" << std::endl;
// #endif
// #ifdef ARCH_ARM_VFP_V4
//     std::cout << "ARCH_ARM_VFP_V4" << std::endl;
// #endif
// #ifdef ARCH_ARM_NEON
//     std::cout << "ARCH_ARM_NEON" << std::endl;
// #endif
// #ifdef ARCH_64BIT
//     std::cout << "ARCH_64BIT" << std::endl;
// #endif
// #ifdef HAS_NUMA
//     std::cout << "HAS_NUMA" << std::endl;
// #endif
// #ifdef HAS_LARGE_PAGES
//     std::cout << "HAS_LARGE_PAGES" << std::endl;
// #endif
// #ifdef HAS_WORD_64
//     std::cout << "HAS_WORD_64" << std::endl;
// #endif
// #ifdef HAS_WORD_128
//     std::cout << "HAS_WORD_128" << std::endl;
// #endif
// #ifdef HAS_WORD_256
//     std::cout << "HAS_WORD_256" << std::endl;
// #endif
// #ifdef HAS_WORD_512
//     std::cout << "HAS_WORD_512" << std::endl;
// #endif
//     std::cout << std::endl;
//     std::cout << "This binary was built with the following compile-time options:" << std::endl;
// #ifdef NDEBUG
//     std::cout << "NDEBUG" << std::endl;
// #endif
// #ifdef USE_OS_TIMER
//     std::cout << "USE_OS_TIMER" << std::endl;
// #endif
// #ifdef USE_HW_TIMER
//     std::cout << "USE_HW_TIMER" << std::endl;
// #endif
// #ifdef USE_QPC_TIMER
//     std::cout << "USE_QPC_TIMER" << std::endl;
// #endif
// #ifdef USE_POSIX_TIMER
//     std::cout << "USE_POSIX_TIMER" << std::endl;
// #endif
// #ifdef USE_TSC_TIMER
//     std::cout << "USE_TSC_TIMER" << std::endl;
// #endif
//     //TODO: ARM timer
// #ifdef BENCHMARK_DURATION_SEC
//     std::cout << "BENCHMARK_DURATION_SEC = " << BENCHMARK_DURATION_SEC << std::endl; //This must be defined
// #endif
// #ifdef THROUGHPUT_BENCHMARK_BYTES_PER_PASS
//     std::cout << "THROUGHPUT_BENCHMARK_BYTES_PER_PASS == " << THROUGHPUT_BENCHMARK_BYTES_PER_PASS << std::endl;
// #endif
// #ifdef POWER_SAMPLING_PERIOD_SEC
//     std::cout << "POWER_SAMPLING_PERIOD_MS == " << POWER_SAMPLING_PERIOD_MS << std::endl;
// #endif
// #ifdef EXT_DELAY_INJECTED_LOADED_LATENCY_BENCHMARK
//     std::cout << "EXT_DELAY_INJECTED_LOADED_LATENCY_BENCHMARK" << std::endl;
// #endif
// #ifdef EXT_STREAM_BENCHMARK
//     std::cout << "EXT_STREAM_BENCHMARK" << std::endl;
// #endif
//     std::cout << std::endl;
// }

void setup_timer() {
    if (g_verbose)
        printf("\nInitializing timer...");

    Timer *timer = newTimer();

    g_ticks_per_ms = getTicksPerMs(timer);
    g_ns_per_tick = getNsPerTick(timer);

    if (g_verbose)
        printf("done\n");

    free(timer);
}

// void xmem::report_timer() {
//     std::cout << "Calculated timer frequency: " << g_ticks_per_ms * 1000 << " Hz == " << (double)(g_ticks_per_ms*1000) / (1e6) << " MHz" << std::endl;
//     std::cout << "Derived timer ns per tick: " << g_ns_per_tick << std::endl;
//     std::cout << std::endl;
// }

// void xmem::test_thread_affinities() {
//     std::cout << std::endl << "Testing thread affinities..." << std::endl;
//     bool success = false;
//     for (uint32_t cpu = 0; cpu < g_num_logical_cpus; cpu++) {
//         std::cout << "Locking to logical CPU " << cpu << "...";
//         success = lock_thread_to_cpu(cpu);
//         std::cout << (success ? "Pass" : "FAIL");
//         std::cout << "      Unlocking" << "...";
//         success = unlock_thread_to_cpu();
//         std::cout << (success ? "Pass" : "FAIL");
//         std::cout << std::endl;
//     }
// }

// bool xmem::lock_thread_to_numa_node(uint32_t numa_node) {
//     std::vector<uint32_t> cpus_in_node;
//     for (uint32_t c = 0; c < g_num_logical_cpus; c++) {
//         int32_t cpu = cpu_id_in_numa_node(numa_node, c);
//         if (cpu >= 0) //valid
//             cpus_in_node.push_back(static_cast<uint32_t>(cpu));
//     }

//     if (cpus_in_node.size() < 1) //Check to see that there was something to lock to
//         return false;

// #ifdef _WIN32
//     HANDLE tid = GetCurrentThread();
//     if (tid == 0)
//         return false;
//     else {
//         //Set thread affinity mask to include all CPUs in the NUMA node of interest
//         DWORD_PTR threadAffinityMask = 0;
//         for (auto it = cpus_in_node.cbegin(); it != cpus_in_node.cend(); it++)
//             threadAffinityMask |= static_cast<DWORD_PTR>((1 << *it));

//         DWORD_PTR prev_mask = SetThreadAffinityMask(tid, threadAffinityMask); //enable the CPUs
//         if (prev_mask == 0)
//             return false;
//     }
//     return true;
// #endif

// #ifdef __gnu_linux__
//     cpu_set_t cpus;
//     CPU_ZERO(&cpus);
//     for (auto it = cpus_in_node.cbegin(); it != cpus_in_node.cend(); it++)
//         CPU_SET(static_cast<int32_t>(*it), &cpus);

//     pthread_t tid = pthread_self();
//     return (!pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpus));
// #endif
// }

// bool xmem::unlock_thread_to_numa_node() {
//     return unlock_thread_to_cpu();
// }

// bool xmem::lock_thread_to_cpu(uint32_t cpu_id) {
// #ifdef _WIN32
//     HANDLE tid = GetCurrentThread();
//     if (tid == 0)
//         return false;
//     else {
//         DWORD_PTR threadAffinityMask = 1 << cpu_id;
//         DWORD_PTR prev_mask = SetThreadAffinityMask(tid, threadAffinityMask); //enable only 1 CPU
//         if (prev_mask == 0)
//             return false;
//     }
//     return true;
// #endif
// #ifdef __gnu_linux__
//     cpu_set_t cpus;
//     CPU_ZERO(&cpus);
//     CPU_SET(static_cast<int32_t>(cpu_id), &cpus);

//     pthread_t tid = pthread_self();
//     return (!pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpus));
// #endif
// }

// bool xmem::unlock_thread_to_cpu() {
// #ifdef _WIN32
//     HANDLE tid = GetCurrentThread();
//     if (tid == 0)
//         return false;
//     else {
//         DWORD_PTR threadAffinityMask = static_cast<DWORD_PTR>(-1);
//         DWORD_PTR prev_mask = SetThreadAffinityMask(tid, threadAffinityMask); //enable all CPUs
//         if (prev_mask == 0)
//             return false;
//     }
//     return true;
// #endif
// #ifdef __gnu_linux__
//     pthread_t tid = pthread_self();
//     cpu_set_t cpus;
//     CPU_ZERO(&cpus);

//     if (pthread_getaffinity_np(tid, sizeof(cpu_set_t), &cpus)) //failure
//         return false;

//     int32_t total_num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
//     for (int32_t c = 0; c < total_num_cpus; c++)
//         CPU_SET(c, &cpus);

//     return (!pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpus));
// #endif
// }

// int32_t xmem::cpu_id_in_numa_node(uint32_t numa_node, uint32_t cpu_in_node) {
// #ifndef HAS_NUMA
//     if (numa_node != 0) {
//         std::cerr << "WARNING: NUMA is not supported on this X-Mem build." << std::endl;
//         return -1;
//     }

//     return cpu_in_node;
// #endif

// #ifdef HAS_NUMA
//     int32_t cpu_id = -1;
//     uint32_t rank_in_node = 0;
// #ifdef _WIN32
//     ULONGLONG processorMask = 0;
//     GetNumaNodeProcessorMask(numa_node, &processorMask);
//     //Select Nth CPU in the node
//     uint32_t shifts = 0;
//     ULONGLONG shiftmask = processorMask;
//     bool done = false;
//     while (!done && shifts < sizeof(shiftmask)*8) {
//         if ((shiftmask & 0x1) == 0x1) { //current CPU is in the NUMA node
//             if (cpu_in_node == rank_in_node) { //found the CPU of interest in the NUMA node
//                 cpu_id = static_cast<int32_t>(shifts);
//                 done = true;
//             }
//             rank_in_node++;
//         }
//         shiftmask = shiftmask >> 1; //shift right by one to examine next CPU
//         shifts++;
//     }
// #endif
// #ifdef __gnu_linux__
//     struct bitmask *bm_ptr = numa_allocate_cpumask();
//     if (!bm_ptr) {
//         std::cerr << "WARNING: Failed to allocate a bitmask for loading NUMA information." << std::endl;
//         return -1;
//     }
//     if (numa_node_to_cpus(static_cast<int32_t>(numa_node), bm_ptr)) //error
//         return -1;

//     //Select Nth CPU in the node
//     for (uint32_t i = 0; i < bm_ptr->size; i++) {
//         if (numa_bitmask_isbitset(bm_ptr, i) == 1) {
//             if (cpu_in_node == rank_in_node) { //found the CPU of interest in the NUMA node
//                 cpu_id = i;
//                 if (bm_ptr)
//                     free(bm_ptr);
//                 return cpu_id;
//             }
//             rank_in_node++;
//         }
//     }

//     if (bm_ptr)
//         free(bm_ptr);
// #endif
//     return cpu_id;
// #endif
// }

void init_globals() {
    //Initialize global variables to defaults.
    g_verbose = false;
    g_log_extended = false;
    g_num_numa_nodes = DEFAULT_NUM_NODES;
    g_num_physical_packages = DEFAULT_NUM_PHYSICAL_PACKAGES;
    g_physical_package_of_cpu = DEFAULT_PHYSICAL_PACKAGE_OF_CPU;
    g_num_physical_cpus = DEFAULT_NUM_PHYSICAL_CPUS;
    g_num_logical_cpus = DEFAULT_NUM_LOGICAL_CPUS;
    g_page_size = DEFAULT_PAGE_SIZE;
    g_large_page_size = DEFAULT_LARGE_PAGE_SIZE;

    g_ticks_per_ms = 0;
    g_ns_per_tick = 0;
}

int32_t query_sys_info() {

    FILE *in;

    in = fopen("/proc/cpuinfo", "r");

    size_t line_size = 512;
    char *line = (char *) malloc(line_size * sizeof(double));
    char *line_str;
    uint32_t id = 0;


    //Get NUMA info
#ifdef HAS_NUMA
    if (numa_available() == -1) { //Check that NUMA is available.
        fprintf(stderr, "WARNING: NUMA API is not available on this system.\n");
        return -1;
    }

    //Get number of nodes. This is easy.
    g_num_numa_nodes = numa_max_node() + 1;
#endif
#ifndef HAS_NUMA //special case
    g_num_numa_nodes = 1;
#endif

    //Get number of physical packages. This is somewhat convoluted, but not sure of a better way on Linux. Technically there could be on-chip NUMA, so...
    uint32_t phys_package_ids[MAX_PHYS_PACKAGE_IDS];
    while (!feof(in)) {

        if (getline(&line, &line_size, in) < 0)
            if (errno != 0)
                fprintf(stderr, "WARNING: Failed reading line of /proc/cpuinfo.\n");

        if((line_str = strstr(line, "physical id")) != NULL) {
            sscanf(line_str, "physical id\t\t\t: %u", &id);

            bool found = false;
            for (int i = 0; (i < g_num_physical_packages) && (i < MAX_PHYS_PACKAGE_IDS); i++) {
                if (phys_package_ids[i] == id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                //have not seen this physical processor yet
                phys_package_ids[g_num_physical_packages] = id; //add to list
                g_num_physical_packages++;
                if (g_num_physical_packages > MAX_PHYS_PACKAGE_IDS) {
                    fprintf(stderr, "ERROR: Define a larger MAX_PHYS_PACKAGE_IDS since physical ids are more than %d.\n", MAX_PHYS_PACKAGE_IDS);
                    return -1;
                }
            }
        }
    }

    printf("phys_package_ids: ");
    for (int i = 0; i < g_num_physical_packages; i++) {
        printf("%d ", phys_package_ids[i]);
    }
    printf("\n");

    //Get number of CPUs
    //Get number of logical CPUs
    g_num_logical_cpus = sysconf(_SC_NPROCESSORS_ONLN); //This isn't really portable -- requires glibc extensions to sysconf()
    if (g_num_logical_cpus == -1) {
        perror("ERROR: sysconf(_SC_NPROCESSORS_ONLN) failed");
        return -1;
    }

    printf("g_num_logical_cpus: %d\n", g_num_logical_cpus);

    fseek(in, 0, SEEK_SET);
    //Get number of physical CPUs. This is somewhat convoluted, but not sure of a better way on Linux. I don't want to assume anything about HyperThreading-like things.
    uint32_t core_ids[MAX_CORE_IDS];
    while (!feof(in)) {

        if (getline(&line, &line_size, in) < 0)
            if (errno != 0)
                fprintf(stderr, "WARNING: Failed reading line of /proc/cpuinfo.\n");

        if((line_str = strstr(line, "core id")) != NULL) {
            sscanf(line_str, "core id\t\t\t: %u", &id);

            bool found = false;
            for (int i = 0; i < num_core_ids; i++) {
                if (core_ids[i] == id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                //have not seen this physical processor yet
                core_ids[num_core_ids] = id; //add to list
                num_core_ids++;
                if (num_core_ids > MAX_CORE_IDS) {
                    fprintf(stderr, "ERROR: Define a larger MAX_CORE_IDS since core ids are more than %d.\n", MAX_CORE_IDS);
                    return -1;
                }
            }
        }
    }
    g_num_physical_cpus = num_core_ids * g_num_physical_packages; //FIXME: currently this assumes each processor package has an equal number of cores. This may not be true in general! Need more complicated /proc/cpuinfo parsing.

    printf("core_ids: ");
    for (int i = 0; i < num_core_ids; i++) {
        printf("%d ", core_ids[i]);
    }
    printf("\n");

    //Get mapping of physical CPUs to packages
    g_physical_package_of_cpu = (uint32_t *) malloc(g_num_logical_cpus * sizeof(uint32_t));

    fseek(in, 0, SEEK_SET);
    uint32_t cpu = 0;
    uint32_t physical_id = 0;
    while (!feof(in)) {

        if (getline(&line, &line_size, in) < 0)
            if (errno != 0)
                fprintf(stderr, "WARNING: Failed reading line of /proc/cpuinfo.\n");

        if ((line_str = strstr(line, "processor")) != NULL) {
            sscanf(line_str, "processor\t\t\t: %u", &cpu);
        } else if ((line_str = strstr(line, "physical id")) != NULL) {
            sscanf(line_str, "physical id\t\t\t: %u", &physical_id);
            g_physical_package_of_cpu[cpu] = physical_id;
        }
    }

    printf("g_physical_package_of_cpus: \n");
    for (int i = 0; i < g_num_logical_cpus; i++) {
        printf("g_physical_package_of_cpu[%d] = %d\n", i, g_physical_package_of_cpu[i]);
    }
    printf("\n");

    //Get page size
    g_page_size = (size_t) (sysconf(_SC_PAGESIZE));
    if (sysconf(_SC_PAGESIZE) == -1) {
        perror("ERROR: sysconf(_SC_PAGESIZE) failed");
        return -1;
    }

#ifdef HAS_LARGE_PAGES
    g_large_page_size = gethugepagesize();
#endif

    printf("g_page_size: %ld\n", g_page_size);


    fclose(in);

    return 0;
}

// void xmem::report_sys_info() {
//     std::cout << std::endl;
//     std::cout << "Number of NUMA nodes: " << g_num_numa_nodes;
//     if (g_num_numa_nodes == DEFAULT_NUM_NODES)
//         std::cout << "?";
//     std::cout << std::endl;
//     std::cout << "Number of physical processor packages: " << g_num_physical_packages;
//     if (g_num_physical_packages == DEFAULT_NUM_PHYSICAL_PACKAGES)
//         std::cout << "?";
//     std::cout << std::endl;
//     if (g_physical_package_of_cpu.empty())
//         std::cout << "Mapping of physical core to packages is empty?";
//     std::cout << std::endl;
//     std::cout << "Number of physical processor cores: " << g_num_physical_cpus;
//     if (g_num_physical_cpus == DEFAULT_NUM_PHYSICAL_CPUS)
//         std::cout << "?";
//     std::cout << std::endl;
//     std::cout << "Number of logical processor cores: " << g_num_logical_cpus;
//     if (g_num_logical_cpus == DEFAULT_NUM_LOGICAL_CPUS)
//         std::cout << "?";
//     std::cout << std::endl;
//     std::cout << std::endl;
//     std::cout << "Regular page size: " << g_page_size << " B" << std::endl;
// #ifdef HAS_LARGE_PAGES
//     std::cout << "Large page size: " << g_large_page_size << " B" << std::endl;
// #endif
// }

tick_t start_timer() {
#ifdef USE_TSC_TIMER
    volatile int32_t dc0 = 0;
    volatile int32_t dc1, dc2, dc3, dc4;
    __cpuid(dc0, dc1, dc2, dc3, dc4); //Serializing instruction. This forces all previous instructions to finish
    return __rdtsc(); //Get clock tick

    /*
    uint32_t low, high;
    __asm__ __volatile__ (
        "cpuid\n\t"
        "rdtsc\n\t"
        "mov %%eax, %0\n\t"
        "mov %%edx, %1\n\n"
        : "=r" (low), "=r" (high)
        : : "%rax", "%rbx", "%rcx", "%rdx");

    return ((static_cast<uint64_t>(high) << 32) | low);
    */
#endif

#ifdef USE_POSIX_TIMER
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (tick_t) (tp.tv_sec * 1e9 + tp.tv_nsec); //Return time in nanoseconds
#endif
}

tick_t stop_timer() {
    //TODO: ARM hardware timer
#ifdef USE_TSC_TIMER
    tick_t tick;
    uint32_t filler;
    volatile int32_t dc0 = 0;
    volatile int32_t dc1, dc2, dc3, dc4;
    tick = __rdtscp(&filler); //Get clock tick. This is a partially serializing instruction. All previous instructions must finish
    __cpuid(dc0, dc1, dc2, dc3, dc4); //Serializing instruction. This forces all previous instructions to finish
    return tick;

    /*
    uint32_t low, high;
    __asm__ __volatile__ (
        "rdtscp\n\t"
        "mov %%eax, %0\n\t"
        "mov %%edx, %1\n\t"
        "cpuid\n\t"
        : "=r" (low), "=r" (high)
        : : "%rax", "%rbx", "%rcx", "%rdx");
    return ((static_cast<uint64_t>(high) << 32) | low);
    */
#endif

#ifdef USE_POSIX_TIMER
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (tick_t) (tp.tv_sec * 1e9 + tp.tv_nsec); //Return time in nanoseconds
#endif
}

// #ifdef __gnu_linux__
// bool xmem::boost_scheduling_priority() {
//     if (nice(-20) == EPERM)
//         return false;
//     return true;
// }
// #endif

// #ifdef __gnu_linux__
// bool xmem::revert_scheduling_priority() {
//     if (nice(0) == EPERM)
//         return false;
//     return true;
// }
// #endif
