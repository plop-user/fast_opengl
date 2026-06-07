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

  bool init(const std::vector<std::string> &obj_paths,
            const std::vector<std::string> &texture_paths);
  void shutdown();

  int addModelInstance(int model_id, glm::vec3 position, glm::vec3 scale,
                       int texture_layer);

  void draw(const glm::mat4 &view, const glm::mat4 &projection) const;
  void updateInstanceMatrices();
  void uploadInstanceData();

  void transformInstance(int model_id, int object_id,
                         const TransformParams &params);
  glm::vec3 translateInstance(int model_id, int object_id, glm::vec3 delta);
  glm::vec3 scaleInstance(int model_id, int object_id, glm::vec3 delta);
  glm::vec3 rotateInstance(int model_id, int object_id, glm::vec3 delta);

  RenderInstance &getInstance(int model_id, int object_id);
  const RenderInstance &getInstance(int model_id, int object_id) const;

  int objectCount(int model_id) const;
  int modelCount() const;

private:
  std::unique_ptr<RendererState> state_;
};
