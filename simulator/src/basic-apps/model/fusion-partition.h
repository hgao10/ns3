#ifndef FUSION_PARTITION_H
#define FUSION_PARTITION_H
#include <stdint.h>
namespace ns3 {
class RingAllReduce;

class FusionPartition
{
  public:
    FusionPartition(){};
    virtual ~FusionPartition(){};
    void SetSize(uint32_t size);
    uint32_t GetSize();
    void SetIdx(uint32_t idx);
    uint32_t GetIdx();
    void UpdateProgress();
    uint32_t GetProgress();
    void ResetProgress();
    // std::vector<uint32_t>  r_tensors;
    RingAllReduce*  GetParent();
    void SetParent(RingAllReduce* parent);
  private:
    uint32_t p_size_bytes = 0;
    uint32_t p_progress = 0;
    uint32_t p_iteration = 0;
    RingAllReduce* p_parent;
    uint32_t p_idx;
};
}
#endif