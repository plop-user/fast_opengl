#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct TransformParams {
  std::optional<glm::vec3> scale;
  std::optional<glm::vec3> position;
  std::optional<glm::vec3> rotation;
};

struct InstanceMaterial {
  // `-1` means "use the mesh part's default material texture/color".
  // Any non-negative value overrides all mesh-part diffuse textures on the
  // instance with a texture loaded through `loadTexture()`.
  int texture_layer = -1;
  glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
};

struct RenderInstance {
  glm::vec3 position{0.0f};
  glm::vec3 scale{1.0f};
  glm::vec3 rotation{0.0f};
  bool dirty = true;
};

class ObjRenderer {
public:
  struct RendererState;

  ObjRenderer();
  ~ObjRenderer();

  ObjRenderer(const ObjRenderer &) = delete;
  ObjRenderer &operator=(const ObjRenderer &) = delete;

  ObjRenderer(ObjRenderer &&) noexcept;
  ObjRenderer &operator=(ObjRenderer &&) noexcept;

  bool init();
  // Loads OBJ geometry. If the OBJ references an MTL, diffuse textures
  // declared with `map_Kd` are loaded automatically relative to the OBJ path.
  int loadMesh(const std::string &path);
  // Loads a standalone texture and returns an override texture id that can be
  // assigned to an instance through InstanceMaterial::texture_layer.
  int loadTexture(const std::string &path);
  void shutdown();
  void clearScene();

  // Passing `texture_layer = -1` keeps the mesh's own MTL-driven textures.
  // Passing a non-negative id forces that texture on the whole instance.
  int addModelInstance(int model_id, glm::vec3 position, glm::vec3 scale,
                       int texture_layer);
  int addModelInstance(int model_id, glm::vec3 position, glm::vec3 scale,
                       const InstanceMaterial &material);

  void draw(const glm::mat4 &view, const glm::mat4 &projection) const;
  void updateInstanceMatrices();
  // Retained for call-site compatibility. Instance data is now consumed
  // directly during draw, so this function is currently a no-op.
  void uploadInstanceData();

  void transformInstance(int model_id, int object_id,
                         const TransformParams &params);
  glm::vec3 translateInstance(int model_id, int object_id, glm::vec3 delta);
  glm::vec3 scaleInstance(int model_id, int object_id, glm::vec3 delta);
  glm::vec3 rotateInstance(int model_id, int object_id, glm::vec3 delta);

  void setInstanceMaterial(int model_id, int object_id,
                           const InstanceMaterial &material);
  InstanceMaterial getInstanceMaterial(int model_id, int object_id) const;

  RenderInstance &getInstance(int model_id, int object_id);
  const RenderInstance &getInstance(int model_id, int object_id) const;

  int objectCount(int model_id) const;
  int modelCount() const;

private:
  std::unique_ptr<RendererState> state_;
};
