#ifndef SPATIALGRID_H
#define SPATIALGRID_H

#include "../particle/SteamParticle.h"
#include <cglm/cglm.h>
#include <vector>

class SpatialGrid {
public:
  SpatialGrid() : cellSize(0.1f) { grid.resize(TABLE_SIZE); }
  ~SpatialGrid() {}

  void Build(const std::vector<SteamParticle> &particles) {
    Clear();
    for (size_t i = 0; i < particles.size(); ++i) {
      if (!particles[i].isActive())
        continue; // Optional: skip inactive
      int id = GetGridIndex(particles[i].position, cellSize);
      grid[id].push_back(i);
    }
  }

  void Clear() {
    for (auto &bucket : grid) {
      bucket.clear();
    }
  }

  // Retrieve neighbors from the grid for a given position
  // Checks the cell the particle is in and the 26 surrounding cells
  std::vector<int> GetNeighbors(const vec3 position) {
    std::vector<int> neighbors;
    int cx = (int)(position[0] / cellSize);
    int cy = (int)(position[1] / cellSize);
    int cz = (int)(position[2] / cellSize);

    for (int x = cx - 1; x <= cx + 1; ++x) {
      for (int y = cy - 1; y <= cy + 1; ++y) {
        for (int z = cz - 1; z <= cz + 1; ++z) {
          // We need to manually invoke hash logic or refactor GetGridIndex to
          // accept indices For simplicity, reusing GetGridIndex by
          // reconstructing a test position or better, refactoring GetGridIndex
          // to take grid coords.

          // Let's refactor GetGridIndex to be usable here if possible,
          // OR mostly just replicate the hash logic here for speed/access since
          // GetGridIndex takes vec3.

          long h1 = x * 73856093;
          long h2 = y * 19349663;
          long h3 = z * 83492791;

          long hash = (h1 ^ h2 ^ h3) % TABLE_SIZE;
          if (hash < 0)
            hash += TABLE_SIZE;
          int id = (int)hash;

          const auto &bucket = grid[id];
          neighbors.insert(neighbors.end(), bucket.begin(), bucket.end());
        }
      }
    }
    return neighbors;
  }

  // Retrieve neighbors from the grid for a given position
  std::vector<int> GetNeighbors(vec3 position) {
    std::vector<int> neighbors;
    int cx = (int)(position[0] / cellSize);
    int cy = (int)(position[1] / cellSize);
    int cz = (int)(position[2] / cellSize);

    for (int x = cx - 1; x <= cx + 1; ++x) {
      for (int y = cy - 1; y <= cy + 1; ++y) {
        for (int z = cz - 1; z <= cz + 1; ++z) {
          long h1 = x * 73856093;
          long h2 = y * 19349663;
          long h3 = z * 83492791;

          long hash = (h1 ^ h2 ^ h3) % TABLE_SIZE;
          if (hash < 0)
            hash += TABLE_SIZE;
          int id = (int)hash;

          const auto &bucket = grid[id];
          neighbors.insert(neighbors.end(), bucket.begin(), bucket.end());
        }
      }
    }
    return neighbors;
  }

  // Helper to allow Engine to set cell size (h)
  void setCellSize(float h) { cellSize = h; }

private:
  static const int TABLE_SIZE = 10007; // Prime number for hashing
  float cellSize;
  std::vector<std::vector<int>> grid;

  int GetGridIndex(const vec3 position, float h) {
    int x = (int)(position[0] / h);
    int y = (int)(position[1] / h);
    int z = (int)(position[2] / h);

    // Large primes for hashing
    long h1 = x * 73856093;
    long h2 = y * 19349663;
    long h3 = z * 83492791;

    long hash = (h1 ^ h2 ^ h3) % TABLE_SIZE;
    if (hash < 0)
      hash += TABLE_SIZE;
    return (int)hash;
  }
};

#endif
