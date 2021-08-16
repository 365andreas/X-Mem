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
#include <unistd.h>


void printHelpText() {

    char *msg = "\n"
                " -a, run benchmarks on all cores\n"
                " -l, run latency matrix benchmarks\n"
                // " -t, run throughput matrix benchmarks\n"
                " -v, verbose mode\n"
                " -r addr, pass physical address to benchmark\n"
                " -R, (XEON PHI systems specific) declare that this node (host or PHI) will only register an address region for remote access\n"
                " -X, (XEON PHI systems specific) declare that this node (host or PHI) will have to connectToPeer to a node that has registered its memory\n"
                " -N, (XEON PHI systems specific) define the remote node (host or PHI) which will have access to the registered memory"
                " for remote access before running the benchmarks\n";
    printf("%s", msg);
}

void printUsageText(char *name){

    fprintf(stderr, "Usage: %s [-alvX] -r region_address [-R] [-N remote peer node id]\n", name);
}

int32_t configureFromInput(Configurator *conf, int argc, char* argv[]) {

    if (conf->configured_) { //If this object was already configured, cannot override from user inputs. This is to prevent an invalid state.
        fprintf(stderr, "WARNING: Something bad happened when configuring X-Mem. This is probably not your fault.\n");
        return EXIT_FAILURE;
    }

    bool isCaseInsensitive = false;
    int opt;
    uint64_t addr;

    while ((opt = getopt(argc, argv, "alvr:RXN:")) != -1) {
        switch (opt) {
            case 'v': conf->verbose_            = true; break;
            case 'a': conf->run_all_cores_      = true; break;
            case 'l': conf->run_latency_matrix_ = true; break;
            // case 't': conf->run_throughput_matrix_ = true; break;
            case 'r':
                // memory regions specified by physical addresses
                conf->mem_regions_in_phys_addr_ = true;
                conf->mem_regions_phys_addr_size = 1;
                conf->mem_regions_phys_addr_ = (uint64_t *) malloc(conf->mem_regions_phys_addr_size * sizeof(uint64_t));
                addr = strtoull(optarg, NULL, 0);
                conf->mem_regions_phys_addr_[0] = addr;
                break;
            case 'R': conf->register_regions_ = true; break;
            case 'X': conf->connect_before_run_ = true; break;
            case 'N': conf->remote_peer_ = (uint16_t) strtoul(optarg, NULL, 0); break;
            default:
                printUsageText(argv[0]);
                printHelpText();
                return EXIT_FAILURE;
        }
    }

    conf->working_set_size_per_thread_ = DEFAULT_WORKING_SET_SIZE_PER_THREAD;

    // memory regions specified by physical addresses
    if ((! conf->mem_regions_in_phys_addr_) && (! conf->register_regions_)) {
        printUsageText(argv[0]);
        printHelpText();
        fprintf(stderr, "ERROR: A region_address was not specified\n");
        return EXIT_FAILURE;
    }

    conf->iterations_ = 10;

    conf->configured_ = true;

    if (conf->verbose_)
        g_verbose = true;

    g_test_index = 0;

    return EXIT_SUCCESS;
}

bool allCoresSelected(Configurator *conf) { return conf->run_all_cores_; }

bool latencyMatrixTestSelected(Configurator *conf) { return conf->run_latency_matrix_; }

bool throughputMatrixTestSelected(Configurator *conf) { return conf->run_throughput_matrix_; }

bool registerRegionsSelected(Configurator *conf) { return conf->register_regions_; }

bool connectBeforeRun(Configurator *conf) { return conf->connect_before_run_; }

bool memoryRegionsInPhysAddr(Configurator *conf) { return conf->mem_regions_in_phys_addr_; }

uint32_t numberOfMemoryRegionsPhysAddresses(Configurator *conf) { return conf->mem_regions_phys_addr_size; }

uint64_t *getMemoryRegionsPhysAddresses(Configurator *conf) { return conf->mem_regions_phys_addr_; }

size_t getWorkingSetSizePerThread(Configurator *conf) { return conf->working_set_size_per_thread_; }

uint32_t getIterationsPerTest(Configurator *conf) { return conf->iterations_; }

//     //Throw out first argument which is usually the program name.
//     argc -= (argc > 0);
//     argv += (argc > 0);

//     //Set up optionparser
//     Stats stats(usage, argc, argv);
//     Option* options = new Option[stats.options_max];
//     Option* buffer = new Option[stats.buffer_max];
//     Parser parse(usage, argc, argv, options, buffer); //Parse input

//     //Check for parser error
//     if (parse.error()) {
//         std::cerr << "ERROR: Argument parsing failed. You could try running \"xmem --help\" for usage information." << std::endl;
//         goto error;
//     }

//     // X-Mem doesn't have any non-option arguments, so we will presume the user wants a help message.
//     if (parse.nonOptionsCount() > 0) {
//         std::cerr << "ERROR: X-Mem does not support any non-option arguments." << std::endl;
//         goto error;
//     }


//     //Verbosity
//     if (options[VERBOSE]) {
//         verbose_ = true; //What the user configuration is.
//         g_verbose = true; //What rest of X-Mem actually uses.
//     }

//     //Enable logging of extended benchmarks
//     if (options[LOG_EXTENDED]) {
//         g_log_extended = true;
//     }

//     //Check runtime modes
//     if (options[MEAS_LATENCY] || options[MEAS_THROUGHPUT] || options[EXTENSION] || options[MEAS_LATENCY_MATRIX] ||
//         options[MEAS_THROUGHPUT_MATRIX]) { //User explicitly picked at least one mode, so override default selection
//         run_latency_           = false;
//         run_throughput_        = false;
//         run_extensions_        = false;
//         run_latency_matrix_    = false;
//         run_throughput_matrix_ = false;
//     }

//     if (options[MEAS_LATENCY])
//         run_latency_ = true;

//     if (options[MEAS_LATENCY_MATRIX])
//         run_latency_matrix_ = true;

//     if (options[MEAS_THROUGHPUT])
//         run_throughput_ = true;

//     if (options[MEAS_THROUGHPUT_MATRIX])
//         run_throughput_matrix_ = true;

//     if (options[SYNC_MEM])
//         sync_mem_ = true;


//     //Check working set size
//     if (options[WORKING_SET_SIZE_PER_THREAD]) { //Override default value with user-specified value
//         if (!check_single_option_occurrence(&options[WORKING_SET_SIZE_PER_THREAD]))
//             goto error;

//         char* endptr = NULL;
//         size_t working_set_size_KB = strtoul(options[WORKING_SET_SIZE_PER_THREAD].arg, &endptr, 10);
//         if ((working_set_size_KB % 4) != 0) {
//             std::cerr << "ERROR: Working set size must be specified in KB and be a multiple of 4 KB." << std::endl;
//             goto error;
//         }

//         working_set_size_per_thread_ = working_set_size_KB * KB; //convert to bytes
//     }

//     //Check NUMA selection
// #ifndef HAS_NUMA
//     numa_enabled_ = false;
//     cpu_numa_node_affinities_.push_back(0);
//     memory_numa_node_affinities_.push_back(0);
// #endif

//     if (options[NUMA_DISABLE]) {
// #ifndef HAS_NUMA
//         std::cerr << "WARNING: NUMA is not supported on this build, so the NUMA-disable option has no effect." << std::endl;
// #else
//         numa_enabled_ = false;
//         cpu_numa_node_affinities_.push_back(0);
//         memory_numa_node_affinities_.push_back(0);
// #endif
//     }

//     if (options[CPU_NUMA_NODE_AFFINITY]) {
//         if (!numa_enabled_)
//             std::cerr << "WARNING: NUMA is disabled, so you cannot specify CPU NUMA node affinity directly. Overriding to only use node 0 for CPU affinity." << std::endl;
//         else {
//             Option* curr = options[CPU_NUMA_NODE_AFFINITY];
//             while (curr) { //CPU_NUMA_NODE_AFFINITY may occur more than once, this is perfectly OK.
//                 char* endptr = NULL;
//                 uint32_t cpu_numa_node_affinity = static_cast<uint32_t>(strtoul(curr->arg, &endptr, 10));
//                 if (cpu_numa_node_affinity >= g_num_numa_nodes) {
//                     std::cerr << "ERROR: CPU NUMA node affinity of " << cpu_numa_node_affinity << " is not supported. There are only " << g_num_numa_nodes << " nodes in this system." << std::endl;
//                     goto error;
//                 }

//                 bool found = false;
//                 for (auto it = cpu_numa_node_affinities_.cbegin(); it != cpu_numa_node_affinities_.cend(); it++) {
//                     if (*it == cpu_numa_node_affinity)
//                         found = true;
//                 }

//                 if (!found)
//                     cpu_numa_node_affinities_.push_back(cpu_numa_node_affinity);

//                 curr = curr->next();
//             }

//             cpu_numa_node_affinities_.sort();
//         }
//     }
//     else if (numa_enabled_) { //Default: use all CPU NUMA nodes
//         for (uint32_t i = 0; i < g_num_numa_nodes; i++)
//             cpu_numa_node_affinities_.push_back(i);
//     }

//     if (options[MEMORY_NUMA_NODE_AFFINITY]) {
//         if (!numa_enabled_)
//             std::cerr << "WARNING: NUMA is disabled, so you cannot specify memory NUMA node affinity directly. Overriding to only use node 0 for memory affinity." << std::endl;
//         else {
//             Option* curr = options[MEMORY_NUMA_NODE_AFFINITY];
//             while (curr) { //MEMORY_NUMA_NODE_AFFINITY may occur more than once, this is perfectly OK.
//                 char* endptr = NULL;
//                 uint32_t memory_numa_node_affinity = static_cast<uint32_t>(strtoul(curr->arg, &endptr, 10));
//                 if (memory_numa_node_affinity >= g_num_numa_nodes) {
//                     std::cerr << "ERROR: memory NUMA node affinity of " << memory_numa_node_affinity << " is not supported. There are only " << g_num_numa_nodes << " nodes in this system." << std::endl;
//                     goto error;
//                 }

//                 bool found = false;
//                 for (auto it = memory_numa_node_affinities_.cbegin(); it != memory_numa_node_affinities_.cend(); it++) {
//                     if (*it == memory_numa_node_affinity)
//                         found = true;
//                 }

//                 if (!found)
//                     memory_numa_node_affinities_.push_back(memory_numa_node_affinity);

//                 curr = curr->next();
//             }

//             memory_numa_node_affinities_.sort();
//         }
//     }
//     else if (numa_enabled_) { //Default: use all memory NUMA nodes
//         for (uint32_t i = 0; i < g_num_numa_nodes; i++)
//             memory_numa_node_affinities_.push_back(i);
//     }

//     //Check if large pages should be used for allocation of memory under test.
//     if (options[USE_LARGE_PAGES]) {
// #if defined(__gnu_linux__) && defined(ARCH_INTEL)
//         if (numa_enabled_) { //For now, large pages are not --simultaneously-- supported alongside NUMA. This is due to lack of NUMA support in hugetlbfs on GNU/Linux.
//             std::cerr << "ERROR: On GNU/Linux version of X-Mem for Intel architectures, large pages are not simultaneously supported alongside NUMA due to reasons outside our control. If you want large pages, then force UMA using the \"-u\" option explicitly." << std::endl;
//             goto error;
//         }
// #endif
// #ifndef HAS_LARGE_PAGES
//         std::cerr << "WARNING: Huge pages are not supported on this build. Regular-sized pages will be used." << std::endl;
// #else
//         use_large_pages_ = true;
// #endif
//     }

//     //Check number of worker threads
//     if (options[NUM_WORKER_THREADS]) { //Override default value
//         if (!check_single_option_occurrence(&options[NUM_WORKER_THREADS]))
//             goto error;

//         char* endptr = NULL;
//         num_worker_threads_ = static_cast<uint32_t>(strtoul(options[NUM_WORKER_THREADS].arg, &endptr, 10));
//         if (num_worker_threads_ > g_num_logical_cpus) {
//             std::cerr << "ERROR: Number of worker threads may not exceed the number of logical CPUs (" << g_num_logical_cpus << ")" << std::endl;
//             goto error;
//         }
//     }

//     //Check iterations
//     if (options[ITERATIONS]) { //Override default value
//         if (!check_single_option_occurrence(&options[ITERATIONS]))
//             goto error;

//         char *endptr = NULL;
//         iterations_ = static_cast<uint32_t>(strtoul(options[ITERATIONS].arg, &endptr, 10));
//     }

//     //Check throughput/loaded latency benchmark access patterns
//     if (options[RANDOM_ACCESS_PATTERN] || options[SEQUENTIAL_ACCESS_PATTERN]) { //override defaults
//         use_random_access_pattern_ = false;
//         use_sequential_access_pattern_ = false;
//     }

//     if (options[RANDOM_ACCESS_PATTERN])
//         use_random_access_pattern_ = true;

//     if (options[SEQUENTIAL_ACCESS_PATTERN])
//         use_sequential_access_pattern_ = true;

//     //Check starting test index
//     if (options[BASE_TEST_INDEX]) { //override defaults
//         if (!check_single_option_occurrence(&options[BASE_TEST_INDEX]))
//             goto error;

//         char *endptr = NULL;
//         starting_test_index_ = static_cast<uint32_t>(strtoul(options[BASE_TEST_INDEX].arg, &endptr, 10)); //What the user specified
//     }
//     g_starting_test_index = starting_test_index_; //What rest of X-Mem uses
//     g_test_index = g_starting_test_index; //What rest of X-Mem uses. The current test index.

//     //Check filename
//     if (options[OUTPUT_FILE]) { //override defaults
//         if (!check_single_option_occurrence(&options[OUTPUT_FILE]))
//             goto error;

//         filename_ = options[OUTPUT_FILE].arg;
//         use_output_file_ = true;
//     }

//     //Check if reads and/or writes should be used in throughput and loaded latency benchmarks
//     if (options[USE_READS] || options[USE_WRITES]) { //override defaults
//         use_reads_ = false;
//         use_writes_ = false;
//     }

//     if (options[USE_READS])
//         use_reads_ = true;

//     if (options[USE_WRITES])
//         use_writes_ = true;

//     if (options[DEC_NET_FILE]) { //override defaults
//         if (!check_single_option_occurrence(&options[DEC_NET_FILE]))
//             goto error;

//         dec_net_filename_ = options[DEC_NET_FILE].arg;
//         use_dec_net_file_ = true;
//     }


//     //Make sure at least one mode is available
//     if (!run_latency_ && !run_throughput_ && !run_extensions_ && !run_latency_matrix_ && !run_throughput_matrix_) {
//         std::cerr << "ERROR: At least one benchmark type must be selected." << std::endl;
//         goto error;
//     }

//     //Make sure at least one access pattern is selectee
//     if (!use_random_access_pattern_ && !use_sequential_access_pattern_) {
//         std::cerr << "ERROR: No access pattern was specified!" << std::endl;
//         goto error;
//     }

//     //Make sure at least one read/write pattern is selected
//     if (!use_reads_ && !use_writes_) {
//         std::cerr << "ERROR: Throughput benchmark was selected, but no read/write pattern was specified!" << std::endl;
//         goto error;
//     }

//     //If the user picked "all" option, override anything else they put in that is relevant.
//     if (options[ALL]) {
//         run_latency_ = true;
//         run_throughput_ = true;
//         run_extensions_ = true;
//         run_latency_matrix_ = true;
//         run_throughput_matrix_ = true;
//         use_random_access_pattern_ = true;
//         use_sequential_access_pattern_ = true;
//         use_reads_ = true;
//         use_writes_ = true;
//     }

// #ifdef HAS_WORD_64
//     //Notify that 32-bit chunks are not used on random throughput benchmarks on 64-bit machines
//     if (use_random_access_pattern_ && use_chunk_32b_)
//         std::cerr << "NOTE: Random-access load kernels used in throughput and loaded latency benchmarks do not support 32-bit chunk sizes on 64-bit machines. These particular combinations will be omitted." << std::endl;
// #endif

//     if (options[MEM_REGIONS] && options[MEM_REGIONS_PHYS]) {
//         std::cerr << "ERROR: Both number of virtual memory regions and physical addresses of regions were set" << std::endl;
//         goto error;
//     }

//     //Check memory regions
//     if (options[MEM_REGIONS]) { //Override default value
//         if (!check_single_option_occurrence(&options[MEM_REGIONS]))
//             goto error;

//         char *endptr = NULL;
//         mem_regions_ = static_cast<uint32_t>(strtoul(options[MEM_REGIONS].arg, &endptr, 10));
//     }

//     if (options[MEM_REGIONS_PHYS]) {
//         if (run_latency_ || run_throughput_) {
//             std::cerr << "ERROR: Passing physical addresses of regions along with non matrix benchmarks is not "
//                       << "supported" << std::endl;
//             goto error;
//         }

//         mem_regions_in_phys_addr_ = true;

//         std::string addr_string;
//         std::stringstream ss(options[MEM_REGIONS_PHYS].arg);

//         mem_regions_ = 0;
//         while(getline(ss, addr_string, ',')) {
//             uint64_t addr = static_cast<uint64_t>(strtoull(addr_string.c_str(), NULL, 16));
//             mem_regions_phys_addr_.push_back(addr);
//             mem_regions_++;
//         }
//     }

//     if (options[ALL_CORES]) { //Override default value
//         run_all_cores_ = true;
//     }

//     //Check for help or bad options
//     if (options[HELP] || options[UNKNOWN] != NULL)
//         goto errorWithUsage;

//     //Report final runtime configuration based on user inputs
//     std::cout << std::endl;
//     if (verbose_) {
//         std::cout << "Verbose output enabled!" << std::endl;

//         std::cout << "Benchmarking modes:" << std::endl;
//         if (run_throughput_)
//             std::cout << "---> Throughput" << std::endl;
//         if (run_latency_) {
//             std::cout << "---> ";
//             if (num_worker_threads_ > 1)
//                 std::cout << "Loaded ";
//             else
//                 std::cout << "Unloaded ";
//             std::cout << "latency" << std::endl;
//         }
//         if (run_latency_matrix_) {
//             std::cout << "---> ";
//             if (num_worker_threads_ > 1)
//                 std::cout << "Loaded ";
//             else
//                 std::cout << "Unloaded ";
//             std::cout << "latency matrix" << std::endl;
//         }
//         if (run_throughput_matrix_) {
//             std::cout << "---> ";
//             if (num_worker_threads_ > 1)
//                 std::cout << "Loaded ";
//             else
//                 std::cout << "Unloaded ";
//             std::cout << "throughput matrix" << std::endl;
//         }
//         if (run_extensions_)
//             std::cout << "---> Extensions" << std::endl;
//         std::cout << std::endl;

//         std::cout << "Benchmark settings:" << std::endl;
//         std::cout << "---> Random access:                   ";
//         if (use_random_access_pattern_)
//             std::cout << "yes";
//         else
//             std::cout << "no";
//         std::cout << std::endl;
//         std::cout << "---> Sequential access:               ";
//         if (use_sequential_access_pattern_)
//             std::cout << "yes";
//         else
//             std::cout << "no";
//         std::cout << std::endl;
//         std::cout << "---> Use memory reads:                ";
//         if (use_reads_)
//             std::cout << "yes";
//         else
//             std::cout << "no";
//         std::cout << std::endl;
//         std::cout << "---> Use memory writes:               ";
//         if (use_writes_)
//             std::cout << "yes";
//         else
//             std::cout << "no";
//         std::cout << std::endl;
//         std::cout << std::endl;
//         std::cout << "---> Number of worker threads:        ";
//         std::cout << num_worker_threads_ << std::endl;
//         std::cout << "---> NUMA enabled:                    ";
// #ifdef HAS_NUMA
//         if (numa_enabled_)
//             std::cout << "yes" << std::endl;
//         else
//             std::cout << "no" << std::endl;
//         std::cout << "------> CPU NUMA node affinities:     ";
//         for (auto it = cpu_numa_node_affinities_.cbegin(); it != cpu_numa_node_affinities_.cend(); it++)
//             std::cout << *it << " ";
//         std::cout << std::endl;
//         std::cout << "------> Memory NUMA node affinities:  ";
//         for (auto it = memory_numa_node_affinities_.cbegin(); it != memory_numa_node_affinities_.cend(); it++)
//             std::cout << *it << " ";
//         std::cout << std::endl;
// #else
//         std::cout << "not supported" << std::endl;
// #endif
//         std::cout << "---> Large pages:                     ";
// #ifdef HAS_LARGE_PAGES
//         if (use_large_pages_)
//             std::cout << "yes" << std::endl;
//         else
//             std::cout << "no" << std::endl;
// #else
//         std::cout << "not supported" << std::endl;
// #endif
//         std::cout << "---> Iterations:                      ";
//         std::cout << iterations_ << std::endl;
//         std::cout << "---> Starting test index:             ";
//         std::cout << starting_test_index_ << std::endl;
//         std::cout << std::endl;
//     }

//         std::cout << "Working set per thread:               ";
//     if (use_large_pages_) {
//         size_t num_large_pages = 0;
//         if (working_set_size_per_thread_ <= g_large_page_size) //sub one large page, round up to one
//             num_large_pages = 1;
//         else if (working_set_size_per_thread_ % g_large_page_size == 0) //multiple of large page
//             num_large_pages = working_set_size_per_thread_ / g_large_page_size;
//         else //larger than one large page but not a multiple of large page
//             num_large_pages = working_set_size_per_thread_ / g_large_page_size + 1;
//         std::cout << working_set_size_per_thread_ << " B == " << working_set_size_per_thread_ / KB  << " KB == " << working_set_size_per_thread_ / MB << " MB (fits in " << num_large_pages << " large pages)" << std::endl;
//     } else {
//         std::cout << working_set_size_per_thread_ << " B == " << working_set_size_per_thread_ / KB  << " KB == " << working_set_size_per_thread_ / MB << " MB (" << working_set_size_per_thread_/(g_page_size) << " pages)" << std::endl;
//     }
//     std::cout << std::endl;

//     //Free up options memory
//     delete[] options;
//     delete[] buffer;

//     return 0;

//     errorWithUsage:
//         printUsage(std::cerr, usage); //Display help message

//     error:

//         //Free up options memory
//         delete[] options;
//         delete[] buffer;

//         return -1;
// }

// bool Configurator::check_single_option_occurrence(Option* opt) const {
//     if (opt->count() > 1) {
//         std::cerr << "ERROR: " << opt->name << " option can only be specified once." << std::endl;
//         return false;
//     }
//     return true;
// }
