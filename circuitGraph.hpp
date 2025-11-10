#pragma once

#include "rootGraph.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <fstream>

typedef struct {
  std::unordered_map<uint64_t, uint64_t> nodeToCluster;
  std::unordered_set<uint64_t> deletedEdges;
} graphClusterResult_t;

class circuitGraph {
public:
  circuitGraph();
  circuitGraph(std::ifstream& circuitGraphFile);
  ~circuitGraph();

  std::vector<uint64_t> circuitEdgeList;
  std::vector<uint64_t> circuitEdgeStartList;
  std::unordered_map<uint64_t, std::vector<uint64_t>> clusterSubnodes;
  std::unordered_map<uint64_t, std::vector<uint64_t>> nodeToEdgesMap;

  graphClusterResult_t createGraphClusters();
  circuitGraph coarsenGraph();
  rootGraph createRootGraph();

private:
};

