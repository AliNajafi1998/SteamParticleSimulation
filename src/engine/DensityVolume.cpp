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

    // Map position to grid index
    // Relative pos
    float rx = p.position[0] - minBounds[0];
    float ry = p.position[1] - minBounds[1];
    float rz = p.position[2] - minBounds[2];

    // integer init coords
    int ix = (int)(rx / cellWidth);
    int iy = (int)(ry / cellHeight);
    int iz = (int)(rz / cellDepth);

    // Bounds check
    if (ix < 0 || ix >= width || iy < 0 || iy >= height || iz < 0 ||
        iz >= depth)
      continue;

    // Linear index
    int index = iz * width * height + iy * width + ix;

    // Add density - could weigh by particle mass or density property
    // data[index] += p.density; // Or just const amount
    data[index] += 0.5f; // Accumulate basic density
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
