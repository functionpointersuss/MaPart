#pragma once

#include <vector>
#include <cstdint>
#include <fstream>

class fpgaGraph {
public:
  fpgaGraph();
  fpgaGraph(std::ifstream& fpgaGraphFile);
  ~fpgaGraph();

  uint32_t getNodeDistance(uint32_t nodeSrc, uint32_t nodeDest);
  uint32_t getNumNodes();

private:
  uint32_t                           numNodes;
  std::vector<uint32_t>              fpgaNodeNeighborList;
  std::vector<uint32_t>              fpgaNodeStartList;
  std::vector<std::vector<uint32_t>> fpgaNodeDistances;

  void initializeFpgaGraph(std::ifstream& fpgaGraphFile);
  std::vector<uint32_t> bfs(uint32_t nodeStart);
  void calcNodeDistances();
};

