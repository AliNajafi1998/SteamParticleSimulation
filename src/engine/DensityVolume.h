#ifndef DENSITYVOLUME_H
#define DENSITYVOLUME_H

#include "SteamEngine.h" // For SteamParticle definiton if not separate.
#include <cglm/cglm.h>
#include <vector>
// Actually SteamEngine.h includes SteamParticle.h usually, or we assume
// SteamParticle is available.
#include "../particle/SteamParticle.h"

class DensityVolume {
public:
  DensityVolume(int width = 64, int height = 64, int depth = 64);
  ~DensityVolume();

  // Splat particles into the density grid
  void Build(const std::vector<SteamParticle> &particles);

  // Get dimensions
  void getParams(int *w, int *h, int *d) const;

  // Get raw data for texture upload (interleaved: density, temperature per voxel)
  const std::vector<float> &getData() const;

  void Clear(); // Moved to public

private:
  int width, height, depth;
  float cellWidth, cellHeight, cellDepth;
  std::vector<float> data; // 2 floats per voxel: [density, temperature]

  // Bounds of the volume (could be static or dynamic, for now static room
  // size?) Room is 30x30x30 often centered or on floor. Let's assume a fixed
  // bounding box for the detailed steam area for now, or matching the Room
  // size? Room is 30 wide, 30 deep, 30 high. Let's cover the Kurna area
  // specifically or the whole room? User wants "Volumetric Ray Marching",
  // covering the whole room is simpler for bounds.
  // Bounds (for coordinate mapping)
  // Bounds (for coordinate mapping)
  vec3 minBounds;
  vec3 maxBounds;
};

#endif
