#include "SteamEngine.h"
#include "Kernels.h"
#include <algorithm> // for std::max
#include <iostream>

SteamEngine::SteamEngine() {
  // Initialization
  // Gravity: Reduced from -9.8 to -0.5 to simulate air resistance/buoyancy
  gravity = -0.5f;
  // Buoyancy: Increased to ensure lift > gravity.
  // Acceleration = (Lift + Gravity) / Mass.
  // If Temp=1, Lift=4.0. Net Y Accel = 3.5.
  buoyancyCoeff = 4.0f;

  coolingRate =
      0.3f; // Slight reduction to let them rise higher before losing lift
  gasConstant = 2.0f;
  ambientTemperature = 0.0f;
}

SteamEngine::~SteamEngine() {
  // Cleanup if needed
}

void SteamEngine::Initialize(int maxParticles) {
  particlePool.clear();
  particlePool.resize(maxParticles); // Default constructor makes them inactive

  // Fill the free list
  deadParticleIndices.clear();
  deadParticleIndices.reserve(maxParticles);
  for (int i = 0; i < maxParticles; i++) {
    deadParticleIndices.push_back(i);
  }
}

void SteamEngine::Update(float deltaTime) {
  SpawnParticles(deltaTime);

  // SPH STEPS
  neighborGrid.Build(particlePool);
  CalculateDensityAndPressure();
  CalculateForces(); // Includes Gravity & Buoyancy
  Integrate(deltaTime);

  // STEAM LOGIC
  UpdateThermodynamics(deltaTime); // Cool down & Fade
}

const std::vector<SteamParticle> &SteamEngine::getParticles() const {
  return particlePool;
}

// A. Emission

// B. Density & Pressure Step
void SteamEngine::CalculateDensityAndPressure() {
  for (size_t i = 0; i < particlePool.size(); i++) {
    SteamParticle &p = particlePool[i];
    if (!p.active)
      continue;

    p.density = 0.0f;
    p.density += p.mass * Kernel::Poly6(0.0f);
    // 1. Get Neighbors
    std::vector<int> neighbors = neighborGrid.GetNeighbors(p.position);

    // 2. Sum Density
    for (int j : neighbors) {
      SteamParticle &n = particlePool[j];
      if (!n.active)
        continue; // Skip inactive neighbors too? usually yes.

      vec3 distVec;
      glm_vec3_sub(p.position, n.position, distVec);
      float r2 = glm_vec3_norm2(distVec);

      if (r2 < Kernel::h2) {
        // Density = Sum(Mass * Kernel)
        p.density += n.mass * Kernel::Poly6(r2);
      }
    }

    // 3. Compute Pressure (Ideal Gas Law: P = k * rho * T)
    p.density = std::max(p.density, 0.001f);
    p.pressure = gasConstant * p.density * p.temperature;
  }
}

// C. Force Accumulation
void SteamEngine::CalculateForces() {
  for (size_t i = 0; i < particlePool.size(); i++) {
    SteamParticle &p = particlePool[i];
    if (!p.active)
      continue;

    // Reset forces
    glm_vec3_zero(p.force);

    // 1. Gravity (Downwards)
    p.force[1] += gravity * p.mass;

    // 2. Buoyancy (Upwards based on Temperature)
    // Hotter particles rise faster.
    float lift = buoyancyCoeff * p.temperature;
    p.force[1] += lift;

    // 3. Pressure Force
    std::vector<int> neighbors = neighborGrid.GetNeighbors(p.position);
    for (int j : neighbors) {
      if (i == (size_t)j)
        continue;
      SteamParticle &n = particlePool[j];
      if (!n.active)
        continue;

      vec3 diff;
      glm_vec3_sub(p.position, n.position, diff);
      float r = glm_vec3_norm(diff);

      if (r < Kernel::h && r > 0.0001f) {
        vec3 gradW;
        Kernel::SpikyGrad(diff, r, gradW);

        // Symmetric Pressure Force
        // F = - m_i * m_j * (P_i/rho_i^2 + P_j/rho_j^2) * GradW
        float rho_i2 = p.density * p.density;
        float rho_j2 = n.density * n.density;
        float p_term = (p.pressure / rho_i2) + (n.pressure / rho_j2);

        vec3 forceP;
        float scalar = -p.mass * n.mass * p_term;
        glm_vec3_scale(gradW, scalar, forceP);
        glm_vec3_add(p.force, forceP, p.force);
      }
    }
  }
}

// D. Integration
void SteamEngine::Integrate(float deltaTime) {
  for (auto &p : particlePool) {
    if (!p.active)
      continue;

    // F = ma => a = F/m
    vec3 accel;
    glm_vec3_scale(p.force, 1.0f / p.mass, accel);

    // v += a * dt
    vec3 dv;
    glm_vec3_scale(accel, deltaTime, dv);
    glm_vec3_add(p.velocity, dv, p.velocity);

    // Damping/Drag
    glm_vec3_scale(p.velocity, 0.99f, p.velocity);

    // p += v * dt
    vec3 dx;
    glm_vec3_scale(p.velocity, deltaTime, dx);
    glm_vec3_add(p.position, dx, p.position);

    // Simple Floor Collision
    // Floor is at y = -15.0f
    if (p.position[1] < -15.0f) {
      p.position[1] = -15.0f;
      p.velocity[1] *= -0.5f;
    }
  }
}

// E. Thermodynamics & Death
void SteamEngine::UpdateThermodynamics(float deltaTime) {
  for (size_t i = 0; i < particlePool.size(); ++i) {
    SteamParticle &p = particlePool[i];
    if (!p.active)
      continue;

    // Cooling
    p.temperature -= coolingRate * deltaTime;
    if (p.temperature < 0.0f)
      p.temperature = 0.0f;

    // Aging
    p.life -= deltaTime;
    if (p.life <= 0.0f || p.temperature <= 0.05f) {
      p.active = false;
      // Return to free list
      deadParticleIndices.push_back((int)i);
    }
  }
}

// F. Spawning
void SteamEngine::SpawnParticles(float deltaTime) {
  static float timeAccumulator = 0.0f;
  timeAccumulator += deltaTime;

  float emissionRate = 200.0f; // Increased rate as optimization allows more
  float interval = 1.0f / emissionRate;

  while (timeAccumulator > interval) {
    timeAccumulator -= interval;

    if (!deadParticleIndices.empty()) {
      int idx = deadParticleIndices.back();
      deadParticleIndices.pop_back();

      SteamParticle &p = particlePool[idx];
      p = SteamParticle(); // Reset
      p.active = true;
      p.life = 10.0f;
      p.temperature = 1.0f;
      p.mass = 1.0f;

      // Random Position
      p.position[0] =
          ((rand() % 100) / 100.0f - 0.5f) * 1.5f; // Slightly wider spread
      p.position[1] =
          -14.0f; // Just above Kurna (Floor is -15, Kurna is 1 high)
      p.position[2] = ((rand() % 100) / 100.0f - 0.5f) * 1.5f;

      glm_vec3_zero(p.velocity);
      p.velocity[1] = 0.5f;
    }
  }
}
