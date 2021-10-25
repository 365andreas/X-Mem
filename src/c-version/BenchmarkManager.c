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

    return bench_mgr;
}

void printMatrix(BenchmarkManager *bench_mgr, MatrixBenchmark **mat_benchmarks, uint32_t mat_benchmarks_size, char *what) {

    Configurator *cfg = bench_mgr->config_;

    printf("Measured %s (in %s)...\n", what, getMetricUnits(mat_benchmarks[0]));

    int width = 3;
    printf("%*c", width, ' ');
    for (uint32_t region_id = 0; region_id < bench_mgr->num_mem_regions_; region_id++) {
        printf("      Region");
    }
    printf("\n");

    uint32_t regions_per_pu = cfg->mem_regions_phys_addr_size;

    printf("CPU");
    for (uint32_t region_id = 0; region_id < bench_mgr->num_mem_regions_; region_id++) {
        uint32_t mem_region = region_id;
        printf(" %*d", 11, mem_region);
    }

    for (uint32_t i = 0; i < mat_benchmarks_size; i++) {
        if (i % regions_per_pu == 0) {
            printf("\n");
            uint32_t pu = getCPUId(mat_benchmarks[i]);
            printf("%*d", width, pu);
        }

        double median_metric = getMedianMetric(mat_benchmarks[i]);
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

    Configurator *cfg = bench_mgr->config_;

    uint64_t *mem_regions_phys_addr = getMemoryRegionsPhysAddresses(cfg);

    uint32_t mem_regions_per_cpu = numberOfMemoryRegionsPhysAddresses(cfg);

    bench_mgr->mem_arrays_      = (void **)  malloc(mem_regions_per_cpu * sizeof(void *));
    bench_mgr->mem_array_lens_  = (size_t *) malloc(mem_regions_per_cpu * sizeof(size_t));
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

            if (latencyMatrixTestSelected(cfg) || throughputMatrixTestSelected(cfg)) {
                printf("Physical address of memory region #%d: 0x%.16llx\n", region_id, (long long unsigned int) phys_addr);
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
    uint32_t cpu = -1;

    if (runForAllCoresSelected(cfg)) {
        num_processor_units = g_num_logical_cpus;
        processor_units = (uint32_t *) malloc(num_processor_units * sizeof(uint32_t));
        for (uint32_t i = 0; i < g_num_logical_cpus; i++) {
            processor_units[i] = i;
        }
    } else {
        num_processor_units = 1;
        processor_units = (uint32_t *) malloc(num_processor_units * sizeof(uint32_t));
        processor_units[0] = getCoreId(cfg);
    }

    uint64_t *mem_regions_phys_addr = getMemoryRegionsPhysAddresses(cfg);

    bench_mgr->lat_mat_benchmarks_size_ = num_processor_units * numberOfMemoryRegionsPhysAddresses(cfg);
    bench_mgr->lat_mat_benchmarks_ = (LatencyMatrixBenchmark **) malloc(bench_mgr->lat_mat_benchmarks_size_ * sizeof(LatencyMatrixBenchmark *));
    int l = 0;
    //Build latency matrix benchmarks
    for (int pu = 0; pu < num_processor_units; pu++) {
        cpu = processor_units[pu];

        for (uint32_t region_id = 0; region_id < numberOfMemoryRegionsPhysAddresses(cfg); region_id++) {
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
                                                                           cfg->assume_existing_pointers_,
                                                                           DEFAULT_NUM_WORKER_THREADS,
                                                                           mem_region,
                                                                           cpu,
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
    chunk_size_t chunk = CHUNK_64b;
    int32_t stride = 1;
    rw_mode_t rw_mode;
    if (useWrites(cfg)) {
        rw_mode = WRITE;
    } else {
        rw_mode = READ;
    }

    //Build throughput matrix benchmarks
    for (int pu = 0; pu != num_processor_units; pu++) {
        cpu = processor_units[pu];

        for (uint32_t region_id = 0; region_id < numberOfMemoryRegionsPhysAddresses(cfg); region_id++) {
            void* mem_array = bench_mgr->mem_arrays_[region_id];
            size_t mem_array_len = bench_mgr->mem_array_lens_[region_id];
            uint32_t mem_region = region_id;


            // Throughput benchmark from every core to every memory region
            sprintf(benchmark_name, "Test #%d TM (ThroughputMatrix)", g_test_index);

            bench_mgr->thr_mat_benchmarks_[l] = initThroughputMatrixBenchmark(mem_array,
                                                                              mem_array_len,
                                                                              getIterationsPerTest(cfg),
                                                                              useNumCores(cfg),
                                                                              mem_region,
                                                                              cpu,
                                                                              SEQUENTIAL,
                                                                              rw_mode,
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
