//Headers
#include <common.h>
#include <Configurator.h>
#include <BenchmarkManager.h>

//Libraries
#include <stdlib.h>
#include <stdio.h>


/**
 *  @brief The main entry point to the program.
 */
int main(int argc, char* argv[]) {
    bool config_success = false;

    init_globals();

    //Get info about the runtime system
    if (query_sys_info()) {
        fprintf(stderr, "ERROR occurred while querying CPU information.\n");
        return -1;
    }

    //Configure runtime based on user inputs
    config_success = !configureFromInput(&config, argc, argv);

    if (config_success) {

        // if (g_verbose) {
        //     print_compile_time_options();
        //     print_types_report();
        //     report_sys_info();
        //     test_thread_affinities();
        // }

        setup_timer();
        // if (g_verbose)
        //     report_timer();

        BenchmarkManager *benchmgr = initBenchMgr(&config);
        if (latencyMatrixTestSelected(&config)) {
            runLatencyMatrixBenchmarks(benchmgr);
        }

        // if (config.throughputMatrixTestSelected()) {
        //     benchmgr.runThroughputMatrixBenchmarks();
        // }
        // }
    }

    if (config_success)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
