// Vector Operations:
// Reflect a direction vector about a normal vector.
float3 reflect(float3 direction, float3 normal)
{
    return normalize(direction - 2 * dot(direction, normal) * normal);
}

// Bump Map:
// Given a TBN (Tangent, Binormal, Normal matrix) anda bump map
// RGB value, returns the corresponding normal. 
float3 bump_to_normal(float3 bump, float3x3 tbn)
{
    float3 bump_normal = (bump - float3(0.5f, 0.5f, 0.5f)) * 2.f;
    // Note: BumpMapBuilder assumes that y coordinate is "up", which corresponds to the normal vector
    // on the surface.
    float3 normal = bump_normal.x * tbn[0] + bump_normal.z * tbn[1] + bump_normal.y * tbn[2];
    return normalize(normal);
}

// Ray Operations:
// Returns t, where t is the intersection distance of the ray and a plane. 
// If there is no intersection, returns -1.
// Assumes that ray_direction and plane_normal are normalized.
float ray_plane_intersection(float3 ray_origin, float3 ray_direc, float3 plane_origin, float3 plane_normal)
{
    float denom = dot(ray_direc, plane_normal);
    float t = -1.f;
    
    if (abs(denom) >= 0.001f)
    {
        t = (dot(plane_normal, plane_origin) - dot(ray_origin, plane_normal)) / denom;
    }
        
    return t;
}

// Computes and returns t, where the ray intersects the sphere. In the case of multiple
// intersection points, chooses the smaller of the two positive t's.
// Returns 0 if there is no intersection.
float ray_sphere_intersection(float3 ray_origin, float3 ray_direction, float3 sphere_origin, float sphere_radius)
{
    float3 sphere_to_ray = ray_origin - sphere_origin;
    
    // Solving for the ray-sphere intersection gives us a quadratic equation to solve.
    // We solve for the roots.
    float a = dot(sphere_to_ray, sphere_to_ray);
    float b = 2 * dot(sphere_to_ray, ray_direction);
    float c = dot(ray_direction, ray_direction) - sphere_radius * sphere_radius;
    
    // Use the quadratic formula to solve for the roots. Depending on the value of delta, we know
    // how many roots (intersection points) we have.
    float delta = b * b - 4 * a * c;
    
    // If delta < 0, we have no intersection
    float result = -1.f;
    
    if (delta == 0)
    {
        result = -b / (2 * a);
    }
    else if (delta > 0)
    {
        float sqrt_delta = sqrt(delta);
        float t1 = (-b + sqrt_delta) / (2 * a);
        float t2 = (-b - sqrt_delta) / (2 * a);
        result = max(t1, t2);
    }
    
    return result;
}