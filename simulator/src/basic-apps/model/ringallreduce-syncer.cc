#include <functional>
#include "ringallreduce-syncer.h"
#include "horovod-worker.h"
namespace ns3{
    // void GlobalRingAllReduceSyncer::AddWorkers(Ptr<HorovodWorker> p_worker){
    //     s_workers.push_back(p_worker);
    // }

    bool GlobalRingAllReduceSyncer::Empty(){
        return s_empty;
    }

    void GlobalRingAllReduceSyncer::AddRingallreduce(uint32_t priority){
        s_inflight_priority = priority;
        s_global_ringallreduce_status.insert({s_inflight_priority, s_synced_worker_ids});
        s_empty = false;
        return;
    }
    
    uint32_t GlobalRingAllReduceSyncer::GetPriority(){
        return s_inflight_priority;
    }

    uint32_t GlobalRingAllReduceSyncer::GetProgress(){
        return s_synced_worker_ids.size();
    }

    void GlobalRingAllReduceSyncer::AddSyncedWorker(uint32_t worker_id, std::function<void ()> func ){
        s_synced_worker_ids.push_back(worker_id);
        s_worker_update_ringallreduce_callbacks.push_back(func);

        return;
    }

    void GlobalRingAllReduceSyncer::NotifyAllWorkersRingallreduceDone(){
        for(auto func : s_worker_update_ringallreduce_callbacks ){
            func();
            s_synced_worker_ids.clear();
        }
        return;
    }


    bool GlobalRingAllReduceSyncer::AllWorkersSynced(uint32_t number_worker){
        return s_synced_worker_ids.size() == number_worker;
    }   


}
