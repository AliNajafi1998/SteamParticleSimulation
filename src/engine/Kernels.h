#ifndef KERNELS_H
#define KERNELS_H

#include <cglm/cglm.h>

namespace Kernel {
// Smoothing Radius Constants
extern float h;
extern float h2;
extern float h9;
extern float h6; // For SpikyGrad

// Poly6 Kernel (For Density)
float Poly6(float rSquared);

// Spiky Kernel Gradient (For Pressure Force)
// Writes result to 'dest'
void SpikyGrad(vec3 rVector, float rLen, vec3 dest);
} // namespace Kernel

#endif
