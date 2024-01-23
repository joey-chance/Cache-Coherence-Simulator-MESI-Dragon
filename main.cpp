#include <iostream>
#include <thread>
#include <cstring>
#include <string>

#include "utils/config.h"
#include "utils/processor.h"
#include "utils/bus.h"
#include "utils/global_lock.h"
#include "utils/logger.h"

int main(int argc, char* argv[]) {
    if (argc != 3 && argc != 6 && argc != 7)
    {
        std::cout << "ERROR: " << argc << " argument(s) doesn't match format." << std::endl;
        std::cout << "This simulator supports 3 syntaxes:" << std::endl;
        std::cout << "  1. Standard: ./coherence <PROTOCOL> <BENCHMARK> <CACHE_SIZE> <ASSOCIATIVITY> <BLOCK_SIZE>" << std::endl;
        std::cout << "  2. Use default cache size, associativity and block size: ./coherence <PROTOCOL> <BENCHMARK>" << std::endl;
        std::cout << "  3. Optimized MESI: ./coherence <PROTOCOL> <BENCHMARK> <CACHE_SIZE> <ASSOCIATIVITY> <BLOCK_SIZE> true" << std::endl;
        return 0;
    }

    Protocol protocol;
    Benchmark benchmark;
    int cache_size = 4096; // value in bytes
    int associativity = 2;
    int block_size = 32;   // value in bytes
    bool optimize = false;

    if (strcmp(argv[1], "MESI") == 0)
        protocol = Protocol::MESI;
    else if (strcmp(argv[1], "Dragon") == 0)
        protocol = Protocol::Dragon;
    else
    {
        std::cout << "ERROR: Unknown protocol " << argv[1] << ". Only MESI and Dragon are supported." << std::endl;
        return 0;
    }

    if (strcmp(argv[2], "blackscholes") == 0)
        benchmark = Benchmark::blackscholes;
    else if (strcmp(argv[2], "bodytrack") == 0)
        benchmark = Benchmark::bodytrack;
    else if (strcmp(argv[2], "fluidanimate") == 0)
        benchmark = Benchmark::fluidanimate;
    else 
    {
        std::cout << "ERROR: Unknown benchmark " << argv[3] << ". Only blackscholes, bodytrack and fluidanimate are supported." << std::endl;
        return 0;
    }

    if (argc == 6 || argc == 7)
    {
        cache_size = std::atoi(argv[3]);
        associativity = std::atoi(argv[4]);
        block_size = std::atoi(argv[5]);
    }

    std::string arguments(argv[1]);
    for (int i = 2; i < argc; ++i)
    {
        arguments += "_";
        arguments += argv[i];
    }
    if (argc == 3)
    {
        arguments += "_4096_2_32";
    }
    else if (argc == 7)
    {
        optimize = true;
    }
    
    GlobalLock *gl = new GlobalLock(cache_size, associativity, block_size);

    Bus *bus;
    if (protocol == Protocol::MESI)
        bus = new MESI_Bus(cache_size, associativity, block_size, optimize, gl);
    else
        bus = new Dragon_Bus(cache_size, associativity, block_size, optimize, gl);

    Processor* core0 = new Processor(0, protocol, benchmark, cache_size, associativity, block_size, bus, gl);
    Processor* core1 = new Processor(1, protocol, benchmark, cache_size, associativity, block_size, bus, gl);
    Processor* core2 = new Processor(2, protocol, benchmark, cache_size, associativity, block_size, bus, gl);
    Processor* core3 = new Processor(3, protocol, benchmark, cache_size, associativity, block_size, bus, gl);

    bus->init_cores(core0, core1, core2, core3);
    bus->init_cache(core0->get_cache(), core1->get_cache(), core2->get_cache(), core3->get_cache());

    Logger logger(core0, core1, core2, core3, arguments, block_size);

    std::thread t0(&Processor::run, core0);
    std::thread t1(&Processor::run, core1);
    std::thread t2(&Processor::run, core2);
    std::thread t3(&Processor::run, core3);

    t0.join();
    t1.join();
    t2.join();
    t3.join();

    logger.print_summary();

    // For printing analysis results
    // std::string analysis_log(argv[1]);
    // analysis_log += "_";
    // analysis_log += argv[2];
    // analysis_log += ".csv";
    // logger.print_analysis(analysis_log, cache_size, associativity);

    // For printing optimized analysis results
    // logger.print_analysis("optimized.csv", cache_size, associativity);

    return 0;
}