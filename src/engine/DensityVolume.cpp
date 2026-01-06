#include "DensityVolume.h"
#include <algorithm>
#include <cmath>
#include <iostream>

DensityVolume::DensityVolume(int w, int h, int d)
    : width(w), height(h), depth(d) {
  data.resize(width * height * depth, 0.0f);

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

  // simple point splatting
  // optimization: parallelize or use more complex kernel later
  for (const auto &p : particles) {
    if (!p.isActive())
      continue;

    // 1. Normalized Grid Coordinates (Float)
    float fx = (p.position[0] - minBounds[0]) / cellWidth;
    float fy = (p.position[1] - minBounds[1]) / cellHeight;
    float fz = (p.position[2] - minBounds[2]) / cellDepth;

    // 2. Base Index (Floor)
    int ix = (int)std::floor(fx - 0.5f);
    int iy = (int)std::floor(fy - 0.5f);
    int iz = (int)std::floor(fz - 0.5f);

    // 3. Fractional Offsets (Weights)
    float dx = fx - 0.5f - ix;
    float dy = fy - 0.5f - iy;
    float dz = fz - 0.5f - iz;

    // 4. Distribute to 8 neighbors
    for (int k = 0; k < 2; k++) {
      for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 2; i++) {

          int nx = ix + i;
          int ny = iy + j;
          int nz = iz + k;

          // Bounds check
          if (nx < 0 || nx >= width || ny < 0 || ny >= height || nz < 0 ||
              nz >= depth)
            continue;

          // Calculate weight (Trilinear)
          float weight = (i * dx + (1 - i) * (1 - dx)) *
                         (j * dy + (1 - j) * (1 - dy)) *
                         (k * dz + (1 - k) * (1 - dz));

          int idx = nz * width * height + ny * width + nx;
          data[idx] += 0.2f * weight; // [TUNED] Scaled amount for smoothness
        }
      }
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
