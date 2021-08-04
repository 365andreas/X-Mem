/**
 * @file
 *
 * @brief Implementation file for the BenchmarkManager struct.
 */

//Headers
#include <BenchmarkManager.h>
#include <common.h>
#include <Configurator.h>

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

    // //Open decoding networks compatible resuslts file
    // if (config_.useDecNetFile()) {
    //     dec_net_results_file_.open(config_.getDecNetFilename().c_str(), std::fstream::out);
    //     if (!dec_net_results_file_.is_open()) {
    //         config_.setUseDecNetFile(false);
    //         std::cerr << "WARNING: Failed to open " << config_.getDecNetFilename()
    //         << " for writing! No results file for decoding networks will be generated." << std::endl;
    //     }
    // }

    // // If extended measurements are enabled for latency matrix benchmark open logfile
    // if (g_log_extended && config_.latencyMatrixTestSelected()) {
	//     lat_mat_logfile_.open("latency_mat.logs");
    //     if (! lat_mat_logfile_) {
    //         std::cerr << "WARNING: Logfile latency_mat.logs was not created." << std::endl;
    //     }

    //     lat_mat_logfile_ << (config_.allCoresSelected() ? "cpu" : "cpu_node") << ","
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

    //     thr_mat_logfile_ << (config_.allCoresSelected() ? "cpu" : "cpu_node") << ","
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

// TODO: replace LatencyMatrixBenchmark ** w/ MatrixBenchmark ** (malloc a new array MatrixBenchmark **)
void printMatrix(BenchmarkManager *bench_mgr, LatencyMatrixBenchmark **lat_mat_benchmarks, char *what) {

    Configurator *cfg = bench_mgr->config_;

    // uint32_t mem_regions_per_numa = config_.getMemoryRegionsPerNUMANode();

    printf("Measured %s (in %s)...\n", what, getMetricUnits(lat_mat_benchmarks[0]->mat_bench));
    printf("(Node, Reg) = (Memory NUMA Node, Region)\n\n");

    // std::vector<uint64_t> mem_regions_phys_addr = config_.getMemoryRegionsPhysAddresses();

    int width = allCoresSelected(cfg) ? 3 : 13;
    printf("%*c", width, ' ');
    for (uint32_t region_id = 0; region_id < bench_mgr->num_mem_regions_; region_id++) {
        printf(" (Node, Reg)");
    }
    printf("\n");

    uint32_t regions_per_pu = cfg->num_mem_regions_phys_addr_;
    // uint32_t regions_per_pu = config_.memoryRegionsInPhysAddr() ? mem_regions_phys_addr.size()
    //                                                             : mem_regions_per_numa * g_num_numa_nodes;

    if (allCoresSelected(cfg))
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
        printf("(?, %*d)", 7, mem_region);
    }

    for (uint32_t i = 0; i < bench_mgr->lat_mat_benchmarks_size_; i++) {
        if (i % regions_per_pu == 0) {
            printf("\n");
            uint32_t pu;
            if (allCoresSelected(cfg))
                pu = getCPUId(lat_mat_benchmarks[i]->mat_bench);
            else
                pu = 0; //mat_benchmarks_[i]->getCPUNode();
            printf("%*d", width, pu);
        }

        double median_metric = getMedianMetric(lat_mat_benchmarks[i]->mat_bench);
        // std::string metric_units = lat_mat_benchmarks_[i]->getMetricUnits();
        printf("%*f", 12, median_metric);
    }
    printf("\n");
}

// bool BenchmarkManager::runThroughputBenchmarks() {
//     if (!built_benchmarks_) {
//         if (!buildBenchmarks()) {
//             std::cerr << "ERROR: Failed to build benchmarks." << std::endl;
//             return false;
//         }
//     }

//     for (uint32_t i = 0; i < tp_benchmarks_.size(); i++) {
//         tp_benchmarks_[i]->run();
//         tp_benchmarks_[i]->reportResults(); //to console

//         //Write to results file if necessary
//         if (config_.useOutputFile()) {
//             results_file_ << tp_benchmarks_[i]->getName() << ",";
//             results_file_ << tp_benchmarks_[i]->getIterations() << ",";
//             results_file_ << static_cast<size_t>(tp_benchmarks_[i]->getLen() / tp_benchmarks_[i]->getNumThreads() / KB) << ",";
//             results_file_ << tp_benchmarks_[i]->getNumThreads() << ",";
//             results_file_ << tp_benchmarks_[i]->getNumThreads() << ",";
//             results_file_ << tp_benchmarks_[i]->getMemNode() << ",";
//             results_file_ << tp_benchmarks_[i]->getCPUNode() << ",";
//             pattern_mode_t pattern = tp_benchmarks_[i]->getPatternMode();
//             switch (pattern) {
//                 case SEQUENTIAL:
//                     results_file_ << "SEQUENTIAL" << ",";
//                     break;
//                 case RANDOM:
//                     results_file_ << "RANDOM" << ",";
//                     break;
//                 default:
//                     results_file_ << "UNKNOWN" << ",";
//                     break;
//             }

//             rw_mode_t rw_mode = tp_benchmarks_[i]->getRWMode();
//             switch (rw_mode) {
//                 case READ:
//                     results_file_ << "READ" << ",";
//                     break;
//                 case WRITE:
//                     results_file_ << "WRITE" << ",";
//                     break;
//                 default:
//                     results_file_ << "UNKNOWN" << ",";
//                     break;
//             }

//             chunk_size_t chunk_size = tp_benchmarks_[i]->getChunkSize();
//             switch (chunk_size) {
//                 case CHUNK_32b:
//                     results_file_ << "32" << ",";
//                     break;
// #ifdef HAS_WORD_64
//                 case CHUNK_64b:
//                     results_file_ << "64" << ",";
//                     break;
// #endif
// #ifdef HAS_WORD_128
//                 case CHUNK_128b:
//                     results_file_ << "128" << ",";
//                     break;
// #endif
// #ifdef HAS_WORD_256
//                 case CHUNK_256b:
//                     results_file_ << "256" << ",";
//                     break;
// #endif
// #ifdef HAS_WORD_512
//                 case CHUNK_512b:
//                     results_file_ << "512" << ",";
//                     break;
// #endif
//                 default:
//                     results_file_ << "UNKNOWN" << ",";
//                     break;
//             }

//             results_file_ << tp_benchmarks_[i]->getStrideSize() << ",";
//             results_file_ << tp_benchmarks_[i]->getMeanMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->getMinMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->get25PercentileMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->getMedianMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->get75PercentileMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->get95PercentileMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->get99PercentileMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->getMaxMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->getModeMetric() << ",";
//             results_file_ << tp_benchmarks_[i]->getMetricUnits() << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             for (uint32_t j = 0; j < g_num_physical_packages; j++) {
//                 results_file_ << tp_benchmarks_[i]->getMeanDRAMPower(j) << ",";
//                 results_file_ << tp_benchmarks_[i]->getPeakDRAMPower(j) << ",";
//             }
//             results_file_ << "N/A" << ",";
//             results_file_ << "" << ",";
//             results_file_ << std::endl;
//         }
//     }

//     if (g_verbose)
//         std::cout << std::endl << "Done running throughput benchmarks." << std::endl;

//     return true;
// }

// bool BenchmarkManager::runLatencyBenchmarks() {
//     if (!built_benchmarks_) {
//         if (!buildBenchmarks()) {
//             std::cerr << "ERROR: Failed to build benchmarks." << std::endl;
//             return false;
//         }
//     }

//     for (uint32_t i = 0; i < lat_benchmarks_.size(); i++) {
//         lat_benchmarks_[i]->run();
//         lat_benchmarks_[i]->reportResults(); //to console

//         //Write to results file if necessary
//         if (config_.useOutputFile()) {
//             results_file_ << lat_benchmarks_[i]->getName() << ",";
//             results_file_ << lat_benchmarks_[i]->getIterations() << ",";
//             results_file_ << static_cast<size_t>(lat_benchmarks_[i]->getLen() / lat_benchmarks_[i]->getNumThreads() / KB) << ",";
//             results_file_ << lat_benchmarks_[i]->getNumThreads() << ",";
//             results_file_ << lat_benchmarks_[i]->getNumThreads()-1 << ",";
//             results_file_ << lat_benchmarks_[i]->getMemNode() << ",";
//             results_file_ << lat_benchmarks_[i]->getCPUNode() << ",";
//             if (lat_benchmarks_[i]->getNumThreads() < 2) {
//                 results_file_ << "N/A" << ",";
//                 results_file_ << "N/A" << ",";
//                 results_file_ << "N/A" << ",";
//                 results_file_ << "N/A" << ",";
//             } else {
//                 pattern_mode_t pattern = lat_benchmarks_[i]->getPatternMode();
//                 switch (pattern) {
//                     case SEQUENTIAL:
//                         results_file_ << "SEQUENTIAL" << ",";
//                         break;
//                     case RANDOM:
//                         results_file_ << "RANDOM" << ",";
//                         break;
//                     default:
//                         results_file_ << "UNKNOWN" << ",";
//                         break;
//                 }

//                 rw_mode_t rw_mode = lat_benchmarks_[i]->getRWMode();
//                 switch (rw_mode) {
//                     case READ:
//                         results_file_ << "READ" << ",";
//                         break;
//                     case WRITE:
//                         results_file_ << "WRITE" << ",";
//                         break;
//                     default:
//                         results_file_ << "UNKNOWN" << ",";
//                         break;
//                 }

//                 chunk_size_t chunk_size = lat_benchmarks_[i]->getChunkSize();
//                 switch (chunk_size) {
//                     case CHUNK_32b:
//                         results_file_ << "32" << ",";
//                         break;
// #ifdef HAS_WORD_64
//                     case CHUNK_64b:
//                         results_file_ << "64" << ",";
//                         break;
// #endif
// #ifdef HAS_WORD_128
//                     case CHUNK_128b:
//                         results_file_ << "128" << ",";
//                         break;
// #endif
// #ifdef HAS_WORD_256
//                     case CHUNK_256b:
//                         results_file_ << "256" << ",";
//                         break;
// #endif
// #ifdef HAS_WORD_512
//                     case CHUNK_512b:
//                         results_file_ << "512" << ",";
//                         break;
// #endif
//                     default:
//                         results_file_ << "UNKNOWN" << ",";
//                         break;
//                 }

//                 results_file_ << lat_benchmarks_[i]->getStrideSize() << ",";
//             }

//             results_file_ << lat_benchmarks_[i]->getMeanLoadMetric() << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "MB/s" << ",";
//             results_file_ << lat_benchmarks_[i]->getMeanMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->getMinMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->get25PercentileMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->getMedianMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->get75PercentileMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->get95PercentileMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->get99PercentileMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->getMaxMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->getModeMetric() << ",";
//             results_file_ << lat_benchmarks_[i]->getMetricUnits() << ",";
//             for (uint32_t j = 0; j < g_num_physical_packages; j++) {
//                 results_file_ << lat_benchmarks_[i]->getMeanDRAMPower(j) << ",";
//                 results_file_ << lat_benchmarks_[i]->getPeakDRAMPower(j) << ",";
//             }
//             results_file_ << "N/A" << ",";
//             results_file_ << "" << ",";
//             results_file_ << std::endl;
//         }
//     }

//     if (g_verbose)
//         std::cout << std::endl << "Done running latency benchmarks." << std::endl;

//     return true;
// }

bool runLatencyMatrixBenchmarks(BenchmarkManager *bench_mgr) {
    if (! bench_mgr->built_benchmarks_) {
        if (! buildBenchmarks(bench_mgr)) {
            fprintf(stderr, "ERROR: Failed to build benchmarks.\n");
            return false;
        }
    }

//     std::vector<uint64_t> mem_regions_phys_addr;
//     if (config_.useDecNetFile()) {
//         mem_regions_phys_addr = config_.getMemoryRegionsPhysAddresses();
//     }

    for (uint32_t i = 0; i < bench_mgr->lat_mat_benchmarks_size_; i++) {
        run(bench_mgr->lat_mat_benchmarks_[i]);
        reportResults(bench_mgr->lat_mat_benchmarks_[i]->mat_bench);

        // if (config_.useDecNetFile() && (mem_regions_phys_addr.size() > 0)) {
        //     dec_net_results_file_ << "bench_result(";
        //     dec_net_results_file_ << lat_mat_benchmarks_[i]->getCPUId() << ", ";
        //     uint32_t region_id = lat_mat_benchmarks_[i]->getMemRegion();
        //     dec_net_results_file_ << mem_regions_phys_addr[region_id] << ", ";
        //     dec_net_results_file_ << "'latency'" << ", ";
        //     dec_net_results_file_ << lat_mat_benchmarks_[i]->getMedianMetric() << ", ";
        //     dec_net_results_file_ << "'" << lat_mat_benchmarks_[i]->getMetricUnits() << "'" << ").";
        //     dec_net_results_file_ << std::endl;
        // }
    }
    printf("\n");

//     // aggregated report of latency matrix benchmarks
//     std::vector<xmem::MatrixBenchmark *> mat_benchmarks_(lat_mat_benchmarks_.begin(), lat_mat_benchmarks_.end());
    printMatrix(bench_mgr, bench_mgr->lat_mat_benchmarks_, "idle latencies");

    if (g_verbose)
        printf("\nDone running latency matrix benchmarks.\n");

    return true;
}

// bool BenchmarkManager::runThroughputMatrixBenchmarks() {
//     if (!built_benchmarks_) {
//         if (!buildBenchmarks()) {
//             std::cerr << "ERROR: Failed to build benchmarks." << std::endl;
//             return false;
//         }
//     }

//     std::vector<uint64_t> mem_regions_phys_addr;
//     if (config_.useDecNetFile()) {
//         mem_regions_phys_addr = config_.getMemoryRegionsPhysAddresses();
//     }

//     for (uint32_t i = 0; i < thr_mat_benchmarks_.size(); i++) {
//         thr_mat_benchmarks_[i]->run();
//         thr_mat_benchmarks_[i]->reportResults(); //to console

//         //Write to results file if necessary
//         if (config_.useOutputFile()) {
//             results_file_ << thr_mat_benchmarks_[i]->getName() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getIterations() << ",";
//             results_file_ << static_cast<size_t>(thr_mat_benchmarks_[i]->getLen() / thr_mat_benchmarks_[i]->getNumThreads() / KB) << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getNumThreads() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getNumThreads()-1 << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getMemNode() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getCPUNode() << ",";
//             if (thr_mat_benchmarks_[i]->getNumThreads() < 2) {
//                 results_file_ << "N/A" << ",";
//                 results_file_ << "N/A" << ",";
//                 results_file_ << "N/A" << ",";
//                 results_file_ << "N/A" << ",";
//             } else {
//                 pattern_mode_t pattern = thr_mat_benchmarks_[i]->getPatternMode();
//                 switch (pattern) {
//                     case SEQUENTIAL:
//                         results_file_ << "SEQUENTIAL" << ",";
//                         break;
//                     case RANDOM:
//                         results_file_ << "RANDOM" << ",";
//                         break;
//                     default:
//                         results_file_ << "UNKNOWN" << ",";
//                         break;
//                 }

//                 rw_mode_t rw_mode = thr_mat_benchmarks_[i]->getRWMode();
//                 switch (rw_mode) {
//                     case READ:
//                         results_file_ << "READ" << ",";
//                         break;
//                     case WRITE:
//                         results_file_ << "WRITE" << ",";
//                         break;
//                     default:
//                         results_file_ << "UNKNOWN" << ",";
//                         break;
//                 }

//                 chunk_size_t chunk_size = thr_mat_benchmarks_[i]->getChunkSize();
//                 switch (chunk_size) {
//                     case CHUNK_32b:
//                         results_file_ << "32" << ",";
//                         break;
// #ifdef HAS_WORD_64
//                     case CHUNK_64b:
//                         results_file_ << "64" << ",";
//                         break;
// #endif
// #ifdef HAS_WORD_128
//                     case CHUNK_128b:
//                         results_file_ << "128" << ",";
//                         break;
// #endif
// #ifdef HAS_WORD_256
//                     case CHUNK_256b:
//                         results_file_ << "256" << ",";
//                         break;
// #endif
// #ifdef HAS_WORD_512
//                     case CHUNK_512b:
//                         results_file_ << "512" << ",";
//                         break;
// #endif
//                     default:
//                         results_file_ << "UNKNOWN" << ",";
//                         break;
//                 }

//                 results_file_ << thr_mat_benchmarks_[i]->getStrideSize() << ",";
//             }

//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "N/A" << ",";
//             results_file_ << "MB/s" << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getMeanMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getMinMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->get25PercentileMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getMedianMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->get75PercentileMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->get95PercentileMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->get99PercentileMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getMaxMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getModeMetric() << ",";
//             results_file_ << thr_mat_benchmarks_[i]->getMetricUnits() << ",";
//             for (uint32_t j = 0; j < g_num_physical_packages; j++) {
//                 results_file_ << thr_mat_benchmarks_[i]->getMeanDRAMPower(j) << ",";
//                 results_file_ << thr_mat_benchmarks_[i]->getPeakDRAMPower(j) << ",";
//             }
//             results_file_ << "N/A" << ",";
//             results_file_ << "" << ",";
//             results_file_ << std::endl;
//         }

//         if (config_.useDecNetFile() && (mem_regions_phys_addr.size() > 0)) {
//             dec_net_results_file_ << "bench_result(";
//             dec_net_results_file_ << thr_mat_benchmarks_[i]->getCPUId() << ", ";
//             uint32_t region_id = thr_mat_benchmarks_[i]->getMemRegion();
//             dec_net_results_file_ << mem_regions_phys_addr[region_id] << ", ";
//             dec_net_results_file_ << "'throughput'" << ", ";
//             dec_net_results_file_ << thr_mat_benchmarks_[i]->getMedianMetric() << ", ";
//             dec_net_results_file_ << "'" << thr_mat_benchmarks_[i]->getMetricUnits() << "'" << ").";
//             dec_net_results_file_ << std::endl;
//         }
//     }
//     std::cout << std::endl;

//     // aggregated report of throughput matrix benchmarks
//     std::vector<xmem::MatrixBenchmark *> mat_benchmarks_(thr_mat_benchmarks_.begin(), thr_mat_benchmarks_.end());
//     printMatrix(mat_benchmarks_, "unloaded throughputs");

//     if (g_verbose)
//         std::cout << std::endl << "Done running throughput matrix benchmarks." << std::endl;

//     return true;
// }

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
        // if (config_.syncMemory()) flags |= O_SYNC;
        flags |= O_SYNC;
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
                printf("Virtual address for memory region #%d: ", region_id);
                printf("0x%.16llx", (long long unsigned int) phys_addr);
                printf(" is 0x%.16llx", (long long unsigned int) virt_addr);

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

//     //Put the enumerations into vectors to make constructing benchmarks more loopable
//     std::vector<chunk_size_t> chunks;
//     if (config_.useChunk32b())
//         chunks.push_back(CHUNK_32b);
// #ifdef HAS_WORD_64
//     if (config_.useChunk64b())
//         chunks.push_back(CHUNK_64b);
// #endif
// #ifdef HAS_WORD_128
//     if (config_.useChunk128b())
//         chunks.push_back(CHUNK_128b);
// #endif
// #ifdef HAS_WORD_256
//     if (config_.useChunk256b())
//         chunks.push_back(CHUNK_256b);
// #endif
// #ifdef HAS_WORD_512
//     if (config_.useChunk512b())
//         chunks.push_back(CHUNK_512b);
// #endif

    // std::vector<rw_mode_t> rws;
    // if (config_.useReads())
    //     rws.push_back(READ);
    // if (config_.useWrites())
    //     rws.push_back(WRITE);

    // std::vector<int32_t> strides;
    // if (config_.useStrideP1())
    //     strides.push_back(1);
    // if (config_.useStrideN1())
    //     strides.push_back(-1);
    // if (config_.useStrideP2())
    //     strides.push_back(2);
    // if (config_.useStrideN2())
    //     strides.push_back(-2);
    // if (config_.useStrideP4())
    //     strides.push_back(4);
    // if (config_.useStrideN4())
    //     strides.push_back(-4);
    // if (config_.useStrideP8())
    //     strides.push_back(8);
    // if (config_.useStrideN8())
    //     strides.push_back(-8);
    // if (config_.useStrideP16())
    //     strides.push_back(16);
    // if (config_.useStrideN16())
    //     strides.push_back(-16);

    if (g_verbose)
        printf("\n");

    char benchmark_name[50];

    uint32_t num_processor_units; // Number of CPU nodes or CPUs to affinitize for benchmark experiments.
    uint32_t *processor_units; // List of CPU nodes or CPUs to affinitize for benchmark experiments.
    bool use_cpu_nodes = false;

    Configurator *cfg = bench_mgr->config_;

    if (allCoresSelected(cfg)) {
        num_processor_units = g_num_logical_cpus;
        processor_units = (uint32_t *) malloc(g_num_logical_cpus * sizeof(uint32_t));
        for (uint32_t i = 0; i < g_num_logical_cpus; i++) {
            processor_units[i] = i;
        }
    } else {
        num_processor_units = bench_mgr->num_cpu_numa_node_affinities_;
        processor_units = bench_mgr->cpu_numa_node_affinities_;
        use_cpu_nodes = true;
    }

    uint64_t *mem_regions_phys_addr = getMemoryRegionsPhysAddresses(cfg);

    uint32_t cpu      = -1;
    uint32_t cpu_node = -1;

    bench_mgr->lat_mat_benchmarks_size_ = num_processor_units * numberOfMemoryRegionsPhysAddresses(cfg);
    bench_mgr->lat_mat_benchmarks_ = (LatencyMatrixBenchmark **) malloc(bench_mgr->lat_mat_benchmarks_size_ * sizeof(LatencyMatrixBenchmark *));
    int l = 0;
    //Build latency matrix benchmarks
    for (int pu = 0; pu < num_processor_units; pu++) { //iterate each cpu NUMA node
        if (allCoresSelected(cfg)) {
            cpu = processor_units[pu];
        }
        else {
            cpu_node = processor_units[pu];
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
                                                                          SEQUENTIAL,
                                                                          READ,
                                                                          chunk,
                                                                          stride,
                                                                          benchmark_name);
            // if (lat_mat_benchmarks_[lat_mat_benchmarks_.size()-1] == NULL) {
            //     std::cerr << "ERROR: Failed to build a LatencyMatrixBenchmark!" << std::endl;
            //     return false;
            // }
            l++;
            g_test_index++;
        }
    }

    // uint32_t num_of_cpus_in_cpu_node = 0;
    // //Build throughput matrix benchmarks
    // for (auto pu = processor_units.cbegin(); pu != processor_units.cend(); pu++) { //iterate each cpu NUMA node
    //     if (config_.allCoresSelected()) {
    //         cpu = *pu;
    //     }
    //     else {
    //         cpu_node = *pu;
    //         // find out how many cores has the cpu NUMA node
    //         num_of_cpus_in_cpu_node = 0;
    //         for (uint32_t i = 0; i < g_num_logical_cpus; i++) {
    //             if (g_physical_package_of_cpu[i] == cpu_node) {
    //                 num_of_cpus_in_cpu_node++;
    //             }
    //         }
    //     }

        // for (uint32_t region_id = 0; region_id < mem_arrays_.size(); region_id++) {
        //     uint32_t mem_node = mem_array_node_[region_id];
        //     void* mem_array = mem_arrays_[region_id];
        //     size_t mem_array_len = mem_array_lens_[region_id];
        //     uint32_t mem_region = region_id;
        //     if (! config_.memoryRegionsInPhysAddr()) {
        //         mem_region = region_id % config_.getMemoryRegionsPerNUMANode();
        //     }

        //     for (uint32_t chunk_index = 0; chunk_index < chunks.size(); chunk_index++) { //iterate different chunk sizes
        //         chunk_size_t chunk = chunks[chunk_index];

        //         for (uint32_t stride_index = 0; stride_index < strides.size(); stride_index++) {  //iterate different stride lengths
        //             int32_t stride = strides[stride_index];

        //             // Throughput benchmark from every core to every memory region
        //             benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream()
        //                                 << "Test #" << g_test_index << "TM (ThroughputMatrix)"))->str();
        //             thr_mat_benchmarks_.push_back(
        //                 new ThroughputMatrixBenchmark(mem_array,
        //                     mem_array_len,
        //                     config_.getIterationsPerTest(),
        //                     use_cpu_nodes ? num_of_cpus_in_cpu_node : 1,
        //                     mem_node,
        //                     mem_region,
        //                     cpu_node,
        //                     cpu,
        //                     use_cpu_nodes,
        //                     SEQUENTIAL,
        //                     READ,
        //                     chunk,
        //                     stride,
        //                     dram_power_readers_,
        //                     benchmark_name,
        //                     thr_mat_logfile_));
        //             if (thr_mat_benchmarks_[thr_mat_benchmarks_.size()-1] == NULL) {
        //                 std::cerr << "ERROR: Failed to build a ThroughputMatrixBenchmark!" << std::endl;
        //                 return false;
        //             }
        //             g_test_index++;
        //         }
        //     }
        // }
    // }

    bench_mgr->built_benchmarks_ = true;

    return true;
}
