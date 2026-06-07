#include <obj.h>

#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glslread.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace {

struct Vertex {
  glm::vec3 pos;
  glm::vec2 uv;

  bool operator==(const Vertex &other) const {
    return pos == other.pos && uv == other.uv;
  }
};

struct VertexHash {
  size_t operator()(const Vertex &vertex) const {
    return ((std::hash<float>()(vertex.pos.x) ^
             (std::hash<float>()(vertex.pos.y) << 1)) >>
            1) ^
           (std::hash<float>()(vertex.pos.z) << 1) ^
           (std::hash<float>()(vertex.uv.x) << 1) ^
           (std::hash<float>()(vertex.uv.y) << 1);
  }
};

GLuint compileShader(const std::string &path, GLenum type) {
  const std::string source = readFile(path);
  if (source.empty()) {
    throw std::runtime_error("Failed to read shader source: " + path);
  }

  const char *csource = source.c_str();
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &csource, nullptr);
  glCompileShader(shader);

  GLint success = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_TRUE) {
    return shader;
  }

  GLint log_length = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  std::string log(log_length > 1 ? log_length - 1 : 0, '\0');
  if (log_length > 0) {
    glGetShaderInfoLog(shader, log_length, nullptr, log.data());
  }

  glDeleteShader(shader);
  throw std::runtime_error("Shader compilation failed for " + path + ": " +
                           log);
}

GLuint linkProgram(GLuint vertex_shader, GLuint fragment_shader) {
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  GLint success = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (success == GL_TRUE) {
    return program;
  }

  GLint log_length = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
  std::string log(log_length > 1 ? log_length - 1 : 0, '\0');
  if (log_length > 0) {
    glGetProgramInfoLog(program, log_length, nullptr, log.data());
  }

  glDeleteProgram(program);
  throw std::runtime_error("Program link failed: " + log);
}

} // namespace

struct ObjRenderer::RendererState {
  struct DirtyRange {
    bool active = false;
    size_t first = 0;
    size_t last = 0;

    void include(size_t index) {
      if (!active) {
        active = true;
        first = index;
        last = index;
        return;
      }
      if (index < first) {
        first = index;
      }
      if (index > last) {
        last = index;
      }
    }

    void reset() {
      active = false;
      first = 0;
      last = 0;
    }
  };

  struct MeshAsset {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei index_count = 0;
  };

  struct InstanceBuffers {
    GLuint matrix_vbo = 0;
    GLuint material_vbo = 0;
    size_t matrix_capacity_bytes = 0;
    size_t material_capacity_bytes = 0;
  };

  struct SceneInstance {
    RenderInstance transform;
    InstanceMaterial material;
  };

  struct GpuInstanceMaterial {
    int texture_layer = -1;
    glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
  };

  struct SceneModel {
    MeshAsset mesh;
    InstanceBuffers buffers;
    std::vector<SceneInstance> instances;
    std::vector<glm::mat4> matrices;
    std::vector<GpuInstanceMaterial> gpu_materials;
    DirtyRange matrix_dirty_range;
    DirtyRange material_dirty_range;
  };

  GLuint shader_program = 0;
  GLuint texture_array_id = 0;
  GLint view_location = -1;
  GLint projection_location = -1;
  GLint texture_array_location = -1;
  std::vector<SceneModel> models;
};

ObjRenderer::ObjRenderer() : state_(std::make_unique<RendererState>()) {}

ObjRenderer::~ObjRenderer() { shutdown(); }

ObjRenderer::ObjRenderer(ObjRenderer &&other) noexcept
    : state_(std::move(other.state_)) {
  if (!state_) {
    state_ = std::make_unique<RendererState>();
  }
}

ObjRenderer &ObjRenderer::operator=(ObjRenderer &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  shutdown();
  state_ = std::move(other.state_);
  if (!state_) {
    state_ = std::make_unique<RendererState>();
  }
  return *this;
}

namespace {

ObjRenderer::RendererState::MeshAsset loadMeshAsset(const std::string &path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;

  const bool ok =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());
  if (!warn.empty()) {
    std::cerr << "OBJ warning for " << path << ": " << warn << '\n';
  }
  if (!ok) {
    throw std::runtime_error("Failed to load OBJ " + path + ": " + err);
  }

  std::vector<Vertex> unique_vertices;
  std::vector<unsigned int> indices;
  std::unordered_map<Vertex, unsigned int, VertexHash> vertex_map;

  for (const auto &shape : shapes) {
    for (const auto &idx : shape.mesh.indices) {
      Vertex vertex{};
      vertex.pos = {attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]};

      if (idx.texcoord_index >= 0) {
        vertex.uv = {attrib.texcoords[2 * idx.texcoord_index + 0],
                     1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]};
      }

      const auto [it, inserted] = vertex_map.emplace(
          vertex, static_cast<unsigned int>(unique_vertices.size()));
      if (inserted) {
        unique_vertices.push_back(vertex);
      }
      indices.push_back(it->second);
    }
  }

  std::vector<float> vertices;
  vertices.reserve(unique_vertices.size() * 5);
  for (const auto &vertex : unique_vertices) {
    vertices.push_back(vertex.pos.x);
    vertices.push_back(vertex.pos.y);
    vertices.push_back(vertex.pos.z);
    vertices.push_back(vertex.uv.x);
    vertices.push_back(vertex.uv.y);
  }

  ObjRenderer::RendererState::MeshAsset mesh{};
  mesh.index_count = static_cast<GLsizei>(indices.size());

  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);

  glBindVertexArray(mesh.vao);

  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)0);

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return mesh;
}

ObjRenderer::RendererState::InstanceBuffers createInstanceBuffers(GLuint vao) {
  ObjRenderer::RendererState::InstanceBuffers buffers{};
  glGenBuffers(1, &buffers.matrix_vbo);
  glGenBuffers(1, &buffers.material_vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, buffers.matrix_vbo);
  const GLsizei vec4_size = sizeof(glm::vec4);
  for (GLuint i = 0; i < 4; ++i) {
    glEnableVertexAttribArray(3 + i);
    glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4_size,
                          (void *)(static_cast<uintptr_t>(i) * vec4_size));
    glVertexAttribDivisor(3 + i, 1);
  }

  glBindBuffer(GL_ARRAY_BUFFER, buffers.material_vbo);
  glEnableVertexAttribArray(7);
  glVertexAttribIPointer(
      7, 1, GL_INT,
      sizeof(ObjRenderer::RendererState::GpuInstanceMaterial),
      (void *)offsetof(ObjRenderer::RendererState::GpuInstanceMaterial,
                       texture_layer));
  glVertexAttribDivisor(7, 1);

  glEnableVertexAttribArray(8);
  glVertexAttribPointer(
      8, 4, GL_FLOAT, GL_FALSE,
      sizeof(ObjRenderer::RendererState::GpuInstanceMaterial),
      (void *)offsetof(ObjRenderer::RendererState::GpuInstanceMaterial, tint));
  glVertexAttribDivisor(8, 1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return buffers;
}

GLuint createTextureArray(const std::vector<std::string> &paths) {
  if (paths.empty()) {
    throw std::runtime_error(
        "Texture array initialization requires at least one texture.");
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    throw std::runtime_error(std::string("SDL_image failed to initialize: ") +
                             IMG_GetError());
  }

  SDL_Surface *first = IMG_Load(paths.front().c_str());
  if (!first) {
    throw std::runtime_error("Failed to load texture " + paths.front() + ": " +
                             IMG_GetError());
  }

  const int width = first->w;
  const int height = first->h;
  SDL_FreeSurface(first);

  GLuint texture_array_id = 0;
  glGenTextures(1, &texture_array_id);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array_id);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height,
               static_cast<GLsizei>(paths.size()), 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);

  for (int layer = 0; layer < static_cast<int>(paths.size()); ++layer) {
    SDL_Surface *surface = IMG_Load(paths[layer].c_str());
    if (!surface) {
      glDeleteTextures(1, &texture_array_id);
      throw std::runtime_error("Failed to load texture " + paths[layer] + ": " +
                               IMG_GetError());
    }

    if (surface->w != width || surface->h != height) {
      SDL_FreeSurface(surface);
      glDeleteTextures(1, &texture_array_id);
      throw std::runtime_error(
          "Texture dimensions must match for texture array: " + paths[layer]);
    }

    SDL_Surface *formatted =
        SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!formatted) {
      glDeleteTextures(1, &texture_array_id);
      throw std::runtime_error("Failed to convert texture " + paths[layer] +
                               " to RGBA32.");
    }

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, width, height, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, formatted->pixels);
    SDL_FreeSurface(formatted);
  }

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  return texture_array_id;
}

void destroyInstanceBuffers(ObjRenderer::RendererState::InstanceBuffers &buffers) {
  if (buffers.material_vbo != 0) {
    glDeleteBuffers(1, &buffers.material_vbo);
  }
  if (buffers.matrix_vbo != 0) {
    glDeleteBuffers(1, &buffers.matrix_vbo);
  }
  buffers = {};
}

void destroyMeshAsset(ObjRenderer::RendererState::MeshAsset &mesh) {
  if (mesh.ebo != 0) {
    glDeleteBuffers(1, &mesh.ebo);
  }
  if (mesh.vbo != 0) {
    glDeleteBuffers(1, &mesh.vbo);
  }
  if (mesh.vao != 0) {
    glDeleteVertexArrays(1, &mesh.vao);
  }
  mesh = {};
}

void releaseSceneModelInstanceStorage(ObjRenderer::RendererState::SceneModel &model) {
  model.instances.clear();
  model.matrices.clear();
  model.gpu_materials.clear();
  model.matrix_dirty_range.reset();
  model.material_dirty_range.reset();

  glBindBuffer(GL_ARRAY_BUFFER, model.buffers.matrix_vbo);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, model.buffers.material_vbo);
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  model.buffers.matrix_capacity_bytes = 0;
  model.buffers.material_capacity_bytes = 0;
}

} // namespace

bool ObjRenderer::init(const std::vector<std::string> &obj_paths,
                       const std::vector<std::string> &texture_paths) {
  shutdown();

  try {
    const GLuint vertex_shader =
        compileShader("shaders/obj_vertex.glsl", GL_VERTEX_SHADER);
    const GLuint fragment_shader =
        compileShader("shaders/obj_fragment.glsl", GL_FRAGMENT_SHADER);

    state_->shader_program = linkProgram(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    state_->view_location =
        glGetUniformLocation(state_->shader_program, "view");
    state_->projection_location =
        glGetUniformLocation(state_->shader_program, "projection");
    state_->texture_array_location =
        glGetUniformLocation(state_->shader_program, "textureArray");

    state_->models.reserve(obj_paths.size());
    for (const auto &path : obj_paths) {
      RendererState::SceneModel scene_model{};
      scene_model.mesh = loadMeshAsset(path);
      scene_model.buffers = createInstanceBuffers(scene_model.mesh.vao);
      state_->models.push_back(std::move(scene_model));
    }

    state_->texture_array_id = createTextureArray(texture_paths);
    return true;
  } catch (const std::exception &ex) {
    std::cerr << "ObjRenderer init failed: " << ex.what() << '\n';
    shutdown();
    return false;
  }
}

void ObjRenderer::shutdown() {
  if (!state_) {
    state_ = std::make_unique<RendererState>();
    return;
  }

  for (auto &model : state_->models) {
    releaseSceneModelInstanceStorage(model);
    destroyInstanceBuffers(model.buffers);
    destroyMeshAsset(model.mesh);
  }
  state_->models.clear();

  if (state_->texture_array_id != 0) {
    glDeleteTextures(1, &state_->texture_array_id);
    state_->texture_array_id = 0;
  }

  if (state_->shader_program != 0) {
    glDeleteProgram(state_->shader_program);
    state_->shader_program = 0;
  }

  state_->view_location = -1;
  state_->projection_location = -1;
  state_->texture_array_location = -1;
}

void ObjRenderer::clearScene() {
  for (auto &model : state_->models) {
    releaseSceneModelInstanceStorage(model);
  }
}

int ObjRenderer::addModelInstance(int model_id, glm::vec3 position,
                                  glm::vec3 scale, int texture_layer) {
  InstanceMaterial material;
  material.texture_layer = texture_layer;
  return addModelInstance(model_id, position, scale, material);
}

int ObjRenderer::addModelInstance(int model_id, glm::vec3 position,
                                  glm::vec3 scale,
                                  const InstanceMaterial &material) {
  if (model_id < 0 || model_id >= static_cast<int>(state_->models.size())) {
    return -1;
  }

  auto &model = state_->models[model_id];
  const size_t index = model.instances.size();

  RendererState::SceneInstance scene_instance;
  scene_instance.transform.position = position;
  scene_instance.transform.scale = scale;
  scene_instance.material = material;

  glm::mat4 matrix(1.0f);
  matrix = glm::translate(matrix, position);
  matrix = glm::scale(matrix, scale);

  RendererState::GpuInstanceMaterial gpu_material;
  gpu_material.texture_layer = material.texture_layer;
  gpu_material.tint = material.tint;

  model.instances.push_back(scene_instance);
  model.matrices.push_back(matrix);
  model.gpu_materials.push_back(gpu_material);
  model.matrix_dirty_range.include(index);
  model.material_dirty_range.include(index);
  return static_cast<int>(index);
}

void ObjRenderer::draw(const glm::mat4 &view,
                       const glm::mat4 &projection) const {
  if (state_->shader_program == 0) {
    return;
  }

  glUseProgram(state_->shader_program);
  glUniformMatrix4fv(state_->view_location, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(state_->projection_location, 1, GL_FALSE,
                     glm::value_ptr(projection));

  glUniform1i(state_->texture_array_location, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, state_->texture_array_id);

  for (const auto &model : state_->models) {
    if (model.instances.empty()) {
      continue;
    }

    glBindVertexArray(model.mesh.vao);
    glDrawElementsInstanced(GL_TRIANGLES, model.mesh.index_count,
                            GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(model.instances.size()));
  }

  glBindVertexArray(0);
}

void ObjRenderer::updateInstanceMatrices() {
  for (auto &model : state_->models) {
    for (size_t i = 0; i < model.instances.size(); ++i) {
      auto &instance = model.instances[i].transform;
      if (!instance.dirty) {
        continue;
      }

      glm::mat4 matrix(1.0f);
      matrix = glm::translate(matrix, instance.position);
      matrix = glm::rotate(matrix, instance.rotation.x, glm::vec3(1, 0, 0));
      matrix = glm::rotate(matrix, instance.rotation.y, glm::vec3(0, 1, 0));
      matrix = glm::rotate(matrix, instance.rotation.z, glm::vec3(0, 0, 1));
      matrix = glm::scale(matrix, instance.scale);

      model.matrices[i] = matrix;
      model.matrix_dirty_range.include(i);
      instance.dirty = false;
    }
  }
}

void ObjRenderer::uploadInstanceData() {
  for (auto &model : state_->models) {
    if (model.instances.empty()) {
      continue;
    }

    if (model.matrix_dirty_range.active) {
      const size_t total_bytes = model.matrices.size() * sizeof(glm::mat4);
      glBindBuffer(GL_ARRAY_BUFFER, model.buffers.matrix_vbo);
      if (total_bytes > model.buffers.matrix_capacity_bytes) {
        model.buffers.matrix_capacity_bytes = total_bytes * 2;
        glBufferData(GL_ARRAY_BUFFER, model.buffers.matrix_capacity_bytes,
                     nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, total_bytes, model.matrices.data());
      } else {
        const size_t first = model.matrix_dirty_range.first;
        const size_t count =
            model.matrix_dirty_range.last - model.matrix_dirty_range.first + 1;
        glBufferSubData(GL_ARRAY_BUFFER, first * sizeof(glm::mat4),
                        count * sizeof(glm::mat4), model.matrices.data() + first);
      }
      model.matrix_dirty_range.reset();
    }

    if (model.material_dirty_range.active) {
      const size_t total_bytes =
          model.gpu_materials.size() *
          sizeof(RendererState::GpuInstanceMaterial);
      glBindBuffer(GL_ARRAY_BUFFER, model.buffers.material_vbo);
      if (total_bytes > model.buffers.material_capacity_bytes) {
        model.buffers.material_capacity_bytes = total_bytes * 2;
        glBufferData(GL_ARRAY_BUFFER, model.buffers.material_capacity_bytes,
                     nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, total_bytes,
                        model.gpu_materials.data());
      } else {
        const size_t first = model.material_dirty_range.first;
        const size_t count = model.material_dirty_range.last -
                             model.material_dirty_range.first + 1;
        glBufferSubData(GL_ARRAY_BUFFER,
                        first * sizeof(RendererState::GpuInstanceMaterial),
                        count * sizeof(RendererState::GpuInstanceMaterial),
                        model.gpu_materials.data() + first);
      }
      model.material_dirty_range.reset();
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ObjRenderer::transformInstance(int model_id, int object_id,
                                    const TransformParams &params) {
  if (model_id < 0 || model_id >= static_cast<int>(state_->models.size())) {
    return;
  }
  auto &instances = state_->models[model_id].instances;
  if (object_id < 0 || object_id >= static_cast<int>(instances.size())) {
    return;
  }

  auto &instance = instances[object_id].transform;
  if (params.position) {
    instance.position = *params.position;
  }
  if (params.scale) {
    instance.scale = *params.scale;
  }
  if (params.rotation) {
    instance.rotation = *params.rotation;
  }
  instance.dirty = true;
}

glm::vec3 ObjRenderer::translateInstance(int model_id, int object_id,
                                         glm::vec3 delta) {
  auto &instance = getInstance(model_id, object_id);
  instance.position += delta;
  instance.dirty = true;
  return instance.position;
}

glm::vec3 ObjRenderer::scaleInstance(int model_id, int object_id,
                                     glm::vec3 delta) {
  auto &instance = getInstance(model_id, object_id);
  instance.scale += delta;
  instance.dirty = true;
  return instance.scale;
}

glm::vec3 ObjRenderer::rotateInstance(int model_id, int object_id,
                                      glm::vec3 delta) {
  auto &instance = getInstance(model_id, object_id);
  instance.rotation += delta;
  instance.dirty = true;
  return instance.rotation;
}

void ObjRenderer::setInstanceMaterial(int model_id, int object_id,
                                      const InstanceMaterial &material) {
  if (model_id < 0 || model_id >= static_cast<int>(state_->models.size())) {
    return;
  }
  auto &model = state_->models[model_id];
  if (object_id < 0 || object_id >= static_cast<int>(model.instances.size())) {
    return;
  }

  model.instances[object_id].material = material;
  model.gpu_materials[object_id].texture_layer = material.texture_layer;
  model.gpu_materials[object_id].tint = material.tint;
  model.material_dirty_range.include(static_cast<size_t>(object_id));
}

InstanceMaterial ObjRenderer::getInstanceMaterial(int model_id,
                                                  int object_id) const {
  if (model_id < 0 || model_id >= static_cast<int>(state_->models.size())) {
    throw std::out_of_range(
        "Invalid model_id in ObjRenderer::getInstanceMaterial");
  }
  const auto &model = state_->models[model_id];
  if (object_id < 0 || object_id >= static_cast<int>(model.instances.size())) {
    throw std::out_of_range(
        "Invalid object_id in ObjRenderer::getInstanceMaterial");
  }
  return model.instances[object_id].material;
}

RenderInstance &ObjRenderer::getInstance(int model_id, int object_id) {
  return const_cast<RenderInstance &>(
      std::as_const(*this).getInstance(model_id, object_id));
}

const RenderInstance &ObjRenderer::getInstance(int model_id,
                                               int object_id) const {
  if (model_id < 0 || model_id >= static_cast<int>(state_->models.size())) {
    throw std::out_of_range("Invalid model_id in ObjRenderer::getInstance");
  }

  const auto &instances = state_->models[model_id].instances;
  if (object_id < 0 || object_id >= static_cast<int>(instances.size())) {
    throw std::out_of_range("Invalid object_id in ObjRenderer::getInstance");
  }

  return instances[object_id].transform;
}

int ObjRenderer::objectCount(int model_id) const {
  if (model_id < 0 || model_id >= static_cast<int>(state_->models.size())) {
    return 0;
  }
  return static_cast<int>(state_->models[model_id].instances.size());
}

int ObjRenderer::modelCount() const {
  return static_cast<int>(state_->models.size());
}
