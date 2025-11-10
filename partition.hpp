#pragma once

#include "fpgaGraph.hpp"
#include "circuitGraph.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

typedef struct {
  uint64_t node;
  uint64_t dist;
} hopLimitedNode_t;

class partitioner {
public:
  partitioner();
  ~partitioner();

private:
  fpgaGraph    fpgaGraphInstance;
  circuitGraph circuitGraphInstance;
  std::unordered_map<uint64_t, std::vector<uint32_t>> candidatePartitions;

  // Coarsening

  // Propogation
  std::unordered_set<uint64_t> propagatedNodes;
  std::vector<hopLimitedNode_t> findPriorityCircuitNodes(uint64_t nodeSrc, uint64_t stopDistance);
  uint32_t propagate(uint64_t placedNode, uint32_t placedNodePartition);
};

