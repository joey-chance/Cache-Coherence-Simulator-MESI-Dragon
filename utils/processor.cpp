#include "processor.h"

#include <sstream>
#include <string>

LRUCache* Processor::get_cache() {
    return cache;
}

long Processor::get_total_cycle() {
    return total_cycle;
}

int Processor::get_compute_cycle() {
    return compute_cycle;
}

int Processor::get_count_mem_instr() {
    return count_mem_instr;
}

int Processor::get_idle_cycle() {
    return idle_cycle;
}

int Processor::get_count_cache_miss()
{
    return cache->count_cache_miss;
}
int Processor::get_count_data_traffic()
{
    return cache->count_data_traffic;
}
int Processor::get_count_update()
{
    return cache->count_update;
}
int Processor::get_count_private_access()
{
    return cache->count_private_access;
}
int Processor::get_count_shared_access()
{
    return cache->count_shared_access;
}

void Processor::run() {
    std::string str_val;
    uint32_t label;
    long val;
    while (benchmark_file >> label >> str_val) {
        val = std::stoi(str_val, nullptr, 16);
        if (label == 0 || label == 1) {
            count_mem_instr += 1;
            int set_index = (val / N) % M;
            int tag = (val / N) / M;
            if (label == 0) { // read
                idle_cycle += cache->pr_read(set_index, tag);      
            } else { // write
                idle_cycle += cache->pr_write(set_index, tag);
            }
            total_cycle += idle_cycle;
        } else {
            if (label != 2) {
                std::cout << "[ERROR] label index value goes out of range." << std::endl;
                return;
            }
            compute_cycle += val;
            total_cycle += val;
        }
    }     
    return; 
}