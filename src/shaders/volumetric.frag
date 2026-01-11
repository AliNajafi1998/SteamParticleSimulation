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
const int MAX_STEPS = 128;

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
    
    // Jitter start position to break up banding
    // [USER SUGGESTION] Random ray offset
    float randomOffset =  0; //fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
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

        // --- GREEN SAMPLE (Center) ---
        if (uvwG.x >= 0.0 && uvwG.x <= 1.0 && uvwG.y >= 0.0 && uvwG.y <= 1.0 && uvwG.z >= 0.0 && uvwG.z <= 1.0) {
            float d = texture(densityTex, uvwG).r;
            if (d > 0.01) {
                float n = fbm(currentPos * 1.5);
                float densityContribution = d * (n * 1.5) * stepSize;
                accumulatedDensity.g += densityContribution;
                
                // Hacky Integration of Light into Alpha
                // We really should accumulate Color and Alpha separately.
                // Let's assume white steam for now, dimmed by shadow.
                // We'll store the "Shadowed Density" in the Green channel for now to visualize it?
                // No, let's keep it simple first: just modify the alpha/density accumulation?
                // Actually, let's just use the shadow to darken the final output.
            }
        }
        
        currentPos += viewDir * stepSize;
        currentDist += stepSize;
        steps++;
        
        if (accumulatedDensity.g > 5.0) break;
    }
    
    // Beer's Law for each channel
    vec3 transmission = exp(-accumulatedDensity * 0.5);
    vec3 alpha = 1.0 - transmission;

    // Approximated Light Color (Global average for now, better would be per-pixel integration)
    // Since we didn't fully integrate light in the loop (to keep it simple in the snippet above),
    // let's do a trick: Darken the bottom/dense parts based on total density.
    float shadowFactor = exp(-accumulatedDensity.g * 0.8);
    
    // [TUNED] Make shadows brighter (0.8 floor) - Lighter Steam
    vec3 finalLight = vec3(1.0) * (shadowFactor * 0.2 + 0.8); 

    // Output
    // [TUNED] Color: Brighter White/Blueish
    vec3 steamColor = vec3(1.2, 1.25, 1.3); // Overdrive > 1.0 for "glowing" white feel
    FragColor = vec4(finalLight * steamColor, alpha.g); 
    
    // Mix dispersion
    vec3 dispersionColor = vec3(alpha.r, alpha.g, alpha.b);
    FragColor.rgb = mix(FragColor.rgb, dispersionColor, dispersionStrength * 5.0); // Boost dispersion visual
}
