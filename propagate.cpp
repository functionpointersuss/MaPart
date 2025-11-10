#include "partition.hpp"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

std::vector<hopLimitedNode_t> partitioner::findPriorityCircuitNodes(uint64_t nodeSrc, uint64_t stopDistance) {
  std::queue<uint64_t> nodeQueue;
  std::unordered_set<uint64_t> visitedNode;
  std::unordered_map<uint64_t, uint64_t> visitedNodeDist;
  std::vector<hopLimitedNode_t> hopLimitedNodeList;

  // Initialize Starting node for BFS
  nodeQueue.push(nodeSrc);
  visitedNode.insert(nodeSrc);
  visitedNodeDist[nodeSrc] = 0;

  // Run BFS until you hit a stop distance
  while (!nodeQueue.empty()) {
    uint64_t node = nodeQueue.front();
    nodeQueue.pop();

    // Get all edges of this node
    for (const auto& edge : circuitGraphInstance.nodeToEdgesMap[node]) {
      // For each edge we process the node's neighbors
      for (uint64_t i = circuitGraphInstance.circuitEdgeStartList[edge]; i < circuitGraphInstance.circuitEdgeStartList[edge+1]; i++) {
        uint64_t neighborNode = circuitGraphInstance.circuitEdgeList[i];

        // Only process nodes that were not propagated or previously visited
        if (!visitedNode.contains(neighborNode) && !propagatedNodes.contains(neighborNode)) {
          uint64_t visitedNeighborNodeDist = visitedNodeDist[node] + 1;

          // Mark as visited, and add this node to the list of nodes under the hop count
          visitedNode.insert(neighborNode);
          visitedNodeDist[neighborNode] = visitedNeighborNodeDist;
          hopLimitedNodeList.push_back({.node = neighborNode, .dist = visitedNeighborNodeDist});

          // Only add this node for further depth if we are under the stop distance
          if (visitedNeighborNodeDist < stopDistance)
            nodeQueue.push(neighborNode);
        }
      }
    }
  }

  return hopLimitedNodeList;
}

uint32_t partitioner::propagate(uint64_t placedNode, uint32_t placedNodePartition) {
  // Get maximum partition distance from placed partition
  uint32_t maxDistFromPlacedPart = 0;
  for (uint32_t i = 0; i < fpgaGraphInstance.getNumNodes(); i++) {
    maxDistFromPlacedPart = std::max(maxDistFromPlacedPart, fpgaGraphInstance.getNodeDistance(placedNodePartition, i));
  }

  std::queue<uint64_t> propagateQueue;
  propagateQueue.push(placedNode);
  while (!propagateQueue.empty()) {
    uint64_t nodeToPropagate = propagateQueue.front();
    propagateQueue.pop();

    // Get list of all circuit nodes with distance less than max partition distance, i.e. we'd really like to place these now,
    // since later they might become illegals in my yard
    std::vector<hopLimitedNode_t> nodesUnderMaxDist = findPriorityCircuitNodes(nodeToPropagate, maxDistFromPlacedPart);
    for (uint32_t nodeIdx = 0; nodeIdx < nodesUnderMaxDist.size(); nodeIdx++) {
      hopLimitedNode_t node = nodesUnderMaxDist[nodeIdx];
      uint32_t nodeDist = node.dist;

      // Get list of FPGAs that are under node dist away (legal placements for this circuit node)
      std::unordered_set<uint32_t> legalPartitions;
      for (uint32_t fpga = 0; fpga < fpgaGraphInstance.getNumNodes(); fpga++) {
        if (fpgaGraphInstance.getNodeDistance(placedNodePartition, fpga) < nodeDist) {
          legalPartitions.insert(fpga);
        }
      }

      // Update candidate list for this circuit node, with remaining legal placements
      std::vector<uint32_t>& candidatePartition = candidatePartitions[node.node];
      auto eraseNonlegalParts = std::remove_if(candidatePartition.begin(), candidatePartition.end(), [&legalPartitions](uint32_t part){return !legalPartitions.contains(part);});
      candidatePartition.erase(eraseNonlegalParts, candidatePartition.end());

      // Based on candidate partition size, either terminate no solution or add this node to be propagated
      if (candidatePartition.size() == 1)
        propagateQueue.push(node.node);
      else if (candidatePartition.size() == 0)
        return 1;
    }

    // Add node to list of propagated nodes
    propagatedNodes.insert(nodeToPropagate);
  }
  return 0;
}
