#ifndef STEAMENGINE_H
#define STEAMENGINE_H

#include "../particle/SteamParticle.h"
#include "SpatialGrid.h"
#include <vector>

class SteamEngine {
public:
  SteamEngine();
  ~SteamEngine();

  // Initialization
  void Initialize(int maxParticles);

  // Main update loop
  void Update(float deltaTime);

  // Rendering Interface
  const std::vector<SteamParticle> &getParticles() const;

private:
  // std::vector<SteamParticle> particles; // Replacing with particlePool

  // Simulation Steps
  // Simulation Steps
  void SpawnParticles(float deltaTime);       // A. Emission
  void CalculateDensityAndPressure();         // B. Density & Pressure
  void CalculateVorticity();                  // [NEW] Vorticity Calculation
  void CalculateForces();                     // C. Force Accumulation
  void Integrate(float deltaTime);            // D. Integration
  void UpdateThermodynamics(float deltaTime); // E. Thermodynamics & Death

  // SETTINGS (Public for UI)
public:
  float gravity;
  float buoyancyCoeff;         // How much it rises
  float coolingRate;           // How fast it fades
  float gasConstant;           // How hard it expands
  float ambientTemperature;    // Temperature where lift stops
  float emissionRate = 200.0f; // Added default

private:
  // MEMORY
  std::vector<SteamParticle> particlePool;
  std::vector<int> deadParticleIndices; // Free list for O(1) spawning
  SpatialGrid neighborGrid;             // Helper for fast lookups
};

#endif
