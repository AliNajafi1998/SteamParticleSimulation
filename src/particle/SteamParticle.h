#ifndef STEAMPARTICLE_H
#define STEAMPARTICLE_H

#include <cglm/cglm.h>

class SteamParticle {
public:
  SteamParticle(); // Default constructor
  SteamParticle(vec3 position, vec3 velocity, float mass, float density,
                float pressure, float temperature, float life = 1.0f);

  // Physical properties
  vec3 position;
  vec3 velocity;
  vec3 force;
  float mass;
  float density;
  float pressure;
  float temperature;
  float life;
  bool active;

  // Update method (placeholder for now)
  void update(float dt);

  // Getters and Setters can be added if stricter encapsulation is needed,
  // but for simulation performance, direct access is often preferred or inline
  // usage. We stick to direct access for now as per plan implicitly, or add
  // trivial ones if requested. Given the request asked for properties, public
  // members are simplest for a physics particle struct/class.

  // Getters
  void getPosition(vec3 dest) const;
  void getVelocity(vec3 dest) const;
  float getMass() const;
  float getDensity() const;
  float getPressure() const;
  float getTemperature() const;
  float getLife() const;
  bool isActive() const;

  // Setters
  void setPosition(vec3 p);
  void setVelocity(vec3 v);
  void setMass(float m);
  void setDensity(float d);
  void setPressure(float p);
  void setTemperature(float t);
  void setLife(float l);
  void setActive(bool a);
};

#endif
