#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 viewPos;
uniform vec3 boxMin; // e.g. -15, -15, -15
uniform vec3 boxMax; // e.g. 15, 15, 15
uniform sampler3D densityTex;

uniform float stepSize; // Step size for ray marching
// const float STEP_SIZE = 0.5; // [REPLACED]
uniform float dispersionStrength; // [NEW] Dispersion Strength
uniform vec3 lightPos; // [NEW] Light Position for Self Radiance
uniform int numSamples; // [NEW] Number of ray march samples for averaging
const int MAX_STEPS = 128;

// [NEW] Background sampling for wall visibility through fog
uniform sampler2D backgroundTex;
uniform vec2 viewportSize;
uniform mat4 view;
uniform mat4 projection;

// Room bounds (walls)
uniform vec3 roomMin; // e.g. -25, -15, -25
uniform vec3 roomMax; // e.g. 25, 15, 25

// [NEW] Refraction parameters
uniform float refractionStrength; // How much the fog bends light (index of refraction effect)
uniform float temperatureIORScale; // [NEW] How much temperature affects IOR (default ~0.5)
const float IOR_AIR = 1.0;

// [NEW] Base IOR for fog - will be modulated by temperature
// Hot steam has lower density = lower IOR, Cold steam has higher density = higher IOR
const float IOR_FOG_BASE = 1.02;

// [NEW] Chromatic dispersion - different wavelengths refract differently
// Based on Cauchy's equation: n(λ) = A + B/λ²
// Red light (longer wavelength) bends less, blue light (shorter wavelength) bends more
const vec3 IOR_FOG_CHROMATIC = vec3(1.018, 1.020, 1.024); // R, G, B - blue bends most

// [NEW] Function to calculate temperature-dependent IOR
// Hot steam (high temp) = lower IOR (less refraction)
// Cold steam (low temp) = higher IOR (more refraction)
float getTemperatureIOR(float baseIOR, float temperature, float strength) {
    // temperature is normalized 0-1 (0 = cold, 1 = hot)
    // Hot air/steam is less dense, so it has lower IOR
    float tempFactor = 1.0 - temperature * strength * 0.05;
    return baseIOR * tempFactor;
}

vec3 getTemperatureIORChromatic(vec3 baseIOR, float temperature, float strength) {
    float tempFactor = 1.0 - temperature * strength * 0.05;
    return baseIOR * tempFactor;
}

// --- NOISE FUNCTIONS ---
// Simple Hash
float hash(float n) { return fract(sin(n) * 43758.5453123); }

// 3D Noise (Simple Value Noise)
float noise(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    
    float n = p.x + p.y * 57.0 + p.z * 113.0;
    
    return mix(mix(mix( hash(n + 0.0), hash(n + 1.0), f.x),
                   mix( hash(n + 57.0), hash(n + 58.0), f.x), f.y),
               mix(mix( hash(n + 113.0), hash(n + 114.0), f.x),
                   mix( hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

// Fractal Brownian Motion
float fbm(vec3 x) {
    float v = 0.0;
    float a = 0.5;
    vec3 shift = vec3(100.0);
    for (int i = 0; i < 4; ++i) { // 4 Octaves
        v += a * noise(x);
        x = x * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

// Function to calculate ray-box intersection
// Returns distance to entry (tNear) and exit (tFar)
vec2 intersectBox(vec3 origin, vec3 dir) {
    vec3 tMin = (boxMin - origin) / dir;
    vec3 tMax = (boxMax - origin) / dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

// [NEW] Intersect with room walls (for sampling background behind fog)
vec2 intersectRoom(vec3 origin, vec3 dir) {
    vec3 tMin = (roomMin - origin) / dir;
    vec3 tMax = (roomMax - origin) / dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

// [NEW] Sample background texture at a world position
vec3 sampleBackground(vec3 worldPos) {
    // Project world position to screen space
    vec4 clipPos = projection * view * vec4(worldPos, 1.0);
    vec3 ndc = clipPos.xyz / clipPos.w;
    vec2 screenUV = ndc.xy * 0.5 + 0.5;
    
    // Clamp to valid range
    screenUV = clamp(screenUV, 0.0, 1.0);
    
    return texture(backgroundTex, screenUV).rgb;
}

// [NEW] Compute density gradient at a position (for refraction normal)
vec3 getDensityGradient(vec3 pos, float epsilon) {
    vec3 uvw = (pos - boxMin) / (boxMax - boxMin);
    
    // Sample density at offset positions to compute gradient
    // Texture now has 2 channels: R = density, G = temperature
    float dx = texture(densityTex, uvw + vec3(epsilon, 0.0, 0.0)).r 
             - texture(densityTex, uvw - vec3(epsilon, 0.0, 0.0)).r;
    float dy = texture(densityTex, uvw + vec3(0.0, epsilon, 0.0)).r 
             - texture(densityTex, uvw - vec3(0.0, epsilon, 0.0)).r;
    float dz = texture(densityTex, uvw + vec3(0.0, 0.0, epsilon)).r 
             - texture(densityTex, uvw - vec3(0.0, 0.0, epsilon)).r;
    
    return vec3(dx, dy, dz);
}

// [NEW] Sample density and temperature from the 3D texture
// Returns vec2(density, temperature)
vec2 sampleDensityAndTemp(vec3 pos) {
    vec3 uvw = (pos - boxMin) / (boxMax - boxMin);
    if (uvw.x < 0.0 || uvw.x > 1.0 || uvw.y < 0.0 || uvw.y > 1.0 || uvw.z < 0.0 || uvw.z > 1.0) {
        return vec2(0.0);
    }
    vec2 sample = texture(densityTex, uvw).rg;
    return sample; // r = density, g = temperature (normalized 0-1)
}

// [NEW] Refract ray direction based on density gradient
vec3 refractRay(vec3 rayDir, vec3 normal, float eta) {
    float cosI = -dot(normal, rayDir);
    float sinT2 = eta * eta * (1.0 - cosI * cosI);
    
    if (sinT2 > 1.0) {
        // Total internal reflection - just reflect
        return reflect(rayDir, normal);
    }
    
    float cosT = sqrt(1.0 - sinT2);
    return eta * rayDir + (eta * cosI - cosT) * normal;
}

void main()
{        
    vec3 viewDir = normalize(FragPos - viewPos);
    
    // Check intersection with the bounding box
    // Camera is outside, so usually tNear is the entry point
    // If inside, tNear would be negative, so we start from 0
    // Actually our cube geometry IS the bounding box essentially, but let's be safe.
    // If we render a cube, FragPos is on the surface.
    // So the ray starts at viewPos, hits FragPos. 
    // We should march defined by the volume limits.
    
    // Let's use the slab method to find entry/exit logic for volume.
    // Ray origin = viewPos, Dir = viewDir.
    
    vec2 t = intersectBox(viewPos, viewDir);
    
    if (t.x > t.y || t.y < 0.0) discard; // No intersection or box entirely behind camera
    
    // Start marching from entry point (or camera position if inside)
    // When camera is inside the volume, t.x is negative, so clamp to 0
    float tStart = max(t.x, 0.0);
    float tEnd = t.y;
    
    // [NEW] Multi-sample averaging accumulators
    vec3 totalAccumulatedDensity = vec3(0.0);
    vec3 totalAccumulatedRefractionR = vec3(0.0);
    vec3 totalAccumulatedRefractionG = vec3(0.0);
    vec3 totalAccumulatedRefractionB = vec3(0.0);
    vec3 totalTotalColor = vec3(0.0);
    
    int actualSamples = max(numSamples, 1);
    
    for (int sampleIdx = 0; sampleIdx < actualSamples; sampleIdx++) {
        // Jitter start position to break up banding
        // [USER SUGGESTION] Random ray offset - different per sample
        float randomOffset = fract(sin(dot(gl_FragCoord.xy + float(sampleIdx) * 17.31, vec2(12.9898, 78.233))) * 43758.5453);
        vec3 currentPos = viewPos + viewDir * (tStart + stepSize * randomOffset);
        float currentDist = tStart;
        
        // [NEW] Chromatic Dispersion
        // Instead of one density, we accumulate a spectral color.
        vec3 totalColor = vec3(0.0);
        vec3 totalAlpha = vec3(0.0); // Transmissivity per channel? 
        // Simplified: Just accumulate density per channel.
        vec3 accumulatedDensity = vec3(0.0);
        
        // Dispersion factor: How much the channels separate
        // We can use stepSize to keep it relative to integration
        float dispersion = dispersionStrength; 
        
        // [NEW] Accumulated refraction - tracks how much the ray has bent
        vec3 accumulatedRefraction = vec3(0.0);
        vec3 currentRayDir = viewDir; // Ray direction that gets refracted as we march
        
        // [NEW] Chromatic dispersion - separate ray directions per color channel
        vec3 currentRayDirR = viewDir; // Red ray (bends least)
        vec3 currentRayDirG = viewDir; // Green ray (reference)
        vec3 currentRayDirB = viewDir; // Blue ray (bends most)
        vec3 accumulatedRefractionR = vec3(0.0);
        vec3 accumulatedRefractionG = vec3(0.0);
        vec3 accumulatedRefractionB = vec3(0.0);
    
        int steps = 0;
        while (currentDist < tEnd && steps < MAX_STEPS) {
        // Map position to UVW [0, 1]
        // Center (Green)
        vec3 uvwG = (currentPos - boxMin) / (boxMax - boxMin);
        
        // Red (Offset forward/outward)
        vec3 posR = currentPos + viewDir * dispersion;
        vec3 uvwR = (posR - boxMin) / (boxMax - boxMin);
        
        // Blue (Offset backward/inward)
        vec3 posB = currentPos - viewDir * dispersion;
        vec3 uvwB = (posB - boxMin) / (boxMax - boxMin);
        
        // --- LIGHT MARCHING (Self-Shadowing) ---
        // For the center sample (Green/Primary), calculate how much light reaches this point.
        if (accumulatedDensity.g < 5.0) { // Optimization: Don't light if already opaque
             vec3 lightDir = normalize(lightPos - currentPos); 
             // vec3 lightPos = currentPos + lightDir * 0.1; // Rename local variable to avoid conflict
             vec3 shadowRayPos = currentPos + lightDir * 0.1;
             
             float lightDensity = 0.0;
             
             // March towards light
             for (int l = 0; l < 4; l++) { 
                 vec3 lUVW = (shadowRayPos - boxMin) / (boxMax - boxMin);
                 if(lUVW.x >= 0.0 && lUVW.x <= 1.0 && lUVW.y >= 0.0 && lUVW.y <= 1.0 && lUVW.z >= 0.0 && lUVW.z <= 1.0) {
                     float ld = texture(densityTex, lUVW).r;
                     lightDensity += ld * 0.5; 
                 }
                 shadowRayPos += lightDir * 0.5; 
             }
             
             float lightTransmittance = exp(-lightDensity * 2.0); // Darkness multiplier
             
             // Tweak: Add ambient light so it's not pitch black
             vec3 lightColor = vec3(1.0) * (lightTransmittance * 0.7 + 0.3);
             
             // Accumulate this light into the color
             // Simple interaction: Density * Light
             // Note: This is an approximation. True scattering is complex.
             // We apply this lighting factor to the density we just accumulated.
             
             // Re-weighting the color accumulation based on light
             totalColor += accumulatedDensity * stepSize * lightColor; 
        }

        // --- RED SAMPLE (Offset forward) ---
        if (uvwR.x >= 0.0 && uvwR.x <= 1.0 && uvwR.y >= 0.0 && uvwR.y <= 1.0 && uvwR.z >= 0.0 && uvwR.z <= 1.0) {
            vec2 sampleR = texture(densityTex, uvwR).rg; // r = density, g = temperature
            float dR = sampleR.r;
            float tempR = sampleR.g;
            if (dR > 0.01) {
                float nR = fbm(posR * 1.5);
                float densityContributionR = dR * (nR * 1.5) * stepSize;
                accumulatedDensity.r += densityContributionR;
                
                // Compute refraction for red channel with temperature-dependent IOR
                vec3 gradientR = getDensityGradient(posR, 0.02);
                float gradientMagR = length(gradientR);
                
                if (gradientMagR > 0.001) {
                    vec3 normalR = normalize(gradientR);
                    // Apply temperature to IOR: hot steam = lower IOR, cold steam = higher IOR
                    float tempIOR_R = getTemperatureIOR(IOR_FOG_CHROMATIC.r, tempR, temperatureIORScale);
                    float etaR = IOR_AIR / tempIOR_R * (1.0 + dispersionStrength);
                    vec3 refractedDirR = refractRay(currentRayDirR, normalR, etaR);
                    
                    float refractionWeightR = dR * refractionStrength * stepSize;
                    accumulatedRefractionR += (refractedDirR - currentRayDirR) * refractionWeightR;
                    
                    float bendFactor = 0.1 * (1.0 + dispersionStrength);
                    currentRayDirR = normalize(currentRayDirR + (refractedDirR - currentRayDirR) * refractionWeightR * bendFactor);
                }
            }
        }

        // --- GREEN SAMPLE (Center) ---
        if (uvwG.x >= 0.0 && uvwG.x <= 1.0 && uvwG.y >= 0.0 && uvwG.y <= 1.0 && uvwG.z >= 0.0 && uvwG.z <= 1.0) {
            vec2 sampleG = texture(densityTex, uvwG).rg; // r = density, g = temperature
            float dG = sampleG.r;
            float tempG = sampleG.g;
            if (dG > 0.01) {
                float nG = fbm(currentPos * 1.5);
                float densityContributionG = dG * (nG * 1.5) * stepSize;
                accumulatedDensity.g += densityContributionG;
                
                // Compute refraction for green channel with temperature-dependent IOR
                vec3 gradientG = getDensityGradient(currentPos, 0.02);
                float gradientMagG = length(gradientG);
                
                if (gradientMagG > 0.001) {
                    vec3 normalG = normalize(gradientG);
                    // Apply temperature to IOR: hot steam = lower IOR, cold steam = higher IOR
                    float tempIOR_G = getTemperatureIOR(IOR_FOG_CHROMATIC.g, tempG, temperatureIORScale);
                    float etaG = IOR_AIR / tempIOR_G * (1.0 + dispersionStrength);
                    vec3 refractedDirG = refractRay(currentRayDirG, normalG, etaG);
                    
                    float refractionWeightG = dG * refractionStrength * stepSize;
                    accumulatedRefractionG += (refractedDirG - currentRayDirG) * refractionWeightG;
                    
                    float bendFactor = 0.1 * (1.0 + dispersionStrength);
                    currentRayDirG = normalize(currentRayDirG + (refractedDirG - currentRayDirG) * refractionWeightG * bendFactor);
                    
                    // Keep the main ray direction as green (reference)
                    currentRayDir = currentRayDirG;
                    accumulatedRefraction = accumulatedRefractionG;
                }
            }
        }

        // --- BLUE SAMPLE (Offset backward) ---
        if (uvwB.x >= 0.0 && uvwB.x <= 1.0 && uvwB.y >= 0.0 && uvwB.y <= 1.0 && uvwB.z >= 0.0 && uvwB.z <= 1.0) {
            vec2 sampleB = texture(densityTex, uvwB).rg; // r = density, g = temperature
            float dB = sampleB.r;
            float tempB = sampleB.g;
            if (dB > 0.01) {
                float nB = fbm(posB * 1.5);
                float densityContributionB = dB * (nB * 1.5) * stepSize;
                accumulatedDensity.b += densityContributionB;
                
                // Compute refraction for blue channel with temperature-dependent IOR
                vec3 gradientB = getDensityGradient(posB, 0.02);
                float gradientMagB = length(gradientB);
                
                if (gradientMagB > 0.001) {
                    vec3 normalB = normalize(gradientB);
                    // Apply temperature to IOR: hot steam = lower IOR, cold steam = higher IOR
                    float tempIOR_B = getTemperatureIOR(IOR_FOG_CHROMATIC.b, tempB, temperatureIORScale);
                    float etaB = IOR_AIR / tempIOR_B * (1.0 + dispersionStrength);
                    vec3 refractedDirB = refractRay(currentRayDirB, normalB, etaB);
                    
                    float refractionWeightB = dB * refractionStrength * stepSize;
                    accumulatedRefractionB += (refractedDirB - currentRayDirB) * refractionWeightB;
                    
                    float bendFactor = 0.1 * (1.0 + dispersionStrength);
                    currentRayDirB = normalize(currentRayDirB + (refractedDirB - currentRayDirB) * refractionWeightB * bendFactor);
                }
            }
        }
        
        currentPos += currentRayDir * stepSize; // Use potentially refracted direction
        currentDist += stepSize;
        steps++;
        
        // Early exit if all channels are saturated
        if (accumulatedDensity.r > 5.0 && accumulatedDensity.g > 5.0 && accumulatedDensity.b > 5.0) break;
    }
    
        // Accumulate this sample's results
        totalAccumulatedDensity += accumulatedDensity;
        totalAccumulatedRefractionR += accumulatedRefractionR;
        totalAccumulatedRefractionG += accumulatedRefractionG;
        totalAccumulatedRefractionB += accumulatedRefractionB;
        totalTotalColor += totalColor;
    } // End of sample loop
    
    // Average over all samples
    float invSamples = 1.0 / float(actualSamples);
    vec3 avgAccumulatedDensity = totalAccumulatedDensity * invSamples;
    vec3 avgAccumulatedRefractionR = totalAccumulatedRefractionR * invSamples;
    vec3 avgAccumulatedRefractionG = totalAccumulatedRefractionG * invSamples;
    vec3 avgAccumulatedRefractionB = totalAccumulatedRefractionB * invSamples;
    
    // Beer's Law for each channel
    vec3 transmission = exp(-avgAccumulatedDensity * 0.5);
    vec3 alpha = 1.0 - transmission;

    // Approximated Light Color (Global average for now, better would be per-pixel integration)
    // Since we didn't fully integrate light in the loop (to keep it simple in the snippet above),
    // let's do a trick: Darken the bottom/dense parts based on total density.
    float shadowFactor = exp(-avgAccumulatedDensity.g * 0.8);
    
    // [TUNED] Make shadows brighter (0.8 floor) - Lighter Steam
    vec3 finalLight = vec3(1.0) * (shadowFactor * 0.2 + 0.8); 

    // [NEW] Sample background/walls where the ray exits the fog volume
    // Find where the ray hits the room walls
    vec2 roomT = intersectRoom(viewPos, viewDir);
    float wallDist = roomT.y; // Distance to far wall
    
    // If the ray exits the fog volume before hitting walls, sample at exit point
    // Otherwise sample at wall hit point
    float sampleDist = max(tEnd, 0.0);
    if (wallDist > 0.0 && wallDist > tEnd) {
        sampleDist = wallDist;
    }
    
    // [NEW] Calculate refracted hit position
    // The ray has been bent by the fog, so we sample from a displaced position
    vec3 hitPos = viewPos + viewDir * sampleDist;
    
    // [NEW] Per-channel refracted positions for chromatic dispersion
    vec3 refractedHitPosR = hitPos + avgAccumulatedRefractionR * sampleDist * 2.0;
    vec3 refractedHitPosG = hitPos + avgAccumulatedRefractionG * sampleDist * 2.0;
    vec3 refractedHitPosB = hitPos + avgAccumulatedRefractionB * sampleDist * 2.0;
    
    // Sample background at the refracted position
    vec2 screenUV = gl_FragCoord.xy / viewportSize;
    
    // [NEW] Calculate per-channel refracted UV offsets for chromatic dispersion
    vec4 originalClip = projection * view * vec4(hitPos, 1.0);
    vec4 refractedClipR = projection * view * vec4(refractedHitPosR, 1.0);
    vec4 refractedClipG = projection * view * vec4(refractedHitPosG, 1.0);
    vec4 refractedClipB = projection * view * vec4(refractedHitPosB, 1.0);
    
    vec2 originalNDC = originalClip.xy / originalClip.w;
    vec2 refractedNDC_R = refractedClipR.xy / refractedClipR.w;
    vec2 refractedNDC_G = refractedClipG.xy / refractedClipG.w;
    vec2 refractedNDC_B = refractedClipB.xy / refractedClipB.w;
    
    // Calculate per-channel UV offsets from dispersion
    vec2 refractionUVOffsetR = (refractedNDC_R - originalNDC) * 0.5;
    vec2 refractionUVOffsetG = (refractedNDC_G - originalNDC) * 0.5;
    vec2 refractionUVOffsetB = (refractedNDC_B - originalNDC) * 0.5;
    
    // Apply chromatic dispersion - additional separation based on dispersionStrength
    float chromaticSpread = dispersionStrength * 0.02;
    vec2 uvR = screenUV + refractionUVOffsetR * (1.0 + chromaticSpread);
    vec2 uvG = screenUV + refractionUVOffsetG;
    vec2 uvB = screenUV + refractionUVOffsetB * (1.0 - chromaticSpread);
    
    // Clamp UVs to valid range
    uvR = clamp(uvR, 0.0, 1.0);
    uvG = clamp(uvG, 0.0, 1.0);
    uvB = clamp(uvB, 0.0, 1.0);
    
    // Sample background with chromatic dispersion
    vec3 backgroundColor;
    backgroundColor.r = texture(backgroundTex, uvR).r;
    backgroundColor.g = texture(backgroundTex, uvG).g;
    backgroundColor.b = texture(backgroundTex, uvB).b;

    // Output
    // [TUNED] Color: Brighter White/Blueish
    vec3 steamColor = vec3(1.2, 1.25, 1.3); // Overdrive > 1.0 for "glowing" white feel
    vec3 fogColor = finalLight * steamColor;
    
    // Mix dispersion into fog color
    vec3 dispersionColor = vec3(alpha.r, alpha.g, alpha.b);
    fogColor = mix(fogColor, dispersionColor, dispersionStrength * 5.0); // Boost dispersion visual
    
    // [NEW] Blend fog with background based on transmittance
    // transmittance = how much background shows through
    // alpha = how much fog occludes
    float fogAlpha = alpha.g;
    vec3 finalColor = mix(backgroundColor, fogColor, fogAlpha);
    
    FragColor = vec4(finalColor, 1.0); // Output as opaque since we've already blended
}
