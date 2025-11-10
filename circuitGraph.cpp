#include "circuitGraph.hpp"
#include "rootGraph.hpp"
#include <algorithm>
#include <queue>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>

circuitGraph::circuitGraph() {
}

circuitGraph::~circuitGraph() {
}

circuitGraph::circuitGraph(std::ifstream& circuitGraphFile) {
  uint64_t numEdges;
  uint64_t numNodes;

  std::string line;
  std::getline(circuitGraphFile, line);
  std::istringstream ss(line);
  ss >> numEdges >> numNodes;

  uint32_t edgeStartIdx = 0;
  for (uint32_t i = 0; i < numEdges; i++) {
    uint64_t edgeNode;
    uint32_t edgeDegree = 0;

    ss.clear();
    std::getline(circuitGraphFile, line);
    ss.str(line);
    while (ss >> edgeNode) {
      circuitEdgeList.push_back(edgeNode);
      nodeToEdgesMap[edgeNode].push_back(circuitEdgeStartList.size());
      edgeDegree++;
    }
    circuitEdgeStartList.push_back(edgeStartIdx);
    edgeStartIdx += edgeDegree;
  }
  // Padding for the graph edge list
  circuitEdgeStartList.push_back(edgeStartIdx);

  if (std::getline(circuitGraphFile, line)) {
    std::cout << "More edges in file than specified edges field" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  for (uint32_t i = 0; i < numNodes; i++) {
    clusterSubnodes[i] = {i};
  }

  return;
}

graphClusterResult_t circuitGraph::createGraphClusters() {
  graphClusterResult_t result;

  // Get number of edges
  uint64_t numEdges = circuitEdgeStartList.size() - 1;

  // Create vector of edge indices and sort by degree (descending)
  std::vector<uint64_t> sortedEdges(numEdges);
  for (uint64_t i = 0; i < numEdges; i++) {
    sortedEdges[i] = i;
  }

  std::sort(sortedEdges.begin(), sortedEdges.end(), [this](uint64_t a, uint64_t b) {
    uint64_t degreeA = circuitEdgeStartList[a+1] - circuitEdgeStartList[a];
    uint64_t degreeB = circuitEdgeStartList[b+1] - circuitEdgeStartList[b];
    return degreeA > degreeB; // Sort descending by degree
  });

  // Track marked nodes
  std::unordered_set<uint64_t> markedNodes;

  // Track unmerged edges
  std::queue<uint64_t> unmergedEdges;

  // Cluster counter
  uint64_t clusterIdx = 0;

  // First pass: iterate through sorted edges
  for (uint64_t edgeIdx : sortedEdges) {
    uint64_t edgeStart = circuitEdgeStartList[edgeIdx];
    uint64_t edgeEnd = circuitEdgeStartList[edgeIdx + 1];

    // Check if all nodes in this edge are unmarked
    bool allUnmarked = true;
    for (uint64_t i = edgeStart; i < edgeEnd; i++) {
      uint64_t node = circuitEdgeList[i];
      if (markedNodes.contains(node)) {
        allUnmarked = false;
        break;
      }
    }

    if (allUnmarked) {
      // Create cluster from all nodes in this edge
      for (uint64_t i = edgeStart; i < edgeEnd; i++) {
        uint64_t node = circuitEdgeList[i];
        result.nodeToCluster[node] = clusterIdx;
        markedNodes.insert(node);
      }

      // Add to deleted edges
      result.deletedEdges.insert(edgeIdx);

      // Increment cluster index
      clusterIdx++;
    } else {
      // Add to unmerged edges
      unmergedEdges.push(edgeIdx);
    }
  }

  // Second pass: iterate through unmerged edges
  while (!unmergedEdges.empty()) {
    uint64_t edgeIdx = unmergedEdges.front();
    unmergedEdges.pop();

    uint64_t edgeStart = circuitEdgeStartList[edgeIdx];
    uint64_t edgeEnd = circuitEdgeStartList[edgeIdx + 1];

    // Collect unmarked nodes in this edge
    std::vector<uint64_t> unmarkedNodesInEdge;
    for (uint64_t i = edgeStart; i < edgeEnd; i++) {
      uint64_t node = circuitEdgeList[i];
      if (!markedNodes.contains(node)) {
        unmarkedNodesInEdge.push_back(node);
      }
    }

    // If there are unmarked nodes, create a cluster from them
    if (!unmarkedNodesInEdge.empty()) {
      for (uint64_t node : unmarkedNodesInEdge) {
        result.nodeToCluster[node] = clusterIdx;
        markedNodes.insert(node);
      }
      clusterIdx++;
    }
  }

  return result;
}

circuitGraph circuitGraph::coarsenGraph() {
  // Get clustering results
  graphClusterResult_t clusterResult = createGraphClusters();

  // Create new coarsened graph
  circuitGraph coarsened;

  // Build clusterSubnodes for coarsened graph
  // For each circuitNode -> clusterNode mapping, add all subnodes from this graph's
  // clusterSubnodes[circuitNode] to coarsened.clusterSubnodes[clusterNode]
  for (const auto& [circuitNode, clusterNode] : clusterResult.nodeToCluster) {
    for (uint64_t subnode : this->clusterSubnodes[circuitNode]) {
      coarsened.clusterSubnodes[clusterNode].push_back(subnode);
    }
  }

  // Build edge lists for coarsened graph
  uint64_t numEdges = circuitEdgeStartList.size() - 1;
  uint64_t edgeStartIdx = 0;
  uint64_t coarsenedEdgeIdx = 0;

  // Iterate through all edges
  for (uint64_t edgeIdx = 0; edgeIdx < numEdges; edgeIdx++) {
    // Skip edges in the deleted edge set
    if (clusterResult.deletedEdges.contains(edgeIdx)) {
      continue;
    }

    // Get nodes in this edge
    uint64_t edgeStart = circuitEdgeStartList[edgeIdx];
    uint64_t edgeEnd = circuitEdgeStartList[edgeIdx + 1];

    // Transform nodes to clusters using set for deduplication
    // Multiple circuit nodes may map to the same cluster, so we need to deduplicate
    std::unordered_set<uint64_t> clusterSet;
    for (uint64_t i = edgeStart; i < edgeEnd; i++) {
      uint64_t node = circuitEdgeList[i];
      // Look up which cluster this node belongs to
      if (clusterResult.nodeToCluster.contains(node)) {
        uint64_t cluster = clusterResult.nodeToCluster[node];
        clusterSet.insert(cluster);
      }
    }

    // Add edge start index to coarsened graph
    coarsened.circuitEdgeStartList.push_back(edgeStartIdx);

    // Add deduplicated cluster nodes to edge list and update nodeToEdgesMap
    for (uint64_t cluster : clusterSet) {
      coarsened.circuitEdgeList.push_back(cluster);
      coarsened.nodeToEdgesMap[cluster].push_back(coarsenedEdgeIdx);
    }

    // Update indices
    edgeStartIdx += clusterSet.size();
    coarsenedEdgeIdx++;
  }

  // Add final padding to edge start list
  coarsened.circuitEdgeStartList.push_back(edgeStartIdx);

  return coarsened;
}

rootGraph circuitGraph::createRootGraph() {
  // Use a map to accumulate edge weights
  // Outer map: source node -> (inner map: dest node -> weight)
  std::map<uint64_t, std::map<uint64_t, double>> adjacencyMap;

  // Get number of edges
  uint64_t numEdges = circuitEdgeStartList.size() - 1;

  // Iterate through each hyperedge
  for (uint64_t edgeIdx = 0; edgeIdx < numEdges; edgeIdx++) {
    uint64_t edgeStart = circuitEdgeStartList[edgeIdx];
    uint64_t edgeEnd = circuitEdgeStartList[edgeIdx + 1];
    uint64_t hyperedgeSize = edgeEnd - edgeStart;

    // Skip hyperedges with size <= 1 (no pairs to connect)
    if (hyperedgeSize <= 1) {
      continue;
    }

    // Calculate edge weight for this hyperedge
    double edgeWeight = 1.0 / (hyperedgeSize - 1);

    // Get all nodes in this hyperedge
    std::vector<uint64_t> nodesInEdge;
    for (uint64_t i = edgeStart; i < edgeEnd; i++) {
      nodesInEdge.push_back(circuitEdgeList[i]);
    }

    // Connect every pair of nodes in this hyperedge
    for (size_t i = 0; i < nodesInEdge.size(); i++) {
      for (size_t j = i + 1; j < nodesInEdge.size(); j++) {
        uint64_t node1 = nodesInEdge[i];
        uint64_t node2 = nodesInEdge[j];

        // Add edge in both directions (undirected graph)
        adjacencyMap[node1][node2] += edgeWeight;
        adjacencyMap[node2][node1] += edgeWeight;
      }
    }
  }

  // Convert adjacency map to CSR format
  rootGraph root;
  uint64_t nodeStartIdx = 0;

  // Iterate through adjacency map in sorted order
  for (const auto& [node, neighbors] : adjacencyMap) {
    // Add start index for this node
    root.rootNodeStartList.push_back(nodeStartIdx);

    // Add all neighbors and their edge weights
    for (const auto& [neighbor, weight] : neighbors) {
      root.rootNodeNeighborList.push_back(neighbor);
      root.rootEdgeWeights.push_back(weight);
      nodeStartIdx++;
    }
  }

  // Add final padding to node start list
  root.rootNodeStartList.push_back(nodeStartIdx);

  return root;
}
