#include "DensityVolume.h"
#include <algorithm>
#include <cmath>
#include <iostream>

DensityVolume::DensityVolume(int w, int h, int d)
    : width(w), height(h), depth(d) {
  // 2 floats per voxel: density (R) and temperature (G)
  data.resize(width * height * depth * 2, 0.0f);

  // Define bounds - covering the typical play area
  // Room is approx -15 to +15 in X and Z, and -15 to +15 in Y (since height is
  // 30, centered?) Let's check Main.cpp or Room.cpp for exact coords.
  // Room(30,30,30) usually means centered at 0 or 0 is floor?
  // Kurna is at y = -15.
  // So bounds likely -15 to +15 on all axes if centered at 0?
  // Let's assume Room 30x30x30 centered gives [-15, 15].
  // We can fine tune this.
  minBounds[0] = -15.0f;
  minBounds[1] = -15.0f;
  minBounds[2] = -15.0f;
  maxBounds[0] = 15.0f;
  maxBounds[1] = 15.0f;
  maxBounds[2] = 15.0f;

  cellWidth = (maxBounds[0] - minBounds[0]) / (float)width;
  cellHeight = (maxBounds[1] - minBounds[1]) / (float)height;
  cellDepth = (maxBounds[2] - minBounds[2]) / (float)depth;
}

DensityVolume::~DensityVolume() {}

void DensityVolume::Clear() { std::fill(data.begin(), data.end(), 0.0f); }

void DensityVolume::Build(const std::vector<SteamParticle> &particles) {
  Clear();

  // We need to track weight sums for proper temperature averaging
  std::vector<float> weightSums(width * height * depth, 0.0f);

  // simple point splatting
  // optimization: parallelize or use more complex kernel later
  for (const auto &p : particles) {
    if (!p.isActive())
      continue;

    // Get particle temperature and normalize it
    // Assume temperature range is roughly 0-100 degrees
    float particleTemp = p.getTemperature();
    float normalizedTemp = std::max(0.0f, std::min(particleTemp / 100.0f, 1.0f));

    // 1. Normalized Grid Coordinates (Float)
    float fx = (p.position[0] - minBounds[0]) / cellWidth;
    float fy = (p.position[1] - minBounds[1]) / cellHeight;
    float fz = (p.position[2] - minBounds[2]) / cellDepth;

    // [NEW] Gaussian Splatting (3x3x3)
    // Center voxel indices (nearest integer)
    int cx = (int)(fx + 0.5f);
    int cy = (int)(fy + 0.5f);
    int cz = (int)(fz + 0.5f);

    // Loop over 3x3x3 block
    for (int k = -1; k <= 1; k++) {
      for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {

          int nx = cx + i;
          int ny = cy + j;
          int nz = cz + k;

          // Bounds check
          if (nx < 0 || nx >= width || ny < 0 || ny >= height || nz < 0 ||
              nz >= depth)
            continue;

          // Squared distance from particle (fx, fy, fz) to voxel center (nx,
          // ny, nz)
          float dx = nx - fx;
          float dy = ny - fy;
          float dz = nz - fz;
          float distSq = dx * dx + dy * dy + dz * dz;

          // Gaussian Falloff
          // Sigma controls the "blobbiness".
          // 1.0 means falloff is gradual over 1 cell.
          float sigma = 1.0f;
          float weight = std::exp(-distSq / (2.0f * sigma * sigma));

          int voxelIdx = nz * width * height + ny * width + nx;
          int dataIdx = voxelIdx * 2; // 2 floats per voxel
          
          data[dataIdx] += 0.4f * weight; // Density (R channel)
          data[dataIdx + 1] += normalizedTemp * weight; // Weighted temperature (G channel)
          weightSums[voxelIdx] += weight; // Track weight for averaging
        }
      }
    }
  }

  // Normalize temperature by weight sum to get proper average
  for (int i = 0; i < width * height * depth; i++) {
    if (weightSums[i] > 0.001f) {
      data[i * 2 + 1] /= weightSums[i];
    }
  }
}

void DensityVolume::getParams(int *w, int *h, int *d) const {
  if (w)
    *w = width;
  if (h)
    *h = height;
  if (d)
    *d = depth;
}

const std::vector<float> &DensityVolume::getData() const { return data; }
