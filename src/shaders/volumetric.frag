#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 viewPos;
uniform vec3 boxMin; // e.g. -15, -15, -15
uniform vec3 boxMax; // e.g. 15, 15, 15
uniform sampler3D densityTex;

uniform float stepSize; // Step size for ray marching
// const float STEP_SIZE = 0.5; // [REPLACED]
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
    
    if (t.x > t.y) discard; // No intersection
    
    // Start marching from entry point (or camera position if inside)
    float tStart = max(t.x, 0.0);
    float tEnd = t.y;
    
    // Jitter start position to break up banding
    // [USER SUGGESTION] Random ray offset
    float randomOffset = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    vec3 currentPos = viewPos + viewDir * (tStart + stepSize * randomOffset);
    float currentDist = tStart;
    
    float totalDensity = 0.0;
    
    int steps = 0;
    while (currentDist < tEnd && steps < MAX_STEPS) {
        // Map position to UVW [0, 1]
        vec3 uvw = (currentPos - boxMin) / (boxMax - boxMin);
        
        // Sampling
        if(uvw.x >= 0.0 && uvw.x <= 1.0 && 
           uvw.y >= 0.0 && uvw.y <= 1.0 && 
           uvw.z >= 0.0 && uvw.z <= 1.0) {
            
            float baseDensity = texture(densityTex, uvw).r;
            
            if (baseDensity > 0.01) {
                // Apply FBM Noise to erode/detail the density
                // Higher frequency (e.g. 1.5) provides structure
                float noiseVal = fbm(currentPos * 1.5); 
                
                // Modulate: 
                // We want to carve out shapes, so multiply by noise.
                // We can also bias it so thick parts stay thick.
                float detailDensity = baseDensity * (noiseVal * 1.5); 
                
                totalDensity += detailDensity * stepSize;
            }
        }
        
        currentPos += viewDir * stepSize;
        currentDist += stepSize;
        steps++;
        
        // Early exit if saturated?
        if (totalDensity > 5.0) break;
    }
    
    float alpha = 1.0 - exp(-totalDensity * 0.5); // Beer's Law simple approx
    
    if (alpha < 0.01) discard;
    
    FragColor = vec4(1.0, 1.0, 1.0, alpha); // White steam
}
