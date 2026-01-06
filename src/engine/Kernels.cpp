#include "Kernels.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Kernel {
float h = 1.0f;
float h2 = h * h;
float h9 = pow(h, 9);
float h6 = pow(h, 6);

// Poly6 Kernel (For Density)
float Poly6(float rSquared) {
  if (rSquared < 0 || rSquared > h2)
    return 0.0f;
  float diff = h2 - rSquared;
  return (315.0f / (64.0f * M_PI * h9)) * (diff * diff * diff);
}

// Spiky Kernel Gradient (For Pressure Force)
void SpikyGrad(vec3 rVector, float rLen, vec3 dest) {
  if (rLen <= 0 || rLen > h) {
    glm_vec3_zero(dest);
    return;
  }
  float diff = h - rLen;
  float coeff = -45.0f / (M_PI * h6);

  // Direction = Normalize(rVector)
  vec3 normalized;
  glm_vec3_copy(rVector, normalized);
  glm_vec3_normalize(normalized);

  // Result = normalized * (coeff * diff * diff)
  float scalar = coeff * diff * diff;
  glm_vec3_scale(normalized, scalar, dest);
}
} // namespace Kernel
