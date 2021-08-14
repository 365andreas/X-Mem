//Headers
#include <common.h>
#include <Configurator.h>
#include <BenchmarkManager.h>
#include <RegisterRegions.h>

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

        if (registerRegionsSelected(&config) && connectBeforeRun(&config)) {
            fprintf(stderr, "ERROR: Wrong configuration. Registering regions (-R option) must not be selected"
                            " along with connecting to a remote node (-X option)\n");
        }

        // if (latencyMatrixTestSelected(&config) || throughputMatrixTestSelected(&config)) {
        if (latencyMatrixTestSelected(&config)) {

            setup_timer();
            // if (g_verbose)
            //     report_timer();

            RemoteRegion *rr = NULL;
            if (connectBeforeRun(&config)) {
                rr = newRemoteRegion(&config);
                connectToPeer(rr);
                void *phys_addr = getPhysicalAddress(rr->vaddr);
                if (phys_addr == NULL) {
                    fprintf(stderr, "ERROR: Could not retrieve physical address of 0x%.16llx",
                            (long long unsigned) rr->vaddr)
                    return -1;
                }
                config.mem_regions_phys_addr_[0] = (uint64_t) phys_addr;
            }

            BenchmarkManager *benchmgr = initBenchMgr(&config);

            // if (latencyMatrixTestSelected(&config))
                runLatencyMatrixBenchmarks(benchmgr);

            // if (throughputMatrixTestSelected((&config))
            //     benchmgr.runThroughputMatrixBenchmarks();

            if (connectBeforeRun(&config)) {
                if (rr == NULL) {
                    fprintf(stderr, "ERROR: RemoteRegister is not set (== NULL).\n");
                    return -1;
                }
                sendFinishedMsg(rr);
            }
        }

        if (registerRegionsSelected(&config)) {
            registerRegions(&config);
        }

    }

    if (config_success)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}
