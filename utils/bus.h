#ifndef _BUS_H
#define _BUS_H

#include <mutex>
#include <vector>

#include "global_lock.h"
#include "config.h"

class Processor;
class LRUCache;

class Bus {
public:
    const int NUM_CORES = 4;
    int num_blocks;
    int associativity;
    int block_size;
    bool optimize;
    GlobalLock *gl;
    std::vector<Processor*> cores = std::vector<Processor*>(4);
    std::vector<LRUCache*> caches = std::vector<LRUCache*>(4);
    std::mutex bus_lock;

    Bus(int _cache_size, int _associativity, int _block_size, bool _optimize, GlobalLock* _gl)
    : num_blocks((_cache_size/_block_size)/_associativity)
    , associativity(_associativity)
    , block_size(_block_size)
    , optimize(_optimize)
    , gl(_gl)
    {}

    void init_cores(Processor* p0, Processor* p1, Processor* p2, Processor* p3);
    void init_cache(LRUCache* c0, LRUCache* c1, LRUCache* c2, LRUCache* c3);

    // Although this is not best practice,
    // child classes don't have additional data
    // hence a virtual destructor is not needed
    virtual int BusRd(int pid, int set_num, int tag) = 0;
    virtual int BusUpd(int pid, int set_num, int tag) = 0;
};

class MESI_Bus : public Bus {
public:
    MESI_Bus(int _cache_size, int _associativity, int _block_size, bool _optimize, GlobalLock* _gl)
    : Bus(_cache_size, _associativity, _block_size, _optimize, _gl)
    {}
    int BusRd(int pid, int set_num, int tag);
    int BusUpd(int pid, int set_num, int tag);
};

class Dragon_Bus : public Bus {
public:
    Dragon_Bus(int _cache_size, int _associativity, int _block_size, bool _optimize, GlobalLock* _gl)
    : Bus(_cache_size, _associativity, _block_size, _optimize, _gl)
    {}
    int BusRd(int pid, int set_num, int tag);
    int BusUpd(int pid, int set_num, int tag);
};

#endif // _BUS_H