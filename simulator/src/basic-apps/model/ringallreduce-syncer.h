#ifndef RINGALLREDUCE_SYNCER_H
#define RINGALLREDUCE_SYNCER_H
#include <stdint.h>
#include <map>
#include <vector>
#include <algorithm>
#include "ns3/ptr.h"

namespace ns3{
class HorovodWorker;
class GlobalRingAllReduceSyncer{
    public:
        GlobalRingAllReduceSyncer(){};
        ~GlobalRingAllReduceSyncer(){};
        // void AddWorkers(Ptr<HorovodWorker> p_worker);
        bool Empty();
        void AddRingallreduce(uint32_t priority);
        uint32_t GetPriority();
        uint32_t GetProgress();
        void AddSyncedWorker(uint32_t worker_id, std::function<void ()> func);
        void NotifyAllWorkersRingallreduceDone();
        bool AllWorkersSynced(uint32_t number_worker);
        void RemoveSyncedRingallreduce();


    private:
        // std::vector<Ptr<HorovodWorker>> s_workers;
        // bool s_empty = true;
        std::vector<uint32_t> s_synced_worker_ids;
        uint32_t s_inflight_priority;
        std::map<uint32_t, std::vector<uint32_t>> s_global_ringallreduce_status; //key: prio, value: worker_ids that are ready 
        std::vector<std::function<void ()>> s_worker_update_ringallreduce_callbacks;
};
}
#endif