#include <cassert>

#include "lru_cache.h"
#include "bus.h"
#include "config.h"

// Returns the cycles taken: 100 if write back, 0 if not
int LRUCache::remove(CacheBlock* cacheBlock)
{
    CacheBlock *p = cacheBlock->prev;
    CacheBlock *n = cacheBlock->next;
    p->next = n;
    n->prev = p;
    if (cacheBlock->status == MESI_status::M || cacheBlock->status == Dragon_status::Md || cacheBlock->status == Dragon_status::Sm)
    {
        // Write-Back
        ++count_data_traffic;
        return 100;
    }
    else 
    {
        return 0;
    }
}

void LRUCache::insert(CacheBlock* cacheBlock, int set_num)
{
    CacheBlock *p = lasts[set_num]->prev;
    CacheBlock *n = lasts[set_num];
    p->next = cacheBlock;
    n->prev = cacheBlock;
    cacheBlock->prev = p;
    cacheBlock->next = n;
}

int LRUCache::removeLRUIfFull(int set_num, int associativity)
{
    int cycles = 0;
    if (cache[set_num].size() >= associativity)
    {
        CacheBlock *lru = firsts[set_num]->next;
        cycles += remove(lru);
        cache[set_num].erase(lru->tag);
    }
    return cycles;
}

/*
****************************************************
MESI Cache Protocol APIs
****************************************************
*/
int MESI_Cache::pr_read(int set_num, int tag)
{
    gl->lockIdx(set_num);
    if (cache[set_num].find(tag) != cache[set_num].end())
    {
        CacheBlock *cacheBlock = cache[set_num][tag];
        remove(cacheBlock);
        if (cache[set_num][tag]->status != MESI_status::I)
        {
            // Read Hit
            insert(cacheBlock, set_num); // reinsert to make it most recently used
            switch (cacheBlock->status) {
                case MESI_status::M: // fallthrough
                case MESI_status::E:
                    ++count_private_access;
                    break;
                case MESI_status::S:
                    ++count_shared_access;
                    break;
            }
            gl->unlockIdx(set_num);
            return 1;
        }
        else
        {
            // Exists in cache but it has been invalidated (stale)
            cache[set_num].erase(cacheBlock->tag);
        }
    }

    // Read Miss
    int count_cycles = removeLRUIfFull(set_num, associativity);

    ++count_cache_miss;
    ++count_data_traffic;

    if (bus->BusRd(pid, set_num, tag) == MESI_status::I)
    {
        // I -> E
        // Fetch block from memory
        ++count_private_access;
        count_cycles += 100;
        cache[set_num][tag] = new CacheBlock(tag, MESI_status::E);
    }
    else 
    {
        // I -> S
        // Fetch block from another cache
        ++count_shared_access;
        count_cycles += 2 * (block_size/4);
        cache[set_num][tag] = new CacheBlock(tag, MESI_status::S);
    }
    insert(cache[set_num][tag], set_num);
    gl->unlockIdx(set_num);
    return count_cycles;
}

int MESI_Cache::pr_write(int set_num, int tag)
{
    int count_cycles = 1;
    int count_invalidations = 0;
    gl->lockIdx(set_num);
    if (cache[set_num].find(tag) != cache[set_num].end())
    {
        CacheBlock *cacheBlock = cache[set_num][tag];
        remove(cacheBlock);
        if (cacheBlock->status != MESI_status::I)
        {
            // Write Hit
            switch (cacheBlock->status) {
                case MESI_status::M:
                    ++count_private_access;
                    break;
                case MESI_status::E:
                    ++count_private_access;
                    cacheBlock->status = MESI_status::M;
                    break;
                case MESI_status::S:
                    ++count_shared_access;
                    cacheBlock->status = MESI_status::M;
                    count_invalidations = bus->BusUpd(pid, set_num, tag);
                    count_update += count_invalidations;
                    count_cycles += 2 * count_invalidations; // only need to invalidate, not sending the word
                    break;
            }
            insert(cacheBlock, set_num); // reinsert
            gl->unlockIdx(set_num);
            return count_cycles;
        }
        else
        {
            // Exists in cache but it has been invalidated (stale)
            cache[set_num].erase(cacheBlock->tag);
            remove(cacheBlock);
        }
    }

    // Write Miss
    // Read block into cache
    ++count_cache_miss;
    ++count_data_traffic;

    count_cycles += removeLRUIfFull(set_num, associativity);

    if (bus->BusRd(pid, set_num, tag) == MESI_status::I)
    {
        // Fetch block from memory
        ++count_private_access;
        count_cycles += 100;
    }
    else
    {
        // Fetch block from another cache
        count_cycles += 2*(block_size/4);

        count_invalidations = bus->BusUpd(pid, set_num, tag); // equivalent to BusRdX
        count_update += count_invalidations;
        ++count_shared_access;

        count_cycles += 2 * count_invalidations;
    }
    cache[set_num][tag] = new CacheBlock(tag, MESI_status::M);
    insert(cache[set_num][tag], set_num);

    gl->unlockIdx(set_num);
    return count_cycles;
}

int MESI_Cache::get_status(int set_num, int tag)
{
    if (cache[set_num].find(tag) != cache[set_num].end())
        return cache[set_num][tag]->status;
    else
        return MESI_status::I;
}

void MESI_Cache::set_status(int set_num, int tag, int new_status)
{
    if (cache[set_num].find(tag) != cache[set_num].end())
        cache[set_num][tag]->status = new_status;
}

/*
****************************************************
Dragon Cache Protocol APIs
****************************************************
*/
int Dragon_Cache::pr_read(int set_num, int tag)
{
    gl->lockIdx(set_num);
    if (cache[set_num].find(tag) != cache[set_num].end())
    {
        CacheBlock *cacheBlock = cache[set_num][tag];
        remove(cacheBlock);
        if (cache[set_num][tag]->status != Dragon_status::not_found)
        {
            // Read Hit
            insert(cacheBlock, set_num); // reinsert to make it most recently used
            switch (cacheBlock->status) {
                case Dragon_status::Md: // fallthrough
                case Dragon_status::Ed:
                    ++count_private_access;
                    break;
                case Dragon_status::Sm:
                case Dragon_status::Sc:
                    ++count_shared_access;
                    break;
            }
            gl->unlockIdx(set_num);
            return 1;
        }
        else
        {
            // Else condition should never happen because there's no invalidation 
            // Exists in cache but it has been invalidated (stale)
            std::cout << "ERROR: Invalidated Dragon cache." << std::endl;
            cache[set_num].erase(cacheBlock->tag);
            remove(cacheBlock);
        }
    }

    // Read Miss
    int count_cycles = removeLRUIfFull(set_num, associativity);

    ++count_cache_miss;
    ++count_data_traffic;

    if (bus->BusRd(pid, set_num, tag) == Dragon_status::not_found)
    {
        // not_found -> E
        // Fetch block from memory
        ++count_private_access;
        count_cycles += 100;
        cache[set_num][tag] = new CacheBlock(tag, Dragon_status::Ed);
    }
    else 
    {
        // not_found -> Sc
        // Fetch block from another cache
        ++count_shared_access;
        count_cycles += 2 * (block_size/4);
        cache[set_num][tag] = new CacheBlock(tag, Dragon_status::Sc);
    }
    insert(cache[set_num][tag], set_num);
    gl->unlockIdx(set_num);
    return count_cycles;
}

int Dragon_Cache::pr_write(int set_num, int tag)
{
    int count_cycles = 1;
    int count_invalidations = 0;
    gl->lockIdx(set_num);
    if (cache[set_num].find(tag) != cache[set_num].end())
    {
        CacheBlock *cacheBlock = cache[set_num][tag];
        remove(cacheBlock);
        if (cacheBlock->status != Dragon_status::not_found)
        {
            // Write Hit
            switch (cacheBlock->status) {
                case Dragon_status::Md:
                    ++count_private_access;
                    break;
                case Dragon_status::Ed:
                    ++count_private_access;
                    cacheBlock->status = Dragon_status::Md;
                    break;
                case Dragon_status::Sc:
                case Dragon_status::Sm:
                    if (bus->BusRd(pid, set_num, tag) == Dragon_status::not_found)
                    {
                        // Not found in other cache
                        ++count_private_access;
                        cacheBlock->status = Dragon_status::Md;
                    }
                    else
                    {
                        // Found in other caches
                        // Each write to another cache block incurs 2N cycles
                        ++count_shared_access;
                        count_invalidations = bus->BusUpd(pid, set_num, tag);
                        cacheBlock->status = Dragon_status::Sm;
                        count_update += count_invalidations;
                        count_data_traffic += count_invalidations;
                        count_cycles += count_invalidations * 2 * (block_size/4);
                    }
                    break;
            }
            insert(cacheBlock, set_num); // reinsert
            gl->unlockIdx(set_num);
            return count_cycles;
        }
        else
        {
            // Else condition should never happen because there's no invalidation 
            // Exists in cache but it has been invalidated (stale)
            std::cout << "ERROR: Invalidated Dragon cache." << std::endl;
            cache[set_num].erase(cacheBlock->tag);
            remove(cacheBlock);
        }
    }

    // Write Miss
    // Read block into cache
    ++count_cache_miss;
    ++count_data_traffic;

    count_cycles += removeLRUIfFull(set_num, associativity);

    if (bus->BusRd(pid, set_num, tag) == Dragon_status::not_found)
    {
        // Fetch block from memory
        ++count_private_access;
        count_cycles += 100;
        cache[set_num][tag] = new CacheBlock(tag, Dragon_status::Md);
    }
    else
    {
        // Fetch block from another cache
        count_cycles += 2*(block_size/4);
        cache[set_num][tag] = new CacheBlock(tag, Dragon_status::Sm);

        count_invalidations = bus->BusUpd(pid, set_num, tag);
        count_update += count_invalidations;
        count_data_traffic += count_invalidations;
        ++count_shared_access;

        count_cycles += 2*count_invalidations*(block_size/4);
    }
    insert(cache[set_num][tag], set_num);

    gl->unlockIdx(set_num);
    return count_cycles;
}

int Dragon_Cache::get_status(int set_num, int tag)
{
    if (cache[set_num].find(tag) != cache[set_num].end())
        return cache[set_num][tag]->status;
    else
        return Dragon_status::not_found;
}

void Dragon_Cache::set_status(int set_num, int tag, int new_status)
{
    if (cache[set_num].find(tag) != cache[set_num].end())
        cache[set_num][tag]->status = new_status;
}