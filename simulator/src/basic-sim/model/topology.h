#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"

namespace ns3 {

class Topology : public Object
{
public:
    static TypeId GetTypeId (void);
    virtual NodeContainer GetNodes() = 0;
    virtual bool IsValidEndpoint(int64_t node_id) = 0;
    virtual std::set<int64_t> GetEndpoints() = 0;
};

}

#endif //TOPOLOGY_H
