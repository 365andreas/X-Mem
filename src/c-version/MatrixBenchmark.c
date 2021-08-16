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
#include <stdio.h>
#include <string.h>
#include <math.h>
// #include <time.h>
#include <util.h>

MatrixBenchmark *newMatrixBenchmark(void* mem_array, size_t mem_array_len, uint32_t iters, uint32_t num_worker_threads,
                                    uint32_t mem_node, uint32_t mem_region, uint32_t cpu_node, uint32_t cpu,
                                    bool use_cpu_nodes, pattern_mode_t pattern_mode, rw_mode_t rw_mode,
                                    chunk_size_t chunk_size, int32_t stride_size, char *benchmark_name,
                                    char *metric_units) {

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
    strcpy(mat_bench->metric_units, metric_units);
    strcpy(mat_bench->name, benchmark_name);

    mat_bench->has_run_ = false;
    mat_bench->warning_ = false;
    mat_bench->metric_on_iter_             = (double *) malloc(iters * sizeof(double));
    mat_bench->enumerator_metric_on_iter_  = (double *) malloc(iters * sizeof(double));
    mat_bench->denominator_metric_on_iter_ = (double *) malloc(iters * sizeof(double));

    return mat_bench;
}

void computeMedian(MatrixBenchmark *mat_bench, uint32_t n) {

    if ((n > mat_bench->iterations) || (n <= 0)) {
        fprintf(stderr, "WARNING: Wrong arg in computeMedian().\n");
        return;
    }

    // Keep only first n elems.
    // // // metrics.resize(n);

    mat_bench->median_metric_ = compute_median(mat_bench->metric_on_iter_, n);

    if (n <= 5) {
        fprintf(stderr, "WARNING: Number of metrics given is too small to compute the CI of the median.\n");
        return;
    }

    // Build sorted array of metrics from each iteration
    double *sortedMetrics = (double *) malloc(n * sizeof(double));
    memcpy(sortedMetrics, mat_bench->metric_on_iter_, n * sizeof(double));

    qsort(sortedMetrics, n, sizeof(double), comparatorDoubleAsc);

    double sqroot = pseudosqrt(n);

    uint32_t lower_95_CI_rank = (uint32_t) ((n - Z_CI_95 * sqroot) / 2.0);
    if (lower_95_CI_rank < 1) lower_95_CI_rank = 1;
    uint32_t upper_95_CI_rank = (uint32_t) (ceil((n + 2 + Z_CI_95 * sqroot) / 2.0));
    bool exceeds_limits = upper_95_CI_rank - n > 0;
    if (exceeds_limits) upper_95_CI_rank = n;

    mat_bench->lower_95_CI_median_ = sortedMetrics[lower_95_CI_rank - 1]; //-1 since it is zero-indexed
    mat_bench->upper_95_CI_median_ = sortedMetrics[upper_95_CI_rank - 1]; //-1 since it is zero-indexed
}

double getMedianMetric(MatrixBenchmark *mat_bench) {

    if (mat_bench->has_run_)
        return mat_bench->median_metric_;
    else {
        //bad call
        fprintf(stderr, "WARNING: getMedianMetric() was called before running the benchmark.\n");
        return -1;
    }
}

uint32_t getCPUId(MatrixBenchmark *mat_bench) {

    if (mat_bench->use_cpu_nodes)
        // TODO: fix me
        return 0; //cpu_id_in_numa_node(getCPUNode(), 0)
    else
        return mat_bench->cpu;
}

char *getMetricUnits(MatrixBenchmark *mat_bench) {
    return mat_bench->metric_units;
}

void reportResults(MatrixBenchmark *mat_bench) {

    if (mat_bench->use_cpu_nodes) {
        printf("CPU NUMA Node: %d ", mat_bench->cpu_node);
    } else {
        printf("CPU: %d ", mat_bench->cpu);
    }
    printf("Memory NUMA Node: %d ", mat_bench->mem_node);
    printf("Region: %d ", mat_bench->mem_region);

    if (mat_bench->has_run_) {
        printf("Mean: %f %s ", mat_bench->mean_metric_, mat_bench->metric_units); // << " and " << mean_load_metric_ << " MB/s mean imposed load (not necessarily matched)";
        printf("Median: %f %s ", mat_bench->median_metric_, mat_bench->metric_units);
        printf("LB 95%% CI: %f %s ", mat_bench->lower_95_CI_median_, mat_bench->metric_units);
        printf("UB 95%% CI: %f %s ", mat_bench->upper_95_CI_median_, mat_bench->metric_units);
        printf("iterations: %d", mat_bench->iterations);
        if (mat_bench->warning_)
            printf(" (WARNING)");
        printf("\n");
    }
    else
        fprintf(stderr, "WARNING: Benchmark has not run yet. No reported results.\n");
}

void computeMetrics(MatrixBenchmark *bench) {
    if (bench->has_run_) {
        //Compute mean
        bench->mean_metric_ = 0;
        for (uint32_t i = 0; i < bench->iterations; i++)
            bench->mean_metric_ += bench->metric_on_iter_[i];
        bench->mean_metric_ /= bench->iterations;

        //Build sorted array of metrics from each iteration
        // std::vector<double> sortedMetrics = metric_on_iter_;
        // std::sort(sortedMetrics.begin(), sortedMetrics.end());

        // //Compute percentiles
        // min_metric_ = sortedMetrics.front();
        // percentile_25_metric_ = sortedMetrics[sortedMetrics.size()/4];
        // percentile_75_metric_ = sortedMetrics[sortedMetrics.size()*3/4];
        bench->median_metric_ = compute_median(bench->metric_on_iter_, bench->iterations);
        // percentile_95_metric_ = sortedMetrics[sortedMetrics.size()*95/100];
        // percentile_99_metric_ = sortedMetrics[sortedMetrics.size()*99/100];
        // max_metric_ = sortedMetrics.back();

        // //Compute mode
        // std::map<double,uint32_t> metricCounts;
        // for (uint32_t i = 0; i < iterations_; i++)
        //     metricCounts[metric_on_iter_[i]]++;
        // mode_metric_ = 0;
        // uint32_t greatest_count = 0;
        // for (auto it = metricCounts.cbegin(); it != metricCounts.cend(); it++) {
        //     if (it->second > greatest_count)
        //         mode_metric_ = it->first;
        // }
    }
}
