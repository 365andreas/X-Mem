/**
 * @file
 *
 * @brief Implementation file for common preprocessor definitions, macros, functions, and global constants.
 */

#define _GNU_SOURCE

//Headers
#include <Configurator.h>
#include <Timer.h>
#include <common.h>
#include <sched.h>

//Libraries
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>


#include <unistd.h>
#include <pthread.h>

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

#define MAX_PHYS_PACKAGE_IDS 10
#define MAX_CORE_IDS 200

bool g_verbose = false; /**< If true, be more verbose with console reporting. */
bool g_log_extended = false; /**< If true, disable early stopping (when CI does not deviate too much) and log measured values to enable statistical processing of the experiments. */
size_t g_page_size; /**< Default page size on the system, in bytes. */
size_t g_large_page_size; /**< Large page size on the system, in bytes. */
uint32_t g_num_logical_cpus; /**< Number of logical CPU cores in the system. This may be different than physical CPUs, e.g. simultaneous multithreading. */
uint32_t g_num_physical_cpus; /**< Number of physical CPU cores in the system. */
uint32_t g_num_physical_packages; /**< Number of physical CPU packages in the system. Generally this is the same as number of NUMA nodes, unless UMA emulation is done in hardware. */
uint32_t *g_physical_package_of_cpu; /**< Mapping of logical CPU cores in the system to the according physical package that they belong. */
uint32_t g_starting_test_index; /**< Numeric identifier for the first benchmark test. */
uint32_t g_test_index; /**< Numeric identifier for the current benchmark test. */
tick_t g_ticks_per_ms; /**< Timer ticks per ms. */
float g_ns_per_tick; /**< Nanoseconds per timer tick. */

uint32_t num_core_ids = 0;

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

//     if (cpus_in_node.size() < 1) //Check to see that there was something to lock to
//         return false;

//     cpu_set_t cpus;
//     CPU_ZERO(&cpus);
//     for (auto it = cpus_in_node.cbegin(); it != cpus_in_node.cend(); it++)
//         CPU_SET(static_cast<int32_t>(*it), &cpus);

//     pthread_t tid = pthread_self();
//     return (!pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpus));
// }

bool lock_thread_to_cpu(uint32_t cpu_id) {
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET((int32_t) cpu_id, &cpus);

    pthread_t tid = pthread_self();
    return (!pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpus));
}

bool unlock_thread_to_cpu() {
    pthread_t tid = pthread_self();
    cpu_set_t cpus;
    CPU_ZERO(&cpus);

    if (pthread_getaffinity_np(tid, sizeof(cpu_set_t), &cpus)) //failure
        return false;

    int32_t total_num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    for (int32_t c = 0; c < total_num_cpus; c++)
        CPU_SET(c, &cpus);

    return (!pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpus));
}

void init_globals() {
    //Initialize global variables to defaults.
    g_verbose = false;
    g_log_extended = false;
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

    if (g_verbose) {
        printf("phys_package_ids: ");
        for (int i = 0; i < g_num_physical_packages; i++) {
            printf("%d ", phys_package_ids[i]);
        }
        printf("\n");
    }

    //Get number of CPUs
    //Get number of logical CPUs
    g_num_logical_cpus = sysconf(_SC_NPROCESSORS_ONLN); //This isn't really portable -- requires glibc extensions to sysconf()
    if (g_num_logical_cpus == -1) {
        perror("ERROR: sysconf(_SC_NPROCESSORS_ONLN) failed");
        return -1;
    }

    if (g_verbose)
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

    if (g_verbose) {
        printf("core_ids: ");
        for (int i = 0; i < num_core_ids; i++) {
            printf("%d ", core_ids[i]);
        }
        printf("\n");
    }

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

    if (g_verbose) {
        printf("g_physical_package_of_cpus: \n");
        for (int i = 0; i < g_num_logical_cpus; i++) {
            printf("g_physical_package_of_cpu[%d] = %d\n", i, g_physical_package_of_cpu[i]);
        }
        printf("\n");
    }

    //Get page size
    g_page_size = (size_t) (sysconf(_SC_PAGESIZE));
    if (sysconf(_SC_PAGESIZE) == -1) {
        perror("ERROR: sysconf(_SC_PAGESIZE) failed");
        return -1;
    }

#ifdef HAS_LARGE_PAGES
    g_large_page_size = gethugepagesize();
#endif

    if (g_verbose)
        printf("g_page_size: %ld\n", g_page_size);


    fclose(in);

    return 0;
}

// void xmem::report_sys_info() {
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

bool boost_scheduling_priority() {
    if (setpriority(PRIO_PROCESS, 0, -20) == -1) {
        perror("setting priority failed");
        return false;
    }
    return true;
}

bool revert_scheduling_priority() {
    if (setpriority(PRIO_PROCESS, 0, 0) == -1) {
        perror("setting priority failed");
        return false;
    }
    return true;
}
