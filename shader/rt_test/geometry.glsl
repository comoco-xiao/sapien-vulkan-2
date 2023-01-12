struct GeometryInstance {
  // mat4 transform;
  uint geometryIndex;
  uint materialIndex;
  int padding0;
  int padding1;
};

struct Material {
  vec4 emission;
  vec4 baseColor;
  float fresnel;
  float roughness;
  float metallic;
  float ior;
  float transmission;
  int textureMask;
  int padding0;
  int padding1;
};

struct TextureIndex {
  int diffuse;
  int metallic;
  int roughness;
  int emission;
  int normal;
  int occlusion;
  int padding0;
  int padding1;
};
