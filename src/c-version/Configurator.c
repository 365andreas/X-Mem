/**
 * @file
 *
 * @brief Implementation file for the Configurator struct and some helper data structures.
 */

//Headers
#include <Configurator.h>
#include <common.h>

//Libraries
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void printHelpText() {

    char *msg = "\n"
                " -a, run benchmarks on all cores\n"
                " -c, number of cores to be used for the throughput matrix benchmarks\n"
                "     if -1 is given, all cores will be used for the throughput matrix benchmarks\n"
                "     if omitted, 1 core will be used for the throughput matrix benchmarks\n"
                " -d filename, writes the results in a format compatible with the decoding nets\n"
                " -l, run latency matrix benchmarks\n"
                " -t, run throughput matrix benchmarks\n"
                " -n iters, pass number of iterations\n"
                " -r addr, pass physical address to benchmark\n"
                " -s, use synchronous operations (O_SYNC enabled)\n"
                " -v, verbose mode\n"
                " -w working set size, pass working set size per thread in KB\n"
                " -x, extended benchmarking mode (running all the iterations)\n"
                " -C, define the core that will be used for the benchmarks (zero-indexed)\n"
                "     if -a is also specified then this option is overided and not used.\n"
                " -N, for latency benchmarks, random pointers are not written to the region inspected, existing\n"
                "     offsets written in region will be used instead\n"
                " -W, writes instead of reads are used for the trhoughput benchmarks\n";
    printf("%s", msg);
}

void printUsageText(char *name){

    fprintf(stderr, "Usage: %s [-a] [-lt] [-dsvx] [-NW] -r region_address [-n iterations_num] [-c num_cores] [-C core_id]\n", name);
}

int32_t configureFromInput(Configurator *conf, int argc, char* argv[]) {

    int opt, regions;

    if (conf->configured_) { //If this object was already configured, cannot override from user inputs. This is to prevent an invalid state.
        fprintf(stderr, "WARNING: Something bad happened when configuring X-Mem. This is probably not your fault.\n");
        return EXIT_FAILURE;
    }

    // default configuration
    conf->iterations_ = 10;
    conf->working_set_size_per_thread_ = DEFAULT_WORKING_SET_SIZE_PER_THREAD;
    conf->sync_mem_ = false;
    conf->use_num_cores_ = 1;
    conf->assume_existing_pointers_ = false;

    for (size_t i = 1; i < argc; i++) {
        if (strstr(argv[i], "-r")) {
            conf->mem_regions_in_phys_addr_ = true;
            conf->mem_regions_phys_addr_size++;
        }
    }
    if (conf->mem_regions_in_phys_addr_) {
        conf->mem_regions_phys_addr_ = (uint64_t *) malloc(conf->mem_regions_phys_addr_size * sizeof(uint64_t));
    }

    regions = 0;
    while ((opt = getopt(argc, argv, "ac:d:lvtn:r:sw:xC:NW")) != -1) {
        switch (opt) {
            case 'a': conf->run_all_cores_ = true; break;
            case 'c':
                if (strtoul(optarg, NULL, 0) == (unsigned long) -1) {
                    conf->use_num_cores_ = g_num_logical_cpus;
                } else {
                    conf->use_num_cores_ = (uint32_t) strtoul(optarg, NULL, 0);
                }
                if ((conf->use_num_cores_ == 0) || (conf->use_num_cores_ > g_num_logical_cpus)) {
                    fprintf(stderr, "ERROR: Specified number of cores is not valid.\n");
                    printUsageText(argv[0]);
                    printHelpText();
                    return EXIT_FAILURE;
                }
                break;
            case 'd':
                conf->use_dec_net_file_ = true;
                conf->dec_net_filename_ = optarg;
                break;
            case 'l': conf->run_latency_matrix_ = true; break;
            case 'n': conf->iterations_         = (uint32_t) strtoul(optarg, NULL, 0); break;
            case 'r':
                // memory regions specified by physical addresses
                conf->mem_regions_phys_addr_[regions++]  = strtoull(optarg, NULL, 0); break;
            case 's': conf->sync_mem_                    = true; break;
            case 't': conf->run_throughput_matrix_       = true; break;
            case 'v': conf->verbose_                     = true; break;
            case 'w': conf->working_set_size_per_thread_ = strtoull(optarg, NULL, 0) * KB; break;
            case 'x': g_log_extended                     = true; break;
            case 'C':
                conf->core_id_ = (uint32_t) strtoul(optarg, NULL, 0);
                if (conf->core_id_ >= g_num_logical_cpus) {
                    fprintf(stderr, "ERROR: Specified number of core id is not valid. Remember that core ids are"
                                    "zero-indexed.\n");
                    printUsageText(argv[0]);
                    printHelpText();
                    return EXIT_FAILURE;
                }
                break;
            case 'N': conf->assume_existing_pointers_ = true; break;
            case 'W': conf->use_writes_               = true; break;
            default:
                printUsageText(argv[0]);
                printHelpText();
                return EXIT_FAILURE;
        }
    }


    // memory regions specified by physical addresses
    if (! conf->mem_regions_in_phys_addr_) {
        printUsageText(argv[0]);
        printHelpText();
        fprintf(stderr, "ERROR: A region_address was not specified\n");
        return EXIT_FAILURE;
    }

    conf->configured_ = true;

    if (conf->verbose_)
        g_verbose = true;

    g_test_index = 0;

    return EXIT_SUCCESS;
}

bool runForAllCoresSelected(Configurator *conf) { return conf->run_all_cores_; }

uint32_t getCoreId(Configurator *conf) { return conf->core_id_; }

uint32_t useNumCores(Configurator *conf) { return conf->use_num_cores_; }

bool latencyMatrixTestSelected(Configurator *conf) { return conf->run_latency_matrix_; }

bool throughputMatrixTestSelected(Configurator *conf) { return conf->run_throughput_matrix_; }

bool syncMemory(Configurator *conf) { return conf->sync_mem_; }

bool memoryRegionsInPhysAddr(Configurator *conf) { return conf->mem_regions_in_phys_addr_; }

uint32_t numberOfMemoryRegionsPhysAddresses(Configurator *conf) { return conf->mem_regions_phys_addr_size; }

uint64_t *getMemoryRegionsPhysAddresses(Configurator *conf) { return conf->mem_regions_phys_addr_; }

size_t getWorkingSetSizePerThread(Configurator *conf) { return conf->working_set_size_per_thread_; }

uint32_t getIterationsPerTest(Configurator *conf) { return conf->iterations_; }

char *getDecNetFilename(Configurator *conf) { return conf->dec_net_filename_; }

bool useDecNetFile(Configurator *conf) { return conf->use_dec_net_file_; }

void setUseDecNetFile(Configurator *conf, bool use) { conf->use_dec_net_file_ = use; }

bool useWrites(Configurator *conf) { return conf->use_writes_; }
