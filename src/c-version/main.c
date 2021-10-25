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

        printf("\n");

        bool locked = lock_thread_to_cpu(config.core_id_);
        if (! locked)
            fprintf(stderr, "WARNING: Failed to lock thread to logical CPU %d! Results may not be correct.\n", config.core_id_);
        else if (g_verbose)
            fprintf(stderr, "Locked main thread to logical CPU %d.\n", config.core_id_);

        setup_timer();

        BenchmarkManager *benchmgr = initBenchMgr(&config);

        if (latencyMatrixTestSelected(&config)) {
            runLatencyMatrixBenchmarks(benchmgr);
        }

        if (throughputMatrixTestSelected(&config)) {
            runThroughputMatrixBenchmarks(benchmgr);
        }
    }

    if (config_success)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
