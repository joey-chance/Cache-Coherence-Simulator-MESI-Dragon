#include "bus.h"
#include "processor.h"
#include "lru_cache.h"

void Bus::init_cores(Processor *p0, Processor *p1, Processor *p2, Processor *p3) 
{
    cores[0] = p0;
    cores[1] = p1;
    cores[2] = p2;
    cores[3] = p3;
}

void Bus::init_cache(LRUCache *c0, LRUCache *c1, LRUCache *c2, LRUCache *c3) 
{
    caches[0] = c0;
    caches[1] = c1;
    caches[2] = c2;
    caches[3] = c3;
}

/*
****************************************************
MESI Bus Protocol APIs
****************************************************
*/
int MESI_Bus::BusRd(int pid, int set_num, int tag)
{
    std::lock_guard<std::mutex> guard(bus_lock);
    for (int i = 0; i < NUM_CORES; ++i)
    {
        if (i == pid) continue;
        int status = caches[i]->get_status(set_num, tag);
        if (status != MESI_status::I)
        {
            if (status == MESI_status::M)
            {
                // M -> S, write back to memory
                caches[i]->count_data_traffic += 1;
                cores[i]->idle_cycle += 100;
            }
            caches[i]->set_status(set_num, tag, MESI_status::S);
            return status;
        }
    }
    return MESI_status::I;
}

int MESI_Bus::BusUpd(int pid, int set_num, int tag)
{
    std::lock_guard<std::mutex> guard(bus_lock);
    int count_invalidations = 0;
    for (int i = 0; i < NUM_CORES; ++i)
    {
        if (i == pid) continue;
        int status = caches[i]->get_status(set_num, tag);
        if (status != MESI_status::I)
        {
            ++count_invalidations;
            caches[i]->set_status(set_num, tag, MESI_status::I); // do i need to write back? no, the other cache has the most recent data
            // Comment for optimization
            if (!optimize && status == MESI_status::M)
                cores[i]->idle_cycle += 100;
        }
    }
    return count_invalidations;
}

/*
****************************************************
Dragon Bus Protocol APIs
****************************************************
*/
int Dragon_Bus::BusRd(int pid, int set_num, int tag)
{
    std::lock_guard<std::mutex> guard(bus_lock);
    for (int i = 0; i < NUM_CORES; ++i)
    {
        if (i == pid) continue;
        int status = caches[i]->get_status(set_num, tag);
        if (status == Dragon_status::Md)
        {
            caches[i]->set_status(set_num, tag, Dragon_status::Sm);
            return status;
        }
        else if (status == Dragon_status::Ed || status == Dragon_status::Sc)
        {
            caches[i]->set_status(set_num, tag, Dragon_status::Sc);
            return status;
        }
    }
    return Dragon_status::not_found;
}
int Dragon_Bus::BusUpd(int pid, int set_num, int tag)
{
    std::lock_guard<std::mutex> guard(bus_lock);
    int count_updates = 0;
    for (int i = 0; i < NUM_CORES; ++i)
    {
        if (i == pid) continue;
        if (caches[i]->get_status(set_num, tag) != Dragon_status::not_found)
        {
            ++count_updates;
            caches[i]->set_status(set_num, tag, Dragon_status::Sc); // do i need to write back? no, the other cache has the most recent data
        }
    }
    return count_updates;
}