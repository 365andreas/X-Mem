/**
 * @file
 *
 * @brief Implementation file for the BenchmarkManager struct.
 */

//Headers
#include <BenchmarkManager.h>
#include <Configurator.h>
#include <common.h>

//Libraries
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef HAS_NUMA
#include <numa.h>
#include <numaif.h>
#endif
#ifdef HAS_LARGE_PAGES
#include <hugetlbfs.h> //for allocating and freeing huge pages
#endif

void writeToDecNetFile(BenchmarkManager *bench_mgr, MatrixBenchmark *mat_bench, uint64_t *mem_regions_phys_addr,
                       char *bench_type) {
    uint32_t region_id = getMemRegion(mat_bench);
    fprintf(bench_mgr->dec_net_results_file_,
            "bench_result(%d, %ld, %s, %f, '%s').\n",
            getCPUId(mat_bench),
            mem_regions_phys_addr[region_id],
            bench_type,
            getMedianMetric(mat_bench),
            getMetricUnits(mat_bench)
    );
}

BenchmarkManager *initBenchMgr(Configurator *config) {

    BenchmarkManager *bench_mgr = (BenchmarkManager *) malloc(sizeof(BenchmarkManager));

    bench_mgr->config_ = config;
    bench_mgr->built_benchmarks_ = false;

    //Set up NUMA stuff
    // cpu_numa_node_affinities_ = config_.getCpuNumaNodeAffinities();
    bench_mgr->num_cpu_numa_node_affinities_ = 1;
    bench_mgr->cpu_numa_node_affinities_ = (uint32_t *) malloc(bench_mgr->num_cpu_numa_node_affinities_ * sizeof(uint32_t));
    for (int i = 0; i < bench_mgr->num_cpu_numa_node_affinities_; i++) {
        bench_mgr->cpu_numa_node_affinities_[i] = i;
    }

    // memory_numa_node_affinities_ = config_.getMemoryNumaNodeAffinities();

    //Build working memory regions
    setupWorkingSets(bench_mgr, getWorkingSetSizePerThread(bench_mgr->config_));

    //Open decoding networks compatible resuslts file
    if (useDecNetFile(bench_mgr->config_)) {
        bench_mgr->dec_net_results_file_ = fopen(getDecNetFilename(bench_mgr->config_), "w");
        if (bench_mgr->dec_net_results_file_ == NULL) {
            setUseDecNetFile(bench_mgr->config_, false);
            fprintf(stderr, "WARNING: Failed to open %s for writing! No results file for decoding"
            "networks will be generated.\n", getDecNetFilename(bench_mgr->config_));
        }
    }

    // // If extended measurements are enabled for latency matrix benchmark open logfile
    // if (g_log_extended && config_.latencyMatrixTestSelected()) {
	//     lat_mat_logfile_.open("latency_mat.logs");
    //     if (! lat_mat_logfile_) {
    //         std::cerr << "WARNING: Logfile latency_mat.logs was not created." << std::endl;
    //     }

    //     lat_mat_logfile_ << (config_.runForAllCoresSelected() ? "cpu" : "cpu_node") << ","
    //                      << "numa_node"                                       << ","
    //                      << "region"                                          << ","
    //                      << "iteration"                                       << ","
    //                      << "metric"                                          << ","
    //                      << "units"                                           << std::endl;
    // }

    // // If extended measurements are enabled for throughput matrix benchmark open logfile
    // if (g_log_extended && config_.throughputMatrixTestSelected()) {
	//     thr_mat_logfile_.open("throughput_mat.logs");
    //     if (! thr_mat_logfile_) {
    //         std::cerr << "WARNING: Logfile throughput_mat.logs was not created." << std::endl;
    //     }

    //     thr_mat_logfile_ << (config_.runForAllCoresSelected() ? "cpu" : "cpu_node") << ","
    //                      << "numa_node"                                       << ","
    //                      << "region"                                          << ","
    //                      << "iteration"                                       << ","
//     //                      << "metric"                                          << ","
//     //                      << "units"                                           << std::endl;
//     // }

    return bench_mgr;
}

// BenchmarkManager::~BenchmarkManager() {
//     //Free throughput benchmarks
//     for (uint32_t i = 0; i < tp_benchmarks_.size(); i++)
//         delete tp_benchmarks_[i];
//     //Free latency benchmarks
//     for (uint32_t i = 0; i < lat_benchmarks_.size(); i++)
//         delete lat_benchmarks_[i];
//     //Free latency matrix benchmarks
//     for (uint32_t i = 0; i < lat_mat_benchmarks_.size(); i++)
//         delete lat_mat_benchmarks_[i];
//     //Free throughput matrix benchmarks
//     for (uint32_t i = 0; i < thr_mat_benchmarks_.size(); i++)
//         delete thr_mat_benchmarks_[i];
//     //Free memory arrays
//     for (uint32_t i = 0; i < mem_arrays_.size(); i++)
//         if (mem_arrays_[i] != nullptr) {
// #ifdef _WIN32
//             VirtualFreeEx(GetCurrentProcess(), mem_arrays_[i], 0, MEM_RELEASE);
// #endif
// #ifdef __gnu_linux__
// #ifdef HAS_LARGE_PAGES
//             if (config_.useLargePages()) {
//                 free_huge_pages(mem_arrays_[i]);
//             } else {
// #endif
//                 if (! config_.memoryRegionsInPhysAddr()) {
// #ifdef HAS_NUMA
//                     numa_free(mem_arrays_[i], mem_array_lens_[i]);
// #else
//                     free(orig_malloc_addr_); //this is somewhat of a band-aid
// #endif
//                 } else {
//                     if (munmap(mem_arrays_[i], mem_array_lens_[i]) < 0) {
//                         perror("Failed to munmap() memory regions:");
//                     }
//                 }
// #ifdef HAS_LARGE_PAGES
//             }
// #endif
// #endif
//         }
//     //Close results file
//     if (results_file_.is_open())
//         results_file_.close();

//     //Close latency matrix extended measurements logfile
//     if (lat_mat_logfile_.is_open())
//         lat_mat_logfile_.close();

//     //Close throughput matrix extended measurements logfile
//     if (thr_mat_logfile_.is_open())
//         thr_mat_logfile_.close();
// }

void printMatrix(BenchmarkManager *bench_mgr, MatrixBenchmark **mat_benchmarks, uint32_t mat_benchmarks_size, char *what) {

    Configurator *cfg = bench_mgr->config_;

    // uint32_t mem_regions_per_numa = config_.getMemoryRegionsPerNUMANode();

    printf("Measured %s (in %s)...\n", what, getMetricUnits(mat_benchmarks[0]));
    printf("(Node, Reg) = (Memory NUMA Node, Region)\n\n");

    // std::vector<uint64_t> mem_regions_phys_addr = config_.getMemoryRegionsPhysAddresses();

    int width = runForAllCoresSelected(cfg) ? 3 : 13;
    printf("%*c", width, ' ');
    for (uint32_t region_id = 0; region_id < bench_mgr->num_mem_regions_; region_id++) {
        printf(" (Node, Reg)");
    }
    printf("\n");

    uint32_t regions_per_pu = cfg->mem_regions_phys_addr_size;
    // uint32_t regions_per_pu = config_.memoryRegionsInPhysAddr() ? mem_regions_phys_addr.size()
    //                                                             : mem_regions_per_numa * g_num_numa_nodes;

    if (runForAllCoresSelected(cfg))
        printf("CPU");
    else
        printf("CPU NUMA Node");
    for (uint32_t region_id = 0; region_id < bench_mgr->num_mem_regions_; region_id++) {
        uint32_t mem_node = bench_mgr->mem_array_node_[region_id];
        uint32_t mem_region = region_id;
        // if (! config_.memoryRegionsInPhysAddr()) {
        //     mem_region = region_id % config_.getMemoryRegionsPerNUMANode();
        // }

        // std::string node_str = (mem_node == static_cast<uint32_t>(-1)) ? "?" : std::to_string(mem_node);
        // std::cout  << std::setw(12) << "(" + node_str + ", " + std::to_string(mem_region) + ")";
        printf(" (   ?, %*d)", 3, mem_region);
    }

    for (uint32_t i = 0; i < mat_benchmarks_size; i++) {
        if (i % regions_per_pu == 0) {
            printf("\n");
            uint32_t pu;
            if (runForAllCoresSelected(cfg))
                pu = getCPUId(mat_benchmarks[i]);
            else
                pu = 0; //mat_benchmarks_[i]->getCPUNode();
            printf("%*d", width, pu);
        }

        double median_metric = getMedianMetric(mat_benchmarks[i]);
        // std::string metric_units = lat_mat_benchmarks_[i]->getMetricUnits();
        printf("%*.4f", 12, median_metric);
    }
    printf("\n");
}

bool runLatencyMatrixBenchmarks(BenchmarkManager *bench_mgr) {
    if (! bench_mgr->built_benchmarks_) {
        if (! buildBenchmarks(bench_mgr)) {
            fprintf(stderr, "ERROR: Failed to build benchmarks.\n");
            return false;
        }
    }

    uint64_t *mem_regions_phys_addr = NULL;
    if (useDecNetFile(bench_mgr->config_)) {
        mem_regions_phys_addr = getMemoryRegionsPhysAddresses(bench_mgr->config_);
    }

    for (uint32_t i = 0; i < bench_mgr->lat_mat_benchmarks_size_; i++) {
        MatrixBenchmark *curr_mat_bench = bench_mgr->lat_mat_benchmarks_[i]->mat_bench;

        runLatency(bench_mgr->lat_mat_benchmarks_[i]);
        reportResults(curr_mat_bench);
        if (useDecNetFile(bench_mgr->config_) && (bench_mgr->config_->mem_regions_phys_addr_size > 0)) {
            writeToDecNetFile(bench_mgr, curr_mat_bench, mem_regions_phys_addr, "'latency'");
        }
    }
    printf("\n");

    // aggregated report of latency matrix benchmarks
    uint32_t size = bench_mgr->lat_mat_benchmarks_size_;
    MatrixBenchmark **mat_benchmarks = (MatrixBenchmark **) malloc(size * sizeof(MatrixBenchmark *));
    for (uint32_t i = 0; i < size; i++)
        mat_benchmarks[i] = bench_mgr->lat_mat_benchmarks_[i]->mat_bench;
    printMatrix(bench_mgr, mat_benchmarks, size, "idle latencies");

    if (g_verbose)
        printf("\nDone running latency matrix benchmarks.\n");

    return true;
}

bool runThroughputMatrixBenchmarks(BenchmarkManager *bench_mgr) {
    if (! bench_mgr->built_benchmarks_) {
        if (! buildBenchmarks(bench_mgr)) {
            fprintf(stderr, "ERROR: Failed to build benchmarks.\n");
            return false;
        }
    }

    uint64_t *mem_regions_phys_addr = NULL;
    if (useDecNetFile(bench_mgr->config_)) {
        mem_regions_phys_addr = getMemoryRegionsPhysAddresses(bench_mgr->config_);
    }

    for (uint32_t i = 0; i < bench_mgr->thr_mat_benchmarks_size_; i++) {
        MatrixBenchmark *curr_mat_bench = bench_mgr->thr_mat_benchmarks_[i]->mat_bench;

        runThroughput(bench_mgr->thr_mat_benchmarks_[i]);
        reportResults(curr_mat_bench);
        if (useDecNetFile(bench_mgr->config_) && (bench_mgr->config_->mem_regions_phys_addr_size > 0)) {
            writeToDecNetFile(bench_mgr, curr_mat_bench, mem_regions_phys_addr, "'throughput'");
        }
    }
    printf("\n");

    // aggregated report of throughput matrix benchmarks
    uint32_t size = bench_mgr->thr_mat_benchmarks_size_;
    MatrixBenchmark **mat_benchmarks = (MatrixBenchmark **) malloc(size * sizeof(MatrixBenchmark *));
    for (uint32_t i = 0; i < size; i++)
        mat_benchmarks[i] = bench_mgr->thr_mat_benchmarks_[i]->mat_bench;
    printMatrix(bench_mgr, mat_benchmarks, size, "unloaded throughputs");

    if (g_verbose)
        printf("\nDone running throughput matrix benchmarks.\n");

    return true;
}

void setupWorkingSets(BenchmarkManager *bench_mgr, size_t working_set_size) {
    //Allocate memory in each NUMA node to be tested

    Configurator *cfg = bench_mgr->config_;

    uint64_t *mem_regions_phys_addr = getMemoryRegionsPhysAddresses(cfg);

    uint32_t mem_regions_per_cpu = numberOfMemoryRegionsPhysAddresses(cfg);

    bench_mgr->mem_arrays_      = (void **)  malloc(mem_regions_per_cpu * sizeof(void *));
    bench_mgr->mem_array_lens_  = (size_t *) malloc(mem_regions_per_cpu * sizeof(size_t));
    bench_mgr->mem_array_node_  = (size_t *) malloc(mem_regions_per_cpu * sizeof(size_t));
    bench_mgr->num_mem_regions_ = mem_regions_per_cpu;

    if (! memoryRegionsInPhysAddr(cfg)) {
        fprintf(stderr, "ERROR: memory regions to be benchmarked were not specified\n");
        exit(-1);
    } else {
        int flags = O_RDWR;
        if (syncMemory(cfg)) flags |= O_SYNC;
        int fd = open("/dev/mem", flags);
        if (fd < 0) {
            perror("ERROR! Failed to open /dev/mem");
            exit(-1);
        }

        for (uint32_t region_id = 0; region_id < mem_regions_per_cpu; region_id++) {

            size_t allocation_size = working_set_size;
            size_t phys_addr = mem_regions_phys_addr[region_id];

            // Truncate offset to a multiple of the page size, or mmap will fail.
            size_t page_size = g_page_size;
            size_t page_base = (phys_addr / page_size) * page_size;
            size_t page_offset = phys_addr - page_base;
            size_t len = page_offset + allocation_size;


            void *virt_addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
            if (virt_addr == MAP_FAILED) {
                perror("ERROR! Failed to mmap /dev/mem");
                exit(-1);
            }

            bench_mgr->mem_arrays_[region_id] = virt_addr;
            bench_mgr->mem_array_lens_[region_id] = allocation_size;
            bench_mgr->mem_array_node_[region_id] = -1;

            if (latencyMatrixTestSelected(cfg) || throughputMatrixTestSelected(cfg)) {
                printf("Physical address of memory region #%d: 0x%.16llx\n", region_id, (long long unsigned int) phys_addr);

#ifdef HAS_NUMA
                // check on which NUMA node this region belongs to
                size_t num_pages = len / page_size;
                num_pages += ((len % page_size) > 0) ? 1 : 0;
                int *status = (int *) malloc(num_pages * sizeof(int));
                void **pages = (void **) malloc(num_pages * sizeof(void *));
                memset(virt_addr, 0x41, len);
                for(uint32_t i = 0; i < num_pages; i++) {
                    pages[i] = &((char *) virt_addr)[i * page_size];
                }
                if (0 != move_pages(0 /*self memory */, 1, pages, NULL, status, MPOL_MF_MOVE_ALL)) {
                    perror("WARNING! In move_page() willing to learn on which NUMA node belongs a physical address");
                }
                for (uint32_t i = 0; i < num_pages; i++) {
                    if (status[i] < 0) {
                        errno = -status[i];
                        perror("WARNING! In move_page() status returned error");
                        fprintf(stderr, "\n^^^ page #%d\n", i + 1);
                        bench_mgr->mem_array_node_[region_id] = (uint32_t) -1;
                    } else if (i == 0) {
                        printf(" on NUMA node %d (first page)", status[0]);
                        bench_mgr->mem_array_node_[region_id] = status[0];
                    } else if (i == num_pages - 1) {
                        printf(" and on NUMA node %d (last page, %ld)", status[num_pages - 1], num_pages);
                    }
                }
                printf("\n");
                free(pages);
                free(status);
#endif
            }
        }
    }
    printf("\n");
}

bool buildBenchmarks(BenchmarkManager *bench_mgr) {
    if (g_verbose)  {
        printf("\nGenerating benchmarks.\n");
    }

    if (g_verbose)
        printf("\n");

    char benchmark_name[50];

    Configurator *cfg = bench_mgr->config_;
    uint32_t num_processor_units; // Number of CPU nodes or CPUs to affinitize for benchmark experiments.
    uint32_t *processor_units = NULL; // List of CPU nodes or CPUs to affinitize for benchmark experiments.
    bool use_cpu_nodes = false;
    uint32_t cpu_node = -1;

    if (runForAllCoresSelected(cfg)) {
        num_processor_units = g_num_logical_cpus;
        processor_units = (uint32_t *) malloc(g_num_logical_cpus * sizeof(uint32_t));
        for (uint32_t i = 0; i < g_num_logical_cpus; i++) {
            processor_units[i] = i;
        }
    } else {
        num_processor_units = 1; //bench_mgr->num_cpu_numa_node_affinities_;
        // processor_units = bench_mgr->cpu_numa_node_affinities_;
        use_cpu_nodes = true;
        cpu_node = getCoreId(cfg);
    }

    uint64_t *mem_regions_phys_addr = getMemoryRegionsPhysAddresses(cfg);

    uint32_t cpu      = -1;

    bench_mgr->lat_mat_benchmarks_size_ = num_processor_units * numberOfMemoryRegionsPhysAddresses(cfg);
    bench_mgr->lat_mat_benchmarks_ = (LatencyMatrixBenchmark **) malloc(bench_mgr->lat_mat_benchmarks_size_ * sizeof(LatencyMatrixBenchmark *));
    int l = 0;
    //Build latency matrix benchmarks
    for (int pu = 0; pu < num_processor_units; pu++) { //iterate each cpu NUMA node
        if (runForAllCoresSelected(cfg)) {
            cpu = processor_units[pu];
        }

        for (uint32_t region_id = 0; region_id < numberOfMemoryRegionsPhysAddresses(cfg); region_id++) {
            uint32_t mem_node = bench_mgr->mem_array_node_[region_id];
            void* mem_array = bench_mgr->mem_arrays_[region_id];
            size_t mem_array_len = bench_mgr->mem_array_lens_[region_id];
            uint32_t mem_region = region_id;

            chunk_size_t chunk = CHUNK_64b;
            int32_t stride = 1;

            // Latency benchmark from every core to every memory region
            sprintf(benchmark_name, "Test #%d LM (LatencyMatrix)", g_test_index);

            bench_mgr->lat_mat_benchmarks_[l] = initLatencyMatrixBenchmark(mem_array,
                                                                           mem_array_len,
                                                                           getIterationsPerTest(cfg),
                                                                           DEFAULT_NUM_WORKER_THREADS,
                                                                           mem_node,
                                                                           mem_region,
                                                                           cpu_node,
                                                                           cpu,
                                                                           use_cpu_nodes,
                                                                           RANDOM,
                                                                           READ,
                                                                           chunk,
                                                                           stride,
                                                                           benchmark_name);
            if (bench_mgr->lat_mat_benchmarks_[l] == NULL) {
                fprintf(stderr, "ERROR: Failed to build a LatencyMatrixBenchmark!\n");
                return false;
            }
            l++;
            g_test_index++;
        }
    }

    uint32_t num_of_cpus_in_cpu_node = 0;
    bench_mgr->thr_mat_benchmarks_size_ = num_processor_units * numberOfMemoryRegionsPhysAddresses(cfg);
    bench_mgr->thr_mat_benchmarks_ = (ThroughputMatrixBenchmark **) malloc(bench_mgr->thr_mat_benchmarks_size_ * sizeof(ThroughputMatrixBenchmark *));
    l = 0;

    //Build throughput matrix benchmarks
    for (int pu = 0; pu != num_processor_units; pu++) { //iterate each cpu
        if (! use_cpu_nodes) {
            cpu = processor_units[pu];
        }

        for (uint32_t region_id = 0; region_id < numberOfMemoryRegionsPhysAddresses(cfg); region_id++) {
            uint32_t mem_node = bench_mgr->mem_array_node_[region_id];
            void* mem_array = bench_mgr->mem_arrays_[region_id];
            size_t mem_array_len = bench_mgr->mem_array_lens_[region_id];
            uint32_t mem_region = region_id;

            chunk_size_t chunk = CHUNK_64b;
            int32_t stride = 1;

            // Throughput benchmark from every core to every memory region
            sprintf(benchmark_name, "Test #%d TM (ThroughputMatrix)", g_test_index);

            bench_mgr->thr_mat_benchmarks_[l] = initThroughputMatrixBenchmark(mem_array,
                                                                              mem_array_len,
                                                                              getIterationsPerTest(cfg),
                                                                              useNumCores(cfg),
                                                                              mem_node,
                                                                              mem_region,
                                                                              cpu_node,
                                                                              cpu,
                                                                              use_cpu_nodes,
                                                                              SEQUENTIAL,
                                                                              READ,
                                                                              chunk,
                                                                              stride,
                                                                              benchmark_name);

            if (bench_mgr->thr_mat_benchmarks_[l] == NULL) {
                fprintf(stderr, "ERROR: Failed to build a ThroughputMatrixBenchmark!\n");
                return false;
            }
            l++;
            g_test_index++;
        }
    }

    bench_mgr->built_benchmarks_ = true;

    return true;
}
