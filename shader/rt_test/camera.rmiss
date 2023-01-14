#version 460
#extension GL_EXT_ray_tracing : require

#include "ray.glsl"
#include "push_constant.glsl"

layout(location=0) rayPayloadInEXT Ray ray;

layout(set = 1, binding = 10) uniform samplerCube samplerEnvironment;  // TODO: check this

void main() {
  vec3 dir = ray.direction;
  dir = vec3(-dir.y, dir.z, -dir.x);
  if (envmap != 0) {
    ray.radiance = texture(samplerEnvironment, dir).xyz;
  } else {
    ray.radiance = ambientLight;
  }

  ray.albedo = ray.radiance;
  ray.normal = vec3(0.0);

  ray.depth = maxDepth + 1;

  // ray.depth = maxDepth + 1;
  // ray.radiance = vec3(100000.0, 0.0, 100000.0);
  // ray.radiance = vec3(0.24740411, 0.62902375, 0.8324022);
  // ray.radiance = vec3(1.0);
  // ray.radiance = vec3(10.0);
}
