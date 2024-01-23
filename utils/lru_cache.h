#ifndef _LRU_CACHE_H
#define _LRU_CACHE_H

#include <vector>
#include <unordered_map>

#include "global_lock.h"
#include "config.h"

class Bus;

class CacheBlock {
public:
    int tag;
    int status;
    CacheBlock *prev;
    CacheBlock *next;
    CacheBlock(int _tag, int _status)
    : tag(_tag)
    , status(_status)
    , prev(nullptr)
    , next(nullptr)
    {}
};

class LRUCache {
public:
    int pid;
    int num_sets;
    int associativity;
    int block_size;
    Bus *bus;
    GlobalLock *gl;
    std::vector<std::unordered_map<int, CacheBlock*>> cache;
    std::vector<CacheBlock*> firsts;
    std::vector<CacheBlock*> lasts;

    LRUCache(int _cache_size, int _associativity, int _block_size, int _pid, Bus* _bus, GlobalLock* _gl)
    : pid(_pid)
    , num_sets((_cache_size/_block_size)/_associativity)
    , associativity(_associativity)
    , block_size(_block_size)
    , bus(_bus)
    , gl(_gl)
    {
        cache.resize(num_sets);
        firsts.resize(num_sets);
        lasts.resize(num_sets);
        for (int i = 0; i < num_sets; ++i)
        {
            firsts[i] = new CacheBlock(-1, -1);
            lasts[i] = new CacheBlock(-1, -1);
            firsts[i]->next = lasts[i];
            lasts[i]->prev = firsts[i];
        }
    }

    // Statistics
    int count_cache_miss = 0;
    int count_data_traffic = 0;
    int count_update = 0; // Number of invalidations or updates on the bus
    int count_private_access = 0;
    int count_shared_access = 0;

    int remove(CacheBlock* cacheBlock);
    void insert(CacheBlock* cacheBlock, int set_num);
    int removeLRUIfFull(int set_num, int associativity);

    virtual int pr_read(int set_num, int tag) = 0;
    virtual int pr_write(int set_num, int tag) = 0;

    virtual int get_status(int set_num, int tag) = 0;
    virtual void set_status(int set_num, int tag, int new_status) = 0;
};

class MESI_Cache : public LRUCache {
public:
    MESI_Cache(int _cache_size, int _associativity, int _block_size, int _pid, Bus* _bus, GlobalLock* _gl)
    : LRUCache(_cache_size, _associativity, _block_size, _pid, _bus, _gl)
    {}
    int pr_read(int set_num, int tag);
    int pr_write(int set_num, int tag);
    int get_status(int set_num, int tag);
    void set_status(int set_num, int tag, int new_status);
};

class Dragon_Cache : public LRUCache {
public:
    Dragon_Cache(int _cache_size, int _associativity, int _block_size, int _pid, Bus* _bus, GlobalLock* _gl)
    : LRUCache(_cache_size, _associativity, _block_size, _pid, _bus, _gl)
    {}
    int pr_read(int set_num, int tag);
    int pr_write(int set_num, int tag);
    int get_status(int set_num, int tag);
    void set_status(int set_num, int tag, int new_status);
};

#endif // _LRU_CACHE_H