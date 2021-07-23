//Headers
#include <common.h>
#include <Configurator.h>
// #include <BenchmarkManager.h>

//Libraries
#include <stdlib.h>
#include <stdio.h>


/**
 *  @brief The main entry point to the program.
 */
int main(int argc, char* argv[]) {
    bool config_success = false;

    init_globals();

    // //Get info about the runtime system
    // if (query_sys_info()) {
    //     std::cerr << "ERROR occurred while querying CPU information." << std::endl;
    //     return -1;
    // }

    //Configure runtime based on user inputs
    config_success = !configureFromInput(&config, argc, argv);

    if (config_success) {

        // if (g_verbose) {
        //     print_compile_time_options();
        //     print_types_report();
        //     report_sys_info();
        //     test_thread_affinities();
        // }

        // setup_timer();
        // if (g_verbose)
        //     report_timer();

        // BenchmarkManager benchmgr(config);
        // if (config.latencyMatrixTestSelected()) {
            // printf("run_latency_matrix: %s\n", latencyMatrixTestSelected(&config) ? "YES" : "NO");
        //     benchmgr.runLatencyMatrixBenchmarks();
        // }

        // if (config.throughputMatrixTestSelected()) {
            // printf("run_throughput_matrix: %s\n", latencyThroughputTestSelected(&config) ? "YES" : "NO");
        //     benchmgr.runThroughputMatrixBenchmarks();
        // }
        // }
    }

    if (config_success)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
