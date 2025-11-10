# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MaPart is a C++ implementation of a multi-level FPGA circuit partitioning algorithm. The system partitions circuit graphs onto FPGA architectures using coarsening and propagation-based placement.

## Architecture

### Core Components

**Graph Representations:**
- `circuitGraph`: Represents the circuit netlist as a hypergraph
  - Stores edges as lists (CSR-like format with `circuitEdgeList` and `circuitEdgeStartList`)
  - Maintains `nodeToEdgesMap` for reverse lookup (node -> edges containing that node)
  - Tracks cluster subnodes via `clusterSubnodes` for hierarchical coarsening

- `fpgaGraph`: Represents the FPGA physical topology
  - Standard graph adjacency list format (`fpgaNodeNeighborList`, `fpgaNodeStartList`)
  - Pre-computes all-pairs shortest paths in `fpgaNodeDistances` via BFS during initialization
  - Provides O(1) distance queries between any two FPGA nodes

**Partitioner:**
- Main algorithm coordinator combining coarsening and propagation
- Key data structures:
  - `candidatePartitions`: Maps circuit nodes to legal FPGA partition candidates
  - `propagatedNodes`: Tracks which circuit nodes have been placed/propagated

**Algorithm Phases:**

1. **Coarsening** (coarsener.cpp/hpp - incomplete):
   - Reduces circuit graph size by clustering nodes
   - Implementation is stubbed out

2. **Propagation** (propagate.cpp):
   - Core placement constraint propagation
   - `findPriorityCircuitNodes()`: BFS-based search within hop limit from a source node, excluding already-propagated nodes
   - `propagate()`: Places a node and propagates constraints to neighbors within distance bounds
   - Uses FPGA topology distances to filter legal placements

### Graph File Format

**Circuit Graph:**
- First line: `<numEdges> <numNodes>`
- Next numEdges lines: space-separated node IDs in each hyperedge

**FPGA Graph:**
- First line: `<numNodes>`
- Next numNodes lines: space-separated neighbor node IDs for each FPGA node

### Key Algorithm Details

The propagation algorithm maintains legality by:
1. When a circuit node is placed on FPGA partition P, find all neighbors within max FPGA diameter
2. For each neighbor at circuit distance D from placed node:
   - Filter candidate FPGA partitions to only those within distance D from P
   - If candidates reduced to 1: add to propagation queue
   - If candidates reduced to 0: partitioning failed (return error)
3. Continue until propagation queue empty

## Building and Running

No build system is currently set up. To compile individual object files:

```bash
g++ -std=c++20 -c circuitGraph.cpp -o circuitGraph.o
g++ -std=c++20 -c fpgaGraph.cpp -o fpgaGraph.o
g++ -std=c++20 -c rootGraph.cpp -o rootGraph.o
g++ -std=c++20 -c propagate.cpp -o propagate.o
g++ -std=c++20 -c partition.cpp -o partition.o
g++ -std=c++20 -c coarsener.cpp -o coarsener.o
```

**Important:** Code requires C++20 for `std::unordered_set::contains()` and `std::unordered_map::contains()` methods.

Note: No main() entry point exists yet, so linking into an executable is not currently possible.

## Development Notes

- The codebase uses compressed sparse row (CSR) style graph storage for efficiency
- All indices are 0-based
- The propagation algorithm assumes valid FPGA distances are pre-computed
- Coarsening phase is partially implemented (createGraphClusters and coarsenGraph methods exist)
- No main() entry point exists yet

## Coarsening Implementation

The circuitGraph class includes methods for hypergraph coarsening:

- **`createGraphClusters()`**: Performs modified hyperedge matching
  - Sorts hyperedges by degree (descending)
  - Creates clusters from hyperedges where all nodes are unmarked
  - Returns node-to-cluster mapping and set of deleted edges

- **`coarsenGraph()`**: Creates a coarsened hypergraph from clustering results
  - Transfers subnodes hierarchically
  - Removes deleted edges, deduplicates nodes within edges using sets
  - Returns a new circuitGraph with reduced node count

- **`createRootGraph()`**: Converts hypergraph to weighted graph (rootGraph class)
  - Connects all node pairs within each hyperedge
  - Edge weight = 1/(hyperedge_size - 1)
  - Accumulates weights when edges connect multiple times
  - Useful for graph partitioning algorithms
