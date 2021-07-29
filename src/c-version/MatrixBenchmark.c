/**
 * @file
 *
 * @brief Implementation file for the MatrixBenchmark class.
 */

//Headers
#include <MatrixBenchmark.h>
#include <common.h>
// #include <benchmark_kernels.h>
// #include <MemoryWorker.h>
// #include <LatencyWorker.h>
// #include <LoadWorker.h>

//Libraries
// #include <algorithm>
// #include <assert.h>
// #include <fstream>
// #include <iostream>
// #include <map>
// #include <random>
// #include <time.h>
// #include <util.h>


uint32_t getCPUId(MatrixBenchmark *mat_bench) {

    if (mat_bench->use_cpu_nodes)
        return -1; //cpu_id_in_numa_node(getCPUNode(), 0)
    else
        return mat_bench->cpu;
}

