#ifndef _LOGGER_H
#define _LOGGER_H

#include <fstream>
#include <string>
#include <unistd.h>

#include "processor.h"

class Logger {
private:
    std::ofstream output_log;
    std::vector<Processor*> cores = std::vector<Processor*>(4);
    std::vector<LRUCache*> caches = std::vector<LRUCache*>(4);
    std::string output_path = "results/";
    long avg_overall = 0;
    long avg_idle = 0;
    double avg_miss = 0;
    int updates = 0;
    const int NUM_CORES = 4;
    int block_size;
public:
    Logger(Processor* core0, Processor* core1, Processor* core2, Processor* core3, std::string arguments, int _block_size)
    : block_size(_block_size)
    {
        cores[0] = core0;
        cores[1] = core1;
        cores[2] = core2;
        cores[3] = core3;

        caches[0] = core0->get_cache();
        caches[1] = core1->get_cache();
        caches[2] = core2->get_cache();
        caches[3] = core3->get_cache();

        output_path += arguments;
        int index = 1;
        while(!access((output_path + "_" + std::to_string(index) + ".log").c_str(), F_OK))
        {
            ++index;
        }

        output_path = output_path + "_" + std::to_string(index) + ".log";
        output_log.open(output_path, std::ios::out);

        output_log << "Input: " << arguments << std::endl;
        output_log << "=========================================" << std::endl;

    }

    void print_total_cycles() {
        output_log << "------------------------------" << std::endl;
        output_log << "1. Overall execution cycle per core" << std::endl;
        for (int i = 0; i < NUM_CORES; i++) {
            long curr_val = cores[i]->get_total_cycle();
            avg_overall += curr_val/NUM_CORES;
            output_log << "Core " << i << ": " << curr_val << std::endl;
        }
    }

    void print_compute_cycles() {
        output_log << "------------------------------" << std::endl;
        output_log << "2. Number of compute cycles per core" << std::endl;
        for (int i = 0; i < NUM_CORES; i++) {
            int curr_val = cores[i]->get_compute_cycle();
            output_log << "Core " << i << ": " << curr_val << std::endl;
        }
    }

    void print_count_mem_instr() {
        output_log << "------------------------------" << std::endl;
        output_log << "3. Number of load/store instructions per core" << std::endl;
        for (int i = 0; i < NUM_CORES; i++) {
            int curr_val = cores[i]->get_count_mem_instr();
            output_log << "Core " << i << ": " << curr_val << std::endl;
        }
    }

    void print_count_idle_cycle() {
        output_log << "------------------------------" << std::endl;
        output_log << "4. Number of idle cycles per core" << std::endl;
        for (int i = 0; i < NUM_CORES; i++) {
            int curr_val = cores[i]->get_idle_cycle();
            avg_idle += curr_val/NUM_CORES;
            output_log << "Core " << i << ": " << curr_val << std::endl;
        }
    }

    void print_cache_miss_rate() {
        output_log << "------------------------------" << std::endl;
        output_log << "5. Data cache miss rate for each core" << std::endl;
        for (int i = 0; i < NUM_CORES; i++) {
            int num_miss = cores[i]->get_count_cache_miss();
            int num_instr = cores[i]->get_count_mem_instr();
            double miss_rate = double(num_miss)/double(num_instr);
            avg_miss += miss_rate/NUM_CORES;
            output_log << "Core " << i << ": " << miss_rate << std::endl;
        }
    }

    void print_amt_of_data_traffic() {
        output_log << "------------------------------" << std::endl;
        output_log << "6. Amount of Data traffic in bytes on the bus" << std::endl;
        int sum_traffic = 0;
        for (int i = 0; i < NUM_CORES; i++) {
            sum_traffic += cores[i]->get_count_data_traffic();
        }
        sum_traffic *= block_size; // recheck
        output_log << sum_traffic << std::endl;
    }

    void print_count_update() {
        output_log << "------------------------------" << std::endl;
        output_log << "7. Number of invalidations or updates on the bus" << std::endl;
        int sum_update = 0;
        for (int i = 0; i < NUM_CORES; i++) {
            sum_update += cores[i]->get_count_update();
        }
        updates = sum_update;
        output_log << sum_update << std::endl;
    }

    void print_distribution_of_access() {
        output_log << "------------------------------" << std::endl;
        output_log << "8. Distribution of accesses to private data versus shared data" << std::endl;
        for (int i = 0; i < NUM_CORES; i++) {
            int num_private = cores[i]->get_count_private_access();
            int num_shared = cores[i]->get_count_shared_access();
            output_log << "Core " << i << ": Private acceses = " << num_private 
                        << " | Shared accesses = " << num_shared << std::endl;
        }

    }

    void print_analysis(std::string path, int cache_size, int associativity) {
        std::ofstream analysis_log;
        analysis_log.open(path, std::ios::app);
        analysis_log << cache_size << ',' << associativity << ',' << block_size << ',';
        int num_sets = (cache_size/associativity)/block_size;
        analysis_log << num_sets << ',' << block_size/4 << ',';
        analysis_log << avg_overall << ',' << avg_idle << ',' << avg_miss << ',' << updates << std::endl;
        analysis_log.close();
    }

    void print_summary() {

        print_total_cycles();
        print_compute_cycles();
        print_count_mem_instr();
        print_count_idle_cycle();
        print_cache_miss_rate();
        print_amt_of_data_traffic();
        print_count_update();
        print_distribution_of_access();

        output_log << "================== END ==================" << std::endl;
        output_log << "=========================================" << std::endl;
        output_log.close();

        std::cout << "DONE: The output summary can be found at " << output_path << std::endl;
    }


};

#endif // _LOGGER_H
