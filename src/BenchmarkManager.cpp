/* The MIT License (MIT)
 *
 * Copyright (c) 2014 Microsoft
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Mark Gottscho <mgottscho@ucla.edu>
 */

/**
 * @file
 *
 * @brief Implementation file for the BenchmarkManager class.
 */

//Headers
#include <BenchmarkManager.h>
#include <common.h>
#include <Configurator.h>

#ifdef EXT_DELAY_INJECTED_LOADED_LATENCY_BENCHMARK
#include <DelayInjectedLoadedLatencyBenchmark.h>
#endif

#ifdef EXT_STREAM_BENCHMARK
#include <StreamBenchmark.h> //TODO: implement this class
#endif

#ifdef _WIN32
#include <win/win_common_third_party.h>
#ifndef ARCH_ARM
#include <win/WindowsDRAMPowerReader.h> //Lacking library support for Windows on ARM
#endif
#endif

//Libraries
#include <assert.h>
#include <cstdint>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <numaif.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __gnu_linux__
#ifdef HAS_NUMA
#include <numa.h>
#endif
#ifdef HAS_LARGE_PAGES
extern "C" {
#include <hugetlbfs.h> //for allocating and freeing huge pages
}
#endif
#endif

using namespace xmem;

BenchmarkManager::BenchmarkManager(
        Configurator &config
    ) :
        config_(config),
        cpu_numa_node_affinities_(),
        memory_numa_node_affinities_(),
        mem_arrays_(),
#ifndef HAS_NUMA
        orig_malloc_addr_(NULL),
#endif
        mem_array_lens_(),
        tp_benchmarks_(),
        lat_benchmarks_(),
        lat_mat_benchmarks_(),
        thr_mat_benchmarks_(),
        dram_power_readers_(),
        results_file_(),
        built_benchmarks_(false)
    {
    //Set up DRAM power measurement
    for (uint32_t i = 0; i < g_num_physical_packages; i++) { //FIXME: this assumes that each physical package has a DRAM power measurement capability
        std::string power_obj_name = static_cast<std::ostringstream*>(&(std::ostringstream() << "Socket " << i << " DRAM"))->str();

#ifdef _WIN32
#ifndef ARCH_ARM //lacking library support for Windows on ARM
        //Put the thread on the last logical CPU in each NUMA node.
        dram_power_readers_.push_back(new WindowsDRAMPowerReader(cpu_id_in_numa_node(i,g_num_logical_cpus / g_num_numa_nodes - 1), POWER_SAMPLING_PERIOD_MS, 1, power_obj_name, cpu_id_in_numa_node(i,g_num_logical_cpus / g_num_numa_nodes - 1)));
#else
        dram_power_readers_.push_back(NULL);
#endif
#endif
#ifdef __gnu_linux__
        //TODO: Implement derived PowerReaders for Linux systems.
        dram_power_readers_.push_back(NULL);
#endif
    }

    //Set up NUMA stuff
    cpu_numa_node_affinities_ = config_.getCpuNumaNodeAffinities();
    memory_numa_node_affinities_ = config_.getMemoryNumaNodeAffinities();

    //Build working memory regions
    setupWorkingSets(config_.getWorkingSetSizePerThread());

    //Open results file
    if (config_.useOutputFile()) {
        results_file_.open(config_.getOutputFilename().c_str(), std::fstream::out);
        if (!results_file_.is_open()) {
            config_.setUseOutputFile(false);
            std::cerr << "WARNING: Failed to open " << config_.getOutputFilename() << " for writing! No results file will be generated." << std::endl;
        }

        //Generate file headers
        results_file_ << "Test Name,Iterations,Working Set Size Per Thread (KB),Total Number of Threads,Number of Load Generating Threads,NUMA Memory Node,NUMA CPU Node,Load Access Pattern,Load Read/Write Mix,Load Chunk Size (bits),Load Stride Size (chunks),Mean Load Throughput,Min Load Throughput,25th Percentile Load Throughput,Median Load Throughput,75th Percentile Load Throughput,95th Percentile Load Throughput,99th Percentile Load Throughput,Max Load Throughput,Mode Load Throughput,Throughput Units,Mean Latency,Min Latency,25th Percentile Latency,Median Latency,75th Percentile Latency,95th Percentile Latency,99th Percentile Latency,Max Latency,Mode Latency,Latency Units,";
        for (uint32_t i = 0; i < dram_power_readers_.size(); i++)  {
            if (dram_power_readers_[i] != NULL) {
                results_file_ << dram_power_readers_[i]->name() << " Mean Power (W),";
                results_file_ << dram_power_readers_[i]->name() << " Peak Power (W),";
            } else {
                results_file_ << "NAME? Mean Power (W),";
                results_file_ << "NAME? Peak Power (W),";
            }
        }
        results_file_ << "Extension Info,";
        results_file_ << "Notes,";
        results_file_ << std::endl;
    }
}

BenchmarkManager::~BenchmarkManager() {
    //Free throughput benchmarks
    for (uint32_t i = 0; i < tp_benchmarks_.size(); i++)
        delete tp_benchmarks_[i];
    //Free latency benchmarks
    for (uint32_t i = 0; i < lat_benchmarks_.size(); i++)
        delete lat_benchmarks_[i];
    //Free latency matrix benchmarks
    for (uint32_t i = 0; i < lat_mat_benchmarks_.size(); i++)
        delete lat_mat_benchmarks_[i];
    //Free throughput matrix benchmarks
    for (uint32_t i = 0; i < thr_mat_benchmarks_.size(); i++)
        delete thr_mat_benchmarks_[i];
    //Free memory arrays
    for (uint32_t i = 0; i < mem_arrays_.size(); i++)
        if (mem_arrays_[i] != nullptr) {
#ifdef _WIN32
            VirtualFreeEx(GetCurrentProcess(), mem_arrays_[i], 0, MEM_RELEASE);
#endif
#ifdef __gnu_linux__
#ifdef HAS_LARGE_PAGES
            if (config_.useLargePages())
                free_huge_pages(mem_arrays_[i]);
            else
#endif
#ifdef HAS_NUMA
                numa_free(mem_arrays_[i], mem_array_lens_[i]);
#else
                free(orig_malloc_addr_); //this is somewhat of a band-aid
#endif
#endif
        }
    //Close results file
    if (results_file_.is_open())
        results_file_.close();
}

bool BenchmarkManager::runAll() {
    bool success = true;

    if (config_.throughputTestSelected())
        success = success && runThroughputBenchmarks();
    if (config_.latencyTestSelected())
        success = success && runLatencyBenchmarks();
    if (config_.latencyMatrixTestSelected())
        success = success && runLatencyMatrixBenchmarks();
    if (config_.throughputMatrixTestSelected())
        success = success && runThroughputMatrixBenchmarks();

    return success;
}

bool BenchmarkManager::runThroughputBenchmarks() {
    if (!built_benchmarks_) {
        if (!buildBenchmarks()) {
            std::cerr << "ERROR: Failed to build benchmarks." << std::endl;
            return false;
        }
    }

    for (uint32_t i = 0; i < tp_benchmarks_.size(); i++) {
        tp_benchmarks_[i]->run();
        tp_benchmarks_[i]->reportResults(); //to console

        //Write to results file if necessary
        if (config_.useOutputFile()) {
            results_file_ << tp_benchmarks_[i]->getName() << ",";
            results_file_ << tp_benchmarks_[i]->getIterations() << ",";
            results_file_ << static_cast<size_t>(tp_benchmarks_[i]->getLen() / tp_benchmarks_[i]->getNumThreads() / KB) << ",";
            results_file_ << tp_benchmarks_[i]->getNumThreads() << ",";
            results_file_ << tp_benchmarks_[i]->getNumThreads() << ",";
            results_file_ << tp_benchmarks_[i]->getMemNode() << ",";
            results_file_ << tp_benchmarks_[i]->getCPUNode() << ",";
            pattern_mode_t pattern = tp_benchmarks_[i]->getPatternMode();
            switch (pattern) {
                case SEQUENTIAL:
                    results_file_ << "SEQUENTIAL" << ",";
                    break;
                case RANDOM:
                    results_file_ << "RANDOM" << ",";
                    break;
                default:
                    results_file_ << "UNKNOWN" << ",";
                    break;
            }

            rw_mode_t rw_mode = tp_benchmarks_[i]->getRWMode();
            switch (rw_mode) {
                case READ:
                    results_file_ << "READ" << ",";
                    break;
                case WRITE:
                    results_file_ << "WRITE" << ",";
                    break;
                default:
                    results_file_ << "UNKNOWN" << ",";
                    break;
            }

            chunk_size_t chunk_size = tp_benchmarks_[i]->getChunkSize();
            switch (chunk_size) {
                case CHUNK_32b:
                    results_file_ << "32" << ",";
                    break;
#ifdef HAS_WORD_64
                case CHUNK_64b:
                    results_file_ << "64" << ",";
                    break;
#endif
#ifdef HAS_WORD_128
                case CHUNK_128b:
                    results_file_ << "128" << ",";
                    break;
#endif
#ifdef HAS_WORD_256
                case CHUNK_256b:
                    results_file_ << "256" << ",";
                    break;
#endif
#ifdef HAS_WORD_512
                case CHUNK_512b:
                    results_file_ << "512" << ",";
                    break;
#endif
                default:
                    results_file_ << "UNKNOWN" << ",";
                    break;
            }

            results_file_ << tp_benchmarks_[i]->getStrideSize() << ",";
            results_file_ << tp_benchmarks_[i]->getMeanMetric() << ",";
            results_file_ << tp_benchmarks_[i]->getMinMetric() << ",";
            results_file_ << tp_benchmarks_[i]->get25PercentileMetric() << ",";
            results_file_ << tp_benchmarks_[i]->getMedianMetric() << ",";
            results_file_ << tp_benchmarks_[i]->get75PercentileMetric() << ",";
            results_file_ << tp_benchmarks_[i]->get95PercentileMetric() << ",";
            results_file_ << tp_benchmarks_[i]->get99PercentileMetric() << ",";
            results_file_ << tp_benchmarks_[i]->getMaxMetric() << ",";
            results_file_ << tp_benchmarks_[i]->getModeMetric() << ",";
            results_file_ << tp_benchmarks_[i]->getMetricUnits() << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            for (uint32_t j = 0; j < g_num_physical_packages; j++) {
                results_file_ << tp_benchmarks_[i]->getMeanDRAMPower(j) << ",";
                results_file_ << tp_benchmarks_[i]->getPeakDRAMPower(j) << ",";
            }
            results_file_ << "N/A" << ",";
            results_file_ << "" << ",";
            results_file_ << std::endl;
        }
    }

    if (g_verbose)
        std::cout << std::endl << "Done running throughput benchmarks." << std::endl;

    return true;
}

bool BenchmarkManager::runLatencyBenchmarks() {
    if (!built_benchmarks_) {
        if (!buildBenchmarks()) {
            std::cerr << "ERROR: Failed to build benchmarks." << std::endl;
            return false;
        }
    }

    for (uint32_t i = 0; i < lat_benchmarks_.size(); i++) {
        lat_benchmarks_[i]->run();
        lat_benchmarks_[i]->reportResults(); //to console

        //Write to results file if necessary
        if (config_.useOutputFile()) {
            results_file_ << lat_benchmarks_[i]->getName() << ",";
            results_file_ << lat_benchmarks_[i]->getIterations() << ",";
            results_file_ << static_cast<size_t>(lat_benchmarks_[i]->getLen() / lat_benchmarks_[i]->getNumThreads() / KB) << ",";
            results_file_ << lat_benchmarks_[i]->getNumThreads() << ",";
            results_file_ << lat_benchmarks_[i]->getNumThreads()-1 << ",";
            results_file_ << lat_benchmarks_[i]->getMemNode() << ",";
            results_file_ << lat_benchmarks_[i]->getCPUNode() << ",";
            if (lat_benchmarks_[i]->getNumThreads() < 2) {
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
            } else {
                pattern_mode_t pattern = lat_benchmarks_[i]->getPatternMode();
                switch (pattern) {
                    case SEQUENTIAL:
                        results_file_ << "SEQUENTIAL" << ",";
                        break;
                    case RANDOM:
                        results_file_ << "RANDOM" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                rw_mode_t rw_mode = lat_benchmarks_[i]->getRWMode();
                switch (rw_mode) {
                    case READ:
                        results_file_ << "READ" << ",";
                        break;
                    case WRITE:
                        results_file_ << "WRITE" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                chunk_size_t chunk_size = lat_benchmarks_[i]->getChunkSize();
                switch (chunk_size) {
                    case CHUNK_32b:
                        results_file_ << "32" << ",";
                        break;
#ifdef HAS_WORD_64
                    case CHUNK_64b:
                        results_file_ << "64" << ",";
                        break;
#endif
#ifdef HAS_WORD_128
                    case CHUNK_128b:
                        results_file_ << "128" << ",";
                        break;
#endif
#ifdef HAS_WORD_256
                    case CHUNK_256b:
                        results_file_ << "256" << ",";
                        break;
#endif
#ifdef HAS_WORD_512
                    case CHUNK_512b:
                        results_file_ << "512" << ",";
                        break;
#endif
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                results_file_ << lat_benchmarks_[i]->getStrideSize() << ",";
            }

            results_file_ << lat_benchmarks_[i]->getMeanLoadMetric() << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "MB/s" << ",";
            results_file_ << lat_benchmarks_[i]->getMeanMetric() << ",";
            results_file_ << lat_benchmarks_[i]->getMinMetric() << ",";
            results_file_ << lat_benchmarks_[i]->get25PercentileMetric() << ",";
            results_file_ << lat_benchmarks_[i]->getMedianMetric() << ",";
            results_file_ << lat_benchmarks_[i]->get75PercentileMetric() << ",";
            results_file_ << lat_benchmarks_[i]->get95PercentileMetric() << ",";
            results_file_ << lat_benchmarks_[i]->get99PercentileMetric() << ",";
            results_file_ << lat_benchmarks_[i]->getMaxMetric() << ",";
            results_file_ << lat_benchmarks_[i]->getModeMetric() << ",";
            results_file_ << lat_benchmarks_[i]->getMetricUnits() << ",";
            for (uint32_t j = 0; j < g_num_physical_packages; j++) {
                results_file_ << lat_benchmarks_[i]->getMeanDRAMPower(j) << ",";
                results_file_ << lat_benchmarks_[i]->getPeakDRAMPower(j) << ",";
            }
            results_file_ << "N/A" << ",";
            results_file_ << "" << ",";
            results_file_ << std::endl;
        }
    }

    if (g_verbose)
        std::cout << std::endl << "Done running latency benchmarks." << std::endl;

    return true;
}

bool BenchmarkManager::runLatencyMatrixBenchmarks() {
    if (!built_benchmarks_) {
        if (!buildBenchmarks()) {
            std::cerr << "ERROR: Failed to build benchmarks." << std::endl;
            return false;
        }
    }

    for (uint32_t i = 0; i < lat_mat_benchmarks_.size(); i++) {
        lat_mat_benchmarks_[i]->run();
        lat_mat_benchmarks_[i]->reportResults(); //to console

        //Write to results file if necessary
        if (config_.useOutputFile()) {
            results_file_ << lat_mat_benchmarks_[i]->getName() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getIterations() << ",";
            results_file_ << static_cast<size_t>(lat_mat_benchmarks_[i]->getLen() / lat_mat_benchmarks_[i]->getNumThreads() / KB) << ",";
            results_file_ << lat_mat_benchmarks_[i]->getNumThreads() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getNumThreads()-1 << ",";
            results_file_ << lat_mat_benchmarks_[i]->getMemNode() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getCPUNode() << ",";
            if (lat_mat_benchmarks_[i]->getNumThreads() < 2) {
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
            } else {
                pattern_mode_t pattern = lat_mat_benchmarks_[i]->getPatternMode();
                switch (pattern) {
                    case SEQUENTIAL:
                        results_file_ << "SEQUENTIAL" << ",";
                        break;
                    case RANDOM:
                        results_file_ << "RANDOM" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                rw_mode_t rw_mode = lat_mat_benchmarks_[i]->getRWMode();
                switch (rw_mode) {
                    case READ:
                        results_file_ << "READ" << ",";
                        break;
                    case WRITE:
                        results_file_ << "WRITE" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                chunk_size_t chunk_size = lat_mat_benchmarks_[i]->getChunkSize();
                switch (chunk_size) {
                    case CHUNK_32b:
                        results_file_ << "32" << ",";
                        break;
#ifdef HAS_WORD_64
                    case CHUNK_64b:
                        results_file_ << "64" << ",";
                        break;
#endif
#ifdef HAS_WORD_128
                    case CHUNK_128b:
                        results_file_ << "128" << ",";
                        break;
#endif
#ifdef HAS_WORD_256
                    case CHUNK_256b:
                        results_file_ << "256" << ",";
                        break;
#endif
#ifdef HAS_WORD_512
                    case CHUNK_512b:
                        results_file_ << "512" << ",";
                        break;
#endif
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                results_file_ << lat_mat_benchmarks_[i]->getStrideSize() << ",";
            }

            results_file_ << lat_mat_benchmarks_[i]->getMeanLoadMetric() << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "MB/s" << ",";
            results_file_ << lat_mat_benchmarks_[i]->getMeanMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getMinMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->get25PercentileMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getMedianMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->get75PercentileMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->get95PercentileMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->get99PercentileMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getMaxMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getModeMetric() << ",";
            results_file_ << lat_mat_benchmarks_[i]->getMetricUnits() << ",";
            for (uint32_t j = 0; j < g_num_physical_packages; j++) {
                results_file_ << lat_mat_benchmarks_[i]->getMeanDRAMPower(j) << ",";
                results_file_ << lat_mat_benchmarks_[i]->getPeakDRAMPower(j) << ",";
            }
            results_file_ << "N/A" << ",";
            results_file_ << "" << ",";
            results_file_ << std::endl;
        }
    }
    std::cout << std::endl;

    // aggregated report of latency matrix benchmarks
    uint32_t mem_regions_per_numa = config_.getMemoryRegionsPerNUMANode();

    std::cout <<  "Measured idle latencies (in " << lat_mat_benchmarks_[0]->getMetricUnits() << ")..." << std::endl;
    std::cout << "(Node, Reg) = " << "(Memory NUMA Node, Region)" << std::endl << std::endl;

    int width = config_.allCoresSelected() ? 3 : 13;
    std::cout << std::setw(width) << " ";
    for (auto it = memory_numa_node_affinities_.cbegin(); it != memory_numa_node_affinities_.cend(); it++) {
        for (uint32_t mem_region = 0; mem_region < mem_regions_per_numa; mem_region++) {
            std::cout << " (Node, Reg)";
        }
    }
    std::cout << std::endl;

    std::cout << (config_.allCoresSelected() ? "CPU" : "CPU NUMA Node");
    for (auto it = memory_numa_node_affinities_.cbegin(); it != memory_numa_node_affinities_.cend(); it++) {
        for (uint32_t mem_region = 0; mem_region < mem_regions_per_numa; mem_region++) {
            std::cout  << std::setw(12) << "(" + std::to_string(*it) + ", " + std::to_string(mem_region) + ")";
        }
    }

    for (uint32_t i = 0; i < lat_mat_benchmarks_.size(); i++) {
        if (i % (mem_regions_per_numa * g_num_numa_nodes) == 0) {
            std::cout << std::endl;
            uint32_t pu = config_.allCoresSelected() ? lat_mat_benchmarks_[i]->getCPUId() : lat_mat_benchmarks_[i]->getCPUNode();
            std::cout << std::setw(width) << pu;
        }

        double median_metric = lat_mat_benchmarks_[i]->getMedianMetric();
        // std::string metric_units = lat_mat_benchmarks_[i]->getMetricUnits();
        std::cout << std::setw(12) << median_metric;
    }
    std::cout << std::endl;

    if (g_verbose)
        std::cout << std::endl << "Done running latency matrix benchmarks." << std::endl;

    return true;
}

bool BenchmarkManager::runThroughputMatrixBenchmarks() {
    if (!built_benchmarks_) {
        if (!buildBenchmarks()) {
            std::cerr << "ERROR: Failed to build benchmarks." << std::endl;
            return false;
        }
    }

    for (uint32_t i = 0; i < thr_mat_benchmarks_.size(); i++) {
        thr_mat_benchmarks_[i]->run();
        thr_mat_benchmarks_[i]->reportResults(); //to console

        //Write to results file if necessary
        if (config_.useOutputFile()) {
            results_file_ << thr_mat_benchmarks_[i]->getName() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getIterations() << ",";
            results_file_ << static_cast<size_t>(thr_mat_benchmarks_[i]->getLen() / thr_mat_benchmarks_[i]->getNumThreads() / KB) << ",";
            results_file_ << thr_mat_benchmarks_[i]->getNumThreads() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getNumThreads()-1 << ",";
            results_file_ << thr_mat_benchmarks_[i]->getMemNode() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getCPUNode() << ",";
            if (thr_mat_benchmarks_[i]->getNumThreads() < 2) {
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
            } else {
                pattern_mode_t pattern = thr_mat_benchmarks_[i]->getPatternMode();
                switch (pattern) {
                    case SEQUENTIAL:
                        results_file_ << "SEQUENTIAL" << ",";
                        break;
                    case RANDOM:
                        results_file_ << "RANDOM" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                rw_mode_t rw_mode = thr_mat_benchmarks_[i]->getRWMode();
                switch (rw_mode) {
                    case READ:
                        results_file_ << "READ" << ",";
                        break;
                    case WRITE:
                        results_file_ << "WRITE" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                chunk_size_t chunk_size = thr_mat_benchmarks_[i]->getChunkSize();
                switch (chunk_size) {
                    case CHUNK_32b:
                        results_file_ << "32" << ",";
                        break;
#ifdef HAS_WORD_64
                    case CHUNK_64b:
                        results_file_ << "64" << ",";
                        break;
#endif
#ifdef HAS_WORD_128
                    case CHUNK_128b:
                        results_file_ << "128" << ",";
                        break;
#endif
#ifdef HAS_WORD_256
                    case CHUNK_256b:
                        results_file_ << "256" << ",";
                        break;
#endif
#ifdef HAS_WORD_512
                    case CHUNK_512b:
                        results_file_ << "512" << ",";
                        break;
#endif
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                results_file_ << thr_mat_benchmarks_[i]->getStrideSize() << ",";
            }

            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "MB/s" << ",";
            results_file_ << thr_mat_benchmarks_[i]->getMeanMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getMinMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->get25PercentileMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getMedianMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->get75PercentileMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->get95PercentileMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->get99PercentileMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getMaxMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getModeMetric() << ",";
            results_file_ << thr_mat_benchmarks_[i]->getMetricUnits() << ",";
            for (uint32_t j = 0; j < g_num_physical_packages; j++) {
                results_file_ << thr_mat_benchmarks_[i]->getMeanDRAMPower(j) << ",";
                results_file_ << thr_mat_benchmarks_[i]->getPeakDRAMPower(j) << ",";
            }
            results_file_ << "N/A" << ",";
            results_file_ << "" << ",";
            results_file_ << std::endl;
        }
    }
    std::cout << std::endl;

    // aggregated report of throughput matrix benchmarks
    uint32_t mem_regions_per_numa = config_.getMemoryRegionsPerNUMANode();

    std::cout <<  "Measured unloaded throughputs (in " << thr_mat_benchmarks_[0]->getMetricUnits() << ")..." << std::endl;
    std::cout << "(Node, Reg) = " << "(Memory NUMA Node, Region)" << std::endl << std::endl; //Change the output and for latency.

    int width = config_.allCoresSelected() ? 3 : 13;
    std::cout << std::setw(width) << " ";
    for (auto it = memory_numa_node_affinities_.cbegin(); it != memory_numa_node_affinities_.cend(); it++) {
        for (uint32_t mem_region = 0; mem_region < mem_regions_per_numa; mem_region++) {
            std::cout << " (Node, Reg)";
        }
    }
    std::cout << std::endl;

    std::cout << (config_.allCoresSelected() ? "CPU" : "CPU NUMA Node");
    for (auto it = memory_numa_node_affinities_.cbegin(); it != memory_numa_node_affinities_.cend(); it++) {
        for (uint32_t mem_region = 0; mem_region < mem_regions_per_numa; mem_region++) {
            std::cout  << std::setw(12) << "(" + std::to_string(*it) + ", " + std::to_string(mem_region) + ")";
        }
    }

    for (uint32_t i = 0; i < thr_mat_benchmarks_.size(); i++) {
        if (i % (mem_regions_per_numa * g_num_numa_nodes) == 0) {
            std::cout << std::endl;
            uint32_t pu = config_.allCoresSelected() ? thr_mat_benchmarks_[i]->getCPUId() : thr_mat_benchmarks_[i]->getCPUNode();
            std::cout << std::setw(width) << pu;
        }

        double median_metric = thr_mat_benchmarks_[i]->getMedianMetric();
        std::cout << std::setw(12) << median_metric;
    }
    std::cout << std::endl;

    if (g_verbose)
        std::cout << std::endl << "Done running throughput matrix benchmarks." << std::endl;

    return true;
}

void BenchmarkManager::setupWorkingSets(size_t working_set_size) {
    //Allocate memory in each NUMA node to be tested

    uint32_t mem_regions_per_numa = config_.getMemoryRegionsPerNUMANode();

    std::vector<uint64_t> mem_regions_phys_addr = config_.getMemoryRegionsPhysAddresses();
    bool use_physical_addresses = ! mem_regions_phys_addr.empty();

    if (! use_physical_addresses) {
        //We reserve the space for these, but that doesn't mean they will all be used.
        mem_arrays_.resize(g_num_numa_nodes * mem_regions_per_numa);
        mem_array_lens_.resize(g_num_numa_nodes * mem_regions_per_numa);

        for (auto it = memory_numa_node_affinities_.cbegin(); it != memory_numa_node_affinities_.cend(); it++) {
            for (uint32_t mem_region = 0; mem_region < mem_regions_per_numa; mem_region++) {
                size_t allocation_size = 0;
                uint32_t numa_node = *it;        // std::cout << "Min: " << min_metric_ << " " << metric_units_;
                uint32_t region_id = numa_node * mem_regions_per_numa + mem_region;

#ifdef HAS_LARGE_PAGES
                if (config_.useLargePages()) {
                    size_t remainder = 0;
                    //For large pages, working set size could be less than a single large page. So let's allocate the right amount of memory, which is the working set size rounded up to nearest large page, which could be more than we actually use.
                    if (config_.getNumWorkerThreads() * working_set_size < g_large_page_size)
                        allocation_size = g_large_page_size;
                    else {
                        remainder = (config_.getNumWorkerThreads() * working_set_size) % g_large_page_size;
                        allocation_size = (config_.getNumWorkerThreads() * working_set_size) + remainder;
                    }

#ifdef _WIN32
                    //Make sure we have necessary privileges
                    HANDLE hToken;
                    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
                        std::cerr << "ERROR: Failed to open process token to adjust privileges! Did you remember to run in Administrator mode?" << std::endl;
                        exit(-1);
                    }
                    if (!SetPrivilege(hToken,"SeLockMemoryPrivilege", true)) {
                        std::cerr << "ERROR: Failed to adjust privileges to allow locking memory pages! Did you remember to run in Administrator mode?" << std::endl;
                        exit(-1);
                    }
                    CloseHandle(hToken);

                    mem_arrays_[numa_node] = VirtualAllocExNuma(GetCurrentProcess(), NULL, allocation_size, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE, numa_node); //Windows NUMA allocation. Make the allocation one page bigger than necessary so that we can do alignment.
#endif
#ifdef __gnu_linux__
                    mem_arrays_[numa_node] = get_huge_pages(allocation_size, GHP_DEFAULT);
#endif
                } else { //Non-large pages (nominal case)
#endif
                    //Under normal (not large-page) operation, working set size is a multiple of regular pages.
                    allocation_size = config_.getNumWorkerThreads() * working_set_size + g_page_size;
#ifdef _WIN32
                    mem_arrays_[region_id] = VirtualAllocExNuma(GetCurrentProcess(), NULL, allocation_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, numa_node); //Windows NUMA allocation. Make the allocation one page bigger than necessary so that we can do alignment.
#endif
#ifdef __gnu_linux__
#ifdef HAS_NUMA
                    numa_set_strict(1); //Enforce NUMA memory allocation to land on specified node or fail otherwise. Alternative node fallback is forbidden.
                    mem_arrays_[region_id] = numa_alloc_onnode(allocation_size, numa_node);
#endif
#ifndef HAS_NUMA //special case
                    mem_arrays_[region_id] = malloc(allocation_size);
                    orig_malloc_addr_ = mem_arrays_[numa_node];
#endif
#endif
#ifdef HAS_LARGE_PAGES
                }
#endif

                if (mem_arrays_[region_id] != nullptr) {
                    mem_array_lens_[region_id] = config_.getNumWorkerThreads() * working_set_size;
                }
                else {
                    std::cerr << "ERROR: Failed to allocate " << allocation_size << " B on NUMA node " << numa_node << " for " << config_.getNumWorkerThreads() << " worker threads." << std::endl;
                    exit(-1);
                }

                if (g_verbose) {
                    std::cout << "Virtual address for memory region #" << mem_region << " on NUMA node " << numa_node << ": ";
                    std::printf("0x%.16llX", reinterpret_cast<long long unsigned int>(mem_arrays_[region_id]));
                    std::cout << std::endl;
                }

                if (config_.latencyMatrixTestSelected() || config_.throughputMatrixTestSelected()) {
                    std::cout << "Virtual address for memory region #" << mem_region << " on NUMA node " << numa_node << ": ";
                    std::printf("0x%.16llX", reinterpret_cast<long long unsigned int>(mem_arrays_[region_id]));
                    std::cout << std::endl;
                }

                //upwards alignment to page boundary
                uintptr_t mask;
                if (config_.useLargePages())
                    mask = static_cast<uintptr_t>(g_large_page_size)-1;
                else
                    mask = static_cast<uintptr_t>(g_page_size)-1; //e.g. 4095 bytes
                uintptr_t tmp_ptr = reinterpret_cast<uintptr_t>(mem_arrays_[region_id]);
                uintptr_t aligned_addr = (tmp_ptr + mask) & ~mask; //add one page to the address, then truncate least significant bits of address to be page aligned.
                mem_arrays_[region_id] = reinterpret_cast<void*>(aligned_addr);

                if (g_verbose) {
                    std::cout << " --- ALIGNED --> ";
                    std::printf("0x%.16llX", reinterpret_cast<long long unsigned int>(mem_arrays_[region_id]));
                    std::cout << std::endl;
                }
            }
        }
    } else {
        int fd = open("/dev/mem", O_SYNC);
        if (fd < 0) {
            perror("Failed to open /dev/mem.");
        }

        for (uint32_t region_id = 0; region_id < mem_regions_phys_addr.size(); region_id++) {

            size_t allocation_size = working_set_size;
            size_t phys_addr = mem_regions_phys_addr[region_id];

            // Truncate offset to a multiple of the page size, or mmap will fail.
            size_t page_size = sysconf(_SC_PAGE_SIZE);
            size_t page_base = (phys_addr / page_size) * page_size;
            size_t page_offset = phys_addr - page_base;
            size_t len = page_offset + allocation_size;


            void *virt_addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, page_base);

            mem_arrays_[region_id] = virt_addr;
            mem_array_lens_[region_id] = allocation_size;

            if (config_.latencyMatrixTestSelected() || config_.throughputMatrixTestSelected()) {
                std::cout << "Virtual address for memory region ";
                std::printf("0x%.16llX", static_cast<long long unsigned int>(phys_addr));
                std::cout << "is ";
                std::printf("0x%.16llX", reinterpret_cast<long long unsigned int>(virt_addr));

                size_t num_pages = len / page_size;
                num_pages += ((len % page_size) > 0) ? 1 : 0;
                int *status = (int *) malloc(num_pages * sizeof(int));
                move_pages(0 /*self memory */, 1, &virt_addr, NULL, status, 0);
                std::cout << " on NUMA node " << status[0] << "  (first page)";
                if (num_pages > 1) {
                    std::cout << " and on NUMA node " << status[num_pages - 1] << "(last page, " << num_pages << ")";
                }
                std::cout << std::endl;
                free(status);
            }
        }
    }
    std::cout << std::endl;
}

bool BenchmarkManager::buildBenchmarks() {
    if (g_verbose)  {
        std::cout << std::endl;
        std::cout << "Generating benchmarks." << std::endl;
    }

    //Put the enumerations into vectors to make constructing benchmarks more loopable
    std::vector<chunk_size_t> chunks;
    if (config_.useChunk32b())
        chunks.push_back(CHUNK_32b);
#ifdef HAS_WORD_64
    if (config_.useChunk64b())
        chunks.push_back(CHUNK_64b);
#endif
#ifdef HAS_WORD_128
    if (config_.useChunk128b())
        chunks.push_back(CHUNK_128b);
#endif
#ifdef HAS_WORD_256
    if (config_.useChunk256b())
        chunks.push_back(CHUNK_256b);
#endif
#ifdef HAS_WORD_512
    if (config_.useChunk512b())
        chunks.push_back(CHUNK_512b);
#endif

    std::vector<rw_mode_t> rws;
    if (config_.useReads())
        rws.push_back(READ);
    if (config_.useWrites())
        rws.push_back(WRITE);

    std::vector<int32_t> strides;
    if (config_.useStrideP1())
        strides.push_back(1);
    if (config_.useStrideN1())
        strides.push_back(-1);
    if (config_.useStrideP2())
        strides.push_back(2);
    if (config_.useStrideN2())
        strides.push_back(-2);
    if (config_.useStrideP4())
        strides.push_back(4);
    if (config_.useStrideN4())
        strides.push_back(-4);
    if (config_.useStrideP8())
        strides.push_back(8);
    if (config_.useStrideN8())
        strides.push_back(-8);
    if (config_.useStrideP16())
        strides.push_back(16);
    if (config_.useStrideN16())
        strides.push_back(-16);

    if (g_verbose)
        std::cout << std::endl;

    std::string benchmark_name;

    //Build throughput benchmarks. This is a humongous nest of for loops, but rest assured, the range of each loop should be small enough. The problem is we have many combinations to test.
    for (auto mem_node_it = memory_numa_node_affinities_.cbegin(); mem_node_it != memory_numa_node_affinities_.cend(); mem_node_it++) { //iterate each memory NUMA node
        for (uint32_t mem_region = 0; mem_region < config_.getMemoryRegionsPerNUMANode(); mem_region++) {
            uint32_t mem_node = *mem_node_it;
            uint32_t region_id = mem_node * config_.getMemoryRegionsPerNUMANode() + mem_region;
            void* mem_array = mem_arrays_[region_id];
            size_t mem_array_len = mem_array_lens_[region_id];

            for (auto cpu_node_it = cpu_numa_node_affinities_.cbegin(); cpu_node_it != cpu_numa_node_affinities_.cend(); cpu_node_it++) { //iterate each cpu NUMA node
                uint32_t cpu_node = *cpu_node_it;
                bool buildLatBench = true; //Want to get at least one latency benchmark for all NUMA node combos

                //DO SEQUENTIAL/STRIDED TESTS
                if (config_.useSequentialAccessPattern()) {
                    for (uint32_t rw_index = 0; rw_index < rws.size(); rw_index++) { //iterate read/write access types
                        rw_mode_t rw = rws[rw_index];

                        for (uint32_t chunk_index = 0; chunk_index < chunks.size(); chunk_index++) { //iterate different chunk sizes
                            chunk_size_t chunk = chunks[chunk_index];

                            for (uint32_t stride_index = 0; stride_index < strides.size(); stride_index++) {  //iterate different stride lengths
                                int32_t stride = strides[stride_index];

                                //Add the throughput benchmark
                                benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream() << "Test #" << g_test_index << "T (Throughput)"))->str();
                                tp_benchmarks_.push_back(new ThroughputBenchmark(mem_array,
                                                                            mem_array_len,
                                                                            config_.getIterationsPerTest(),
                                                                            config_.getNumWorkerThreads(),
                                                                            mem_node,
                                                                            cpu_node,
                                                                            SEQUENTIAL,
                                                                            rw,
                                                                            chunk,
                                                                            stride,
                                                                            dram_power_readers_,
                                                                            benchmark_name));
                                if (tp_benchmarks_[tp_benchmarks_.size()-1] == NULL) {
                                    std::cerr << "ERROR: Failed to build a ThroughputBenchmark!" << std::endl;
                                    return false;
                                }

                                //Add the latency benchmark

                                //Special case: number of worker threads is 1, only need 1 latency thread in general to do unloaded latency tests.
                                if (config_.getNumWorkerThreads() > 1 || buildLatBench) {
                                    benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream() << "Test #" << g_test_index << "L (Latency)"))->str();
                                    lat_benchmarks_.push_back(new LatencyBenchmark(mem_array,
                                                                                    mem_array_len,
                                                                                    config_.getIterationsPerTest(),
                                                                                    config_.getNumWorkerThreads(),
                                                                                    mem_node,
                                                                                    cpu_node,
                                                                                    SEQUENTIAL,
                                                                                    rw,
                                                                                    chunk,
                                                                                    stride,
                                                                                    dram_power_readers_,
                                                                                    benchmark_name));
                                    if (lat_benchmarks_[lat_benchmarks_.size()-1] == NULL) {
                                        std::cerr << "ERROR: Failed to build a LatencyBenchmark!" << std::endl;
                                        return false;
                                    }
                                    buildLatBench = false; //Wait for next NUMA combo
                                }
                                g_test_index++;
                            }
                        }
                    }
                }

                if (config_.useRandomAccessPattern()) {
                    //DO RANDOM TESTS
                    for (uint32_t rw_index = 0; rw_index < rws.size(); rw_index++) { //iterate read/write access types
                        rw_mode_t rw = rws[rw_index];

                        for (uint32_t chunk_index = 0; chunk_index < chunks.size(); chunk_index++) { //iterate different chunk sizes
                            chunk_size_t chunk = chunks[chunk_index];

                            if (chunk == CHUNK_32b) //Special case: random load workers cannot use 32-bit chunks, so skip this benchmark combination
                                continue;

                            //Add the throughput benchmark
                            benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream() << "Test #" << g_test_index << "T (Throughput)"))->str();
                            tp_benchmarks_.push_back(new ThroughputBenchmark(mem_array,
                                                                            mem_array_len,
                                                                            config_.getIterationsPerTest(),
                                                                            config_.getNumWorkerThreads(),
                                                                            mem_node,
                                                                            cpu_node,
                                                                            RANDOM,
                                                                            rw,
                                                                            chunk,
                                                                            0,
                                                                            dram_power_readers_,
                                                                            benchmark_name));
                            if (tp_benchmarks_[tp_benchmarks_.size()-1] == NULL) {
                                std::cerr << "ERROR: Failed to build a ThroughputBenchmark!" << std::endl;
                                return false;
                            }

                            //Add the latency benchmark
                            //Special case: number of worker threads is 1, only need 1 latency thread in general to do unloaded latency tests.
                            if (config_.getNumWorkerThreads() > 1 || buildLatBench) {
                                benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream() << "Test #" << g_test_index << "L (Latency)"))->str();
                                lat_benchmarks_.push_back(new LatencyBenchmark(mem_array,
                                                                                mem_array_len,
                                                                                config_.getIterationsPerTest(),
                                                                                config_.getNumWorkerThreads(),
                                                                                mem_node,
                                                                                cpu_node,
                                                                                RANDOM,
                                                                                rw,
                                                                                chunk,
                                                                                0,
                                                                                dram_power_readers_,
                                                                                benchmark_name));
                                if (lat_benchmarks_[lat_benchmarks_.size()-1] == NULL) {
                                    std::cerr << "ERROR: Failed to build a LatencyBenchmark!" << std::endl;
                                    return false;
                                }

                                buildLatBench = false; //Wait for next NUMA combo
                            }

                            g_test_index++;
                        }
                    }
                }
            }
        }
    }

    std::list<uint32_t> processor_units; // List of CPU nodes or CPUs to affinitize for benchmark experiments.
    bool use_cpu_nodes = false;

    if (config_.allCoresSelected()) {
        for (uint32_t i = 0; i < g_num_logical_cpus; i++) {
            processor_units.push_back(i);
        }
    } else {
        processor_units = cpu_numa_node_affinities_;
        use_cpu_nodes = true;
    }

    uint32_t cpu      = -1;
    uint32_t cpu_node = -1;
    //Build latency matrix benchmarks
    for (auto pu = processor_units.cbegin(); pu != processor_units.cend(); pu++) { //iterate each cpu NUMA node
        if (config_.allCoresSelected()) {
            cpu = *pu;
        }
        else {
            cpu_node = *pu;
        }

        for (auto mem_node_it = memory_numa_node_affinities_.cbegin(); mem_node_it != memory_numa_node_affinities_.cend(); mem_node_it++) { //iterate each memory NUMA node
            for (uint32_t mem_region = 0; mem_region < config_.getMemoryRegionsPerNUMANode(); mem_region++) {
                uint32_t mem_node = *mem_node_it;
                uint32_t region_id = mem_node * config_.getMemoryRegionsPerNUMANode() + mem_region;
                void* mem_array = mem_arrays_[region_id];
                size_t mem_array_len = mem_array_lens_[region_id];

                for (uint32_t chunk_index = 0; chunk_index < chunks.size(); chunk_index++) { //iterate different chunk sizes
                    chunk_size_t chunk = chunks[chunk_index];

                    for (uint32_t stride_index = 0; stride_index < strides.size(); stride_index++) {  //iterate different stride lengths
                        int32_t stride = strides[stride_index];

                        // Latency benchmark from every core to every memory region
                        benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream()
                                            << "Test #" << g_test_index << "LM (LatencyMatrix)"))->str();
                        lat_mat_benchmarks_.push_back(new LatencyMatrixBenchmark(mem_array,
                                                                                   mem_array_len,
                                                                                   config_.getIterationsPerTest(),
                                                                                   config_.getNumWorkerThreads(),
                                                                                   mem_node,
                                                                                   mem_region,
                                                                                   cpu_node,
                                                                                   cpu,
                                                                                   use_cpu_nodes,
                                                                                   SEQUENTIAL,
                                                                                   READ,
                                                                                   chunk,
                                                                                   stride,
                                                                                   dram_power_readers_,
                                                                                   benchmark_name));
                        if (lat_mat_benchmarks_[lat_mat_benchmarks_.size()-1] == NULL) {
                            std::cerr << "ERROR: Failed to build a LatencyMatrixBenchmark!" << std::endl;
                            return false;
                        }
                        g_test_index++;
                    }
                }
            }
        }
    }

    //Build throughput matrix benchmarks
    for (auto pu = processor_units.cbegin(); pu != processor_units.cend(); pu++) { //iterate each cpu NUMA node
        if (config_.allCoresSelected()) {
            cpu = *pu;
        }
        else {
            cpu_node = *pu;
        }

        for (auto mem_node_it = memory_numa_node_affinities_.cbegin(); mem_node_it != memory_numa_node_affinities_.cend(); mem_node_it++) { //iterate each memory NUMA node
            for (uint32_t mem_region = 0; mem_region < config_.getMemoryRegionsPerNUMANode(); mem_region++) {
                uint32_t mem_node = *mem_node_it;
                uint32_t region_id = mem_node * config_.getMemoryRegionsPerNUMANode() + mem_region;
                void* mem_array = mem_arrays_[region_id];
                size_t mem_array_len = mem_array_lens_[region_id];

                for (uint32_t chunk_index = 0; chunk_index < chunks.size(); chunk_index++) { //iterate different chunk sizes
                    chunk_size_t chunk = chunks[chunk_index];

                    for (uint32_t stride_index = 0; stride_index < strides.size(); stride_index++) {  //iterate different stride lengths
                        int32_t stride = strides[stride_index];

                        // Throughput benchmark from every core to every memory region
                        benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream()
                                            << "Test #" << g_test_index << "TM (ThroughputMatrix)"))->str();
                        thr_mat_benchmarks_.push_back(
                            new ThroughputMatrixBenchmark(mem_array,
                                mem_array_len,
                                config_.getIterationsPerTest(),
                                g_num_logical_cpus, // get all cores requesting data from memory in order to achieve higher b/w
                                mem_node,
                                mem_region,
                                cpu_node,
                                cpu,
                                use_cpu_nodes,
                                SEQUENTIAL,
                                READ,
                                chunk,
                                stride,
                                dram_power_readers_,
                                benchmark_name));
                        if (thr_mat_benchmarks_[thr_mat_benchmarks_.size()-1] == NULL) {
                            std::cerr << "ERROR: Failed to build a ThroughputMatrixBenchmark!" << std::endl;
                            return false;
                        }
                        g_test_index++;
                    }
                }
            }
        }
    }

    built_benchmarks_ = true;
    return true;
}

#ifdef EXT_DELAY_INJECTED_LOADED_LATENCY_BENCHMARK
bool BenchmarkManager::runExtDelayInjectedLoadedLatencyBenchmark() {
    if (config_.getNumWorkerThreads() < 2) {
        std::cerr << "ERROR: Number of worker threads must be at least 1." << std::endl;
        return false;
    }

    std::vector<DelayInjectedLoadedLatencyBenchmark*> del_lat_benchmarks;

    //Put the enumerations into vectors to make constructing benchmarks more loopable
    std::vector<chunk_size_t> chunks;
    chunks.push_back(CHUNK_32b);
#ifdef HAS_WORD_64
    chunks.push_back(CHUNK_64b);
#endif
#ifdef HAS_WORD_128
    chunks.push_back(CHUNK_128b);
#endif
#ifdef HAS_WORD_256
    chunks.push_back(CHUNK_256b);
#endif
#ifdef HAS_WORD_512
    chunks.push_back(CHUNK_512b);
#endif

    //Build benchmarks
    for (auto mem_node_it = memory_numa_node_affinities_.cbegin(); mem_node_it != memory_numa_node_affinities_.cend(); mem_node_it++) { //iterate each memory NUMA node
        uint32_t mem_node = *mem_node_it;

        void* mem_array = mem_arrays_[mem_node];
        size_t mem_array_len = mem_array_lens_[mem_node];

        for (auto cpu_node_it = cpu_numa_node_affinities_.cbegin(); cpu_node_it != cpu_numa_node_affinities_.cend(); cpu_node_it++) { //iterate each cpuory NUMA node
            uint32_t cpu_node = *cpu_node_it;

            for (uint32_t chunk_index = 0; chunk_index < chunks.size(); chunk_index++) { //iterate different chunk sizes
                chunk_size_t chunk = chunks[chunk_index];

                uint32_t d = 0;
                while (d <= 1024) { //Iterate different delay values

                    std::string benchmark_name = static_cast<std::ostringstream*>(&(std::ostringstream() << "Test #" << g_test_index++ << "E" << EXT_NUM_DELAY_INJECTED_LOADED_LATENCY_BENCHMARK << " (Extension: Delay-Injected Loaded Latency)"))->str();

                    del_lat_benchmarks.push_back(new DelayInjectedLoadedLatencyBenchmark(mem_array,
                                                                             mem_array_len,
                                                                             config_.getIterationsPerTest(),
                                                                             config_.getNumWorkerThreads(),
                                                                             mem_node,
                                                                             cpu_node,
                                                                             chunk,
                                                                             dram_power_readers_,
                                                                             benchmark_name,
                                                                             d));
                    if (del_lat_benchmarks[del_lat_benchmarks.size()-1] == NULL) {
                        std::cerr << "ERROR: Failed to build a DelayInjectedLoadedLatencyBenchmark!" << std::endl;
                        return false;
                    }

                    if (d == 0) //special case
                        d = 1;
                    else
                        d *= 2;
                }
            }
        }
    }

    //Run benchmarks
    for (uint32_t i = 0; i < del_lat_benchmarks.size(); i++) {
        del_lat_benchmarks[i]->run();
        del_lat_benchmarks[i]->reportResults(); //to console

        //Write to results file if necessary
        if (config_.useOutputFile()) {
            results_file_ << del_lat_benchmarks[i]->getName() << ",";
            results_file_ << del_lat_benchmarks[i]->getIterations() << ",";
            results_file_ << static_cast<size_t>(del_lat_benchmarks[i]->getLen() / del_lat_benchmarks[i]->getNumThreads() / KB) << ",";
            results_file_ << del_lat_benchmarks[i]->getNumThreads() << ",";
            results_file_ << del_lat_benchmarks[i]->getNumThreads()-1 << ",";
            results_file_ << del_lat_benchmarks[i]->getMemNode() << ",";
            results_file_ << del_lat_benchmarks[i]->getCPUNode() << ",";
            if (del_lat_benchmarks[i]->getNumThreads() < 2) {
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
                results_file_ << "N/A" << ",";
            } else {
                pattern_mode_t pattern = del_lat_benchmarks[i]->getPatternMode();
                switch (pattern) {
                    case SEQUENTIAL:
                        results_file_ << "SEQUENTIAL" << ",";
                        break;
                    case RANDOM:
                        results_file_ << "RANDOM" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                rw_mode_t rw_mode = del_lat_benchmarks[i]->getRWMode();
                switch (rw_mode) {
                    case READ:
                        results_file_ << "READ" << ",";
                        break;
                    case WRITE:
                        results_file_ << "WRITE" << ",";
                        break;
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                chunk_size_t chunk_size = del_lat_benchmarks[i]->getChunkSize();
                switch (chunk_size) {
                    case CHUNK_32b:
                        results_file_ << "32" << ",";
                        break;
#ifdef HAS_WORD_64
                    case CHUNK_64b:
                        results_file_ << "64" << ",";
                        break;
#endif
#ifdef HAS_WORD_128
                    case CHUNK_128b:
                        results_file_ << "128" << ",";
                        break;
#endif
#ifdef HAS_WORD_256
                    case CHUNK_256b:
                        results_file_ << "256" << ",";
                        break;
#endif
#ifdef HAS_WORD_512
                    case CHUNK_512b:
                        results_file_ << "512" << ",";
                        break;
#endif
                    default:
                        results_file_ << "UNKNOWN" << ",";
                        break;
                }

                results_file_ << del_lat_benchmarks[i]->getStrideSize() << ",";
            }

            results_file_ << del_lat_benchmarks[i]->getMeanLoadMetric() << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "N/A" << ",";
            results_file_ << "MB/s" << ",";
            results_file_ << del_lat_benchmarks[i]->getMeanMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->getMinMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->get25PercentileMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->getMedianMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->get75PercentileMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->get95PercentileMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->get99PercentileMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->getMaxMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->getModeMetric() << ",";
            results_file_ << del_lat_benchmarks[i]->getMetricUnits() << ",";
            for (uint32_t j = 0; j < g_num_physical_packages; j++) {
                results_file_ << del_lat_benchmarks[i]->getMeanDRAMPower(j) << ",";
                results_file_ << del_lat_benchmarks[i]->getPeakDRAMPower(j) << ",";
            }
            results_file_ << del_lat_benchmarks[i]->getDelay() << ",";
            results_file_ << "<-- load threads' memory access delay value in nops" << ",";
            results_file_ << std::endl;
        }
    }

    return true;
}
#endif

#ifdef EXT_STREAM_BENCHMARK
bool BenchmarkManager::runExtStreamBenchmark() {
    //TODO: implement me
    return true;
}
#endif
