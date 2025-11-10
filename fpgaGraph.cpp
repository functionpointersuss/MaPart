#include "fpgaGraph.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <queue>
#include <cstdlib>
#include <cstdint>

fpgaGraph::fpgaGraph() {
}

fpgaGraph::~fpgaGraph() {
}

void fpgaGraph::initializeFpgaGraph(std::ifstream& fpgaGraphFile) {
  std::string line;
  std::getline(fpgaGraphFile, line);
  std::istringstream ss(line);
  ss >> numNodes;

  uint32_t nodeStartIdx = 0;
  for (uint32_t i = 0; i < numNodes; i++) {
    uint32_t nodeNeighbor;
    uint32_t nodeDegree = 0;
    ss.clear();
    std::getline(fpgaGraphFile, line);
    ss.str(line);
    while (ss >> nodeNeighbor) {
      fpgaNodeNeighborList.push_back(nodeNeighbor);
      nodeDegree++;
    }
    fpgaNodeStartList.push_back(nodeStartIdx);
    nodeStartIdx+=nodeDegree;
  }
  // Padding for the graph edge list starts so you have the last node's end index
  fpgaNodeStartList.push_back(nodeStartIdx);

  if (std::getline(fpgaGraphFile, line)) {
    std::cout << "More nodes in file than specified nodes field" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  return;
}

// Get BFS distances from a single node to all other nodes
std::vector<uint32_t> fpgaGraph::bfs(uint32_t nodeSrc) {
  std::queue<uint32_t> nodeQueue;
  std::vector<uint32_t> nodeDistances(numNodes, UINT32_MAX);

  // Node is zero from itself
  nodeDistances[nodeSrc] = 0;

  // Run BFS going through all nodes and getting their distances
  nodeQueue.push(nodeSrc);
  while (!nodeQueue.empty()) {
    uint32_t node = nodeQueue.front();
    nodeQueue.pop();

    // Look at all neighbors of this node
    for (uint32_t i = fpgaNodeStartList[node]; i < fpgaNodeStartList[node+1]; i++) {
      uint32_t neighborNode = fpgaNodeNeighborList[i];
      if (nodeDistances[neighborNode] == UINT32_MAX) {
        nodeDistances[neighborNode] = nodeDistances[node] + 1;
        nodeQueue.push(neighborNode);
      }
    }
  }

  return nodeDistances;
}

void fpgaGraph::calcNodeDistances() {
  // Run BFS for each node
  for (uint32_t i = 0; i < numNodes; i++) {
    fpgaNodeDistances.push_back(bfs(i));
  }
}

uint32_t fpgaGraph::getNodeDistance(uint32_t nodeSrc, uint32_t nodeDest) {
  return fpgaNodeDistances[nodeSrc][nodeDest];
}

uint32_t fpgaGraph::getNumNodes() {
  return numNodes;
}

fpgaGraph::fpgaGraph(std::ifstream& fpgaGraphFile) {
  initializeFpgaGraph(fpgaGraphFile);
  calcNodeDistances();
}
