#pragma once

#include <vector>
#include <cstdint>

class rootGraph {
public:
  rootGraph();
  ~rootGraph();

  std::vector<uint64_t> rootNodeNeighborList;
  std::vector<uint64_t> rootNodeStartList;
  std::vector<double>   rootEdgeWeights;

private:
};
