#ifndef _GLOBAL_LOCK_H
#define _GLOBAL_LOCK_H

/**
 * Global Lock
 * A global lock is needed for each set to avoid race conditions when accessing 
 * same or different memory at addresses with the same block number.
 * This preserves the total program order for accesses to these memory addresses.
*/

#include <iostream>
#include <mutex>
#include <vector>

class GlobalLock {
public:
    int num_blocks;
    std::vector<std::mutex> mutexes;

    GlobalLock(int cache_size, int associativity, int block_size)
    {
        this->num_blocks = (cache_size / block_size) / associativity;
        mutexes = std::vector<std::mutex>(this->num_blocks);
    }

    void lockIdx(int idx)
    {
        if (idx < 0 || idx >= this->num_blocks)
            std::cout << "ERROR: Trying to acquire an out-of-bounds lock." << std::endl;
        else
            mutexes[idx].lock();
    }

    void unlockIdx(int idx)
    {
        if (idx < 0 || idx >= this->num_blocks)
            std::cout << "ERROR: Trying to acquire an out-of-bounds lock." << std::endl;
        else
            mutexes[idx].unlock();
    }
};

#endif // _GLOBAL_LOCK_H