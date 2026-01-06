#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 viewPos;
uniform vec3 boxMin; // e.g. -15, -15, -15
uniform vec3 boxMax; // e.g. 15, 15, 15
uniform sampler3D densityTex;

const float STEP_SIZE = 0.5; // Step size for ray marching
const int MAX_STEPS = 128;

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
    
    vec3 currentPos = viewPos + viewDir * tStart;
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
            
            float d = texture(densityTex, uvw).r;
            totalDensity += d * STEP_SIZE;
        }
        
        currentPos += viewDir * STEP_SIZE;
        currentDist += STEP_SIZE;
        steps++;
        
        // Early exit if saturated?
        if (totalDensity > 5.0) break;
    }
    
    float alpha = 1.0 - exp(-totalDensity * 0.5); // Beer's Law simple approx
    
    if (alpha < 0.01) discard;
    
    FragColor = vec4(1.0, 1.0, 1.0, alpha); // White steam
}
