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
#include <string.h>
// #include <assert.h>
// #include <time.h>
// #include <util.h>

MatrixBenchmark *newMatrixBenchmark(void* mem_array, size_t mem_array_len, uint32_t iters, uint32_t num_worker_threads,
                                    uint32_t mem_node, uint32_t mem_region, uint32_t cpu_node, uint32_t cpu,
                                    bool use_cpu_nodes, pattern_mode_t pattern_mode, rw_mode_t rw_mode,
                                    chunk_size_t chunk_size, int32_t stride_size, char *benchmark_name) {

    MatrixBenchmark *mat_bench = (MatrixBenchmark *) malloc(sizeof(MatrixBenchmark));

    mat_bench->mem_array          = mem_array;
    mat_bench->len                = mem_array_len;
    mat_bench->iterations         = iters;
    mat_bench->num_worker_threads = num_worker_threads;
    mat_bench->mem_node           = mem_node;
    mat_bench->mem_region         = mem_region;
    mat_bench->cpu_node           = cpu_node;
    mat_bench->cpu                = cpu;
    mat_bench->use_cpu_nodes      = use_cpu_nodes;
    mat_bench->pattern_mode       = pattern_mode;
    mat_bench->rw_mode            = rw_mode;
    mat_bench->chunk_size         = chunk_size;
    mat_bench->stride_size        = stride_size;
    strcpy(mat_bench->metric_units, "ns/access");
    strcpy(mat_bench->name, benchmark_name);

    mat_bench->warning_ = false;
    mat_bench->metric_on_iter_             = (double *) malloc(iters * sizeof(double));
    mat_bench->enumerator_metric_on_iter_  = (double *) malloc(iters * sizeof(double));
    mat_bench->denominator_metric_on_iter_ = (double *) malloc(iters * sizeof(double));

    return mat_bench;
}

uint32_t getCPUId(MatrixBenchmark *mat_bench) {

    if (mat_bench->use_cpu_nodes)
        // TODO: fix me
        return 0; //cpu_id_in_numa_node(getCPUNode(), 0)
    else
        return mat_bench->cpu;
}

