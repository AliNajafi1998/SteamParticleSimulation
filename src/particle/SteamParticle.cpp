#include "SteamParticle.h"
#include <cstring> // for memcpy if needed, though cglm handles vectors

// Default constructor: creates an inactive particle
SteamParticle::SteamParticle()
    : mass(1.0f), density(0.0f), pressure(0.0f), temperature(20.0f), life(0.0f),
      active(false) {
  glm_vec3_zero(this->position);
  glm_vec3_zero(this->velocity);
  glm_vec3_zero(this->angularVelocity);
  this->currentAngle = 0.0f;
}

SteamParticle::SteamParticle(vec3 pos, vec3 vel, float m, float d, float p,
                             float t, float l)
    : mass(m), density(d), pressure(p), temperature(t), life(l), active(true) {
  glm_vec3_copy(pos, this->position);
  glm_vec3_copy(vel, this->velocity);
  glm_vec3_zero(this->angularVelocity);
  this->currentAngle = 0.0f;
}

void SteamParticle::update(float dt) {
  // Placeholder for physics update: e.g., position += velocity * dt
  // For now, empty as requested to just create the class.
  // Example basic Euler integration:
  // glm_vec3_muladds(velocity, dt, position);
  (void)dt; // Suppress unused parameter warning
}

void SteamParticle::getPosition(vec3 dest) const {
  glm_vec3_copy(const_cast<float *>(this->position), dest);
}

void SteamParticle::getVelocity(vec3 dest) const {
  glm_vec3_copy(const_cast<float *>(this->velocity), dest);
}

float SteamParticle::getMass() const { return mass; }

float SteamParticle::getDensity() const { return density; }

float SteamParticle::getPressure() const { return pressure; }

float SteamParticle::getTemperature() const { return temperature; }

float SteamParticle::getLife() const { return life; }

bool SteamParticle::isActive() const { return active; }

void SteamParticle::setPosition(vec3 p) { glm_vec3_copy(p, this->position); }

void SteamParticle::setVelocity(vec3 v) { glm_vec3_copy(v, this->velocity); }

void SteamParticle::setMass(float m) { mass = m; }

void SteamParticle::setDensity(float d) { density = d; }

void SteamParticle::setPressure(float p) { pressure = p; }

void SteamParticle::setTemperature(float t) { temperature = t; }

void SteamParticle::setLife(float l) { life = l; }

void SteamParticle::setActive(bool a) { active = a; }
