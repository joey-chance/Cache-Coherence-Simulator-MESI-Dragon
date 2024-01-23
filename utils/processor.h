#ifndef _PROCESSOR_H
#define _PROCESSOR_H

#include <atomic>
#include <fstream>
#include <string>
#include <iostream>

#include "config.h"
#include "lru_cache.h"

class Processor {
private:
    std::ifstream benchmark_file;
    int N;
    int M;
    int pid;
    LRUCache* cache;
    Bus* bus;
    GlobalLock* gl;

    // Statistics
    long total_cycle = 0;
    long compute_cycle = 0;
    long count_mem_instr = 0;

public:
    std::atomic<long> idle_cycle = 0;

    Processor(int _pid, Protocol _protocol, Benchmark _benchmark, int _cache_size, int _associativity, int _block_size, Bus* _bus, GlobalLock* _gl)
    : pid(_pid)
    , bus(_bus)
    , gl(_gl)
    , M((_cache_size/_block_size) / _associativity)
    , N(_block_size)
    {
        if (_protocol == Protocol::MESI)
        {
            cache = new MESI_Cache(_cache_size, _associativity, _block_size, _pid, _bus, _gl);
        }
        else
        {
            cache = new Dragon_Cache(_cache_size, _associativity, _block_size, _pid, _bus, _gl);
        }

        std::string path;
        if (_benchmark == Benchmark::blackscholes)
        {
            path = "blackscholes_four/blackscholes_";
        }
        else if (_benchmark == Benchmark::bodytrack)
        {
            path = "bodytrack_four/bodytrack_";
        }
        else
        {
            path = "fluidanimate_four/fluidanimate_";
        }
        path += std::to_string(pid) + ".data";
        benchmark_file.open(path, std::ifstream::in);

        if (_cache_size % _block_size != 0)
        {
            std::cout << "ERROR: Cache size must be divisible by block size." << std::endl;
        }
        else if ((_cache_size/_block_size) % _associativity != 0)
        {
            std::cout << "ERROR: Total number of cache block must be divisible by associativity." << std::endl;
        }
    }
    LRUCache* get_cache();
    void run();
    long get_total_cycle();
    int get_compute_cycle();
    int get_count_mem_instr();
    int get_idle_cycle();
    int get_count_cache_miss();
    int get_count_data_traffic();
    int get_count_update();
    int get_count_private_access();
    int get_count_shared_access();
};

#endif // _PROCESSOR_H