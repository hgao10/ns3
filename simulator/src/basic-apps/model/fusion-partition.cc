#include <stdint.h>
#include "fusion-partition.h"

namespace ns3 {

    void FusionPartition::SetSize(uint32_t size) {p_size_bytes = size;}
    uint32_t FusionPartition::GetSize() {return p_size_bytes;}
    void FusionPartition::SetIdx(uint32_t idx) {p_idx = idx;}
    uint32_t FusionPartition::GetIdx(){return p_idx;}
    void FusionPartition::UpdateProgress(uint32_t progress) {p_progress = progress;}
    uint32_t FusionPartition::GetProgress(){return p_progress;}
    void FusionPartition::ResetProgress(){p_progress = 0;}
    RingAllReduce* FusionPartition::GetParent(){return p_parent;}
    void FusionPartition::SetParent(RingAllReduce* parent){p_parent = parent;}

}

