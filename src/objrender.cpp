#include <obj.h>

#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glslread.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace fs = std::filesystem;

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

std::string normalizeTextureReference(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  return value;
}

std::string normalizePathString(const fs::path &path) {
  fs::path normalized = path.lexically_normal();
  if (normalized.is_relative()) {
    normalized = fs::absolute(normalized);
  }
  return normalized.string();
}

void ensureImageFormats() {
  const int wanted = IMG_INIT_PNG | IMG_INIT_JPG;
  const int initialized = IMG_Init(wanted);
  if ((initialized & wanted) != wanted) {
    throw std::runtime_error(std::string("SDL_image failed to initialize: ") +
                             IMG_GetError());
  }
}

GLuint loadTexture2D(const std::string &path) {
  ensureImageFormats();

  SDL_Surface *surface = IMG_Load(path.c_str());
  if (!surface) {
    throw std::runtime_error("Failed to load texture " + path + ": " +
                             IMG_GetError());
  }

  SDL_Surface *formatted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(surface);
  if (!formatted) {
    throw std::runtime_error("Failed to convert texture " + path +
                             " to RGBA32.");
  }

  GLuint texture_id = 0;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, formatted->w, formatted->h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, formatted->pixels);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  SDL_FreeSurface(formatted);
  return texture_id;
}

struct PartBuilder {
  int material_id = -1;
  std::vector<Vertex> unique_vertices;
  std::vector<unsigned int> indices;
  std::unordered_map<Vertex, unsigned int, VertexHash> vertex_map;
};

} // namespace

struct ObjRenderer::RendererState {
  struct MeshPart {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei index_count = 0;
    int default_texture_id = -1;
    glm::vec4 base_color{1.0f, 1.0f, 1.0f, 1.0f};
  };

  struct MeshAsset {
    std::vector<MeshPart> parts;
  };

  struct SceneInstance {
    RenderInstance transform;
    InstanceMaterial material;
  };

  struct SceneModel {
    MeshAsset mesh;
    std::vector<SceneInstance> instances;
    std::vector<glm::mat4> matrices;
  };

  GLuint shader_program = 0;
  GLint model_location = -1;
  GLint view_location = -1;
  GLint projection_location = -1;
  GLint texture_location = -1;
  GLint use_texture_location = -1;
  GLint base_color_location = -1;
  GLint instance_tint_location = -1;
  std::vector<SceneModel> models;
  std::vector<GLuint> texture_ids;
  std::vector<std::string> texture_paths;
  std::unordered_map<std::string, int> texture_lookup;
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

int getOrLoadTexture(ObjRenderer::RendererState &state, const fs::path &path) {
  const std::string normalized_path = normalizePathString(path);
  const auto found = state.texture_lookup.find(normalized_path);
  if (found != state.texture_lookup.end()) {
    return found->second;
  }

  const GLuint texture_id = loadTexture2D(normalized_path);
  const int texture_index = static_cast<int>(state.texture_ids.size());
  state.texture_ids.push_back(texture_id);
  state.texture_paths.push_back(normalized_path);
  state.texture_lookup.emplace(normalized_path, texture_index);
  return texture_index;
}

ObjRenderer::RendererState::MeshPart createMeshPart(
    const PartBuilder &builder, int default_texture_id,
    const glm::vec4 &base_color) {
  if (builder.indices.empty()) {
    throw std::runtime_error("Cannot create mesh part without indices.");
  }

  std::vector<float> vertices;
  vertices.reserve(builder.unique_vertices.size() * 5);
  for (const auto &vertex : builder.unique_vertices) {
    vertices.push_back(vertex.pos.x);
    vertices.push_back(vertex.pos.y);
    vertices.push_back(vertex.pos.z);
    vertices.push_back(vertex.uv.x);
    vertices.push_back(vertex.uv.y);
  }

  ObjRenderer::RendererState::MeshPart part{};
  part.index_count = static_cast<GLsizei>(builder.indices.size());
  part.default_texture_id = default_texture_id;
  part.base_color = base_color;

  try {
    glGenVertexArrays(1, &part.vao);
    glGenBuffers(1, &part.vbo);
    glGenBuffers(1, &part.ebo);

    glBindVertexArray(part.vao);

    glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 builder.indices.size() * sizeof(unsigned int),
                 builder.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return part;
  } catch (...) {
    if (part.ebo != 0) {
      glDeleteBuffers(1, &part.ebo);
    }
    if (part.vbo != 0) {
      glDeleteBuffers(1, &part.vbo);
    }
    if (part.vao != 0) {
      glDeleteVertexArrays(1, &part.vao);
    }
    throw;
  }
}

ObjRenderer::RendererState::MeshAsset loadMeshAsset(
    ObjRenderer::RendererState &state, const std::string &path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn;
  std::string err;

  const fs::path obj_path(path);
  const fs::path obj_dir =
      obj_path.has_parent_path() ? obj_path.parent_path() : fs::path(".");

  const std::string obj_path_string = obj_path.string();
  const std::string obj_dir_string = obj_dir.string();
  const bool ok =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                       obj_path_string.c_str(), obj_dir_string.c_str());
  if (!warn.empty()) {
    std::cerr << "OBJ warning for " << path << ": " << warn << '\n';
  }
  if (!ok) {
    throw std::runtime_error("Failed to load OBJ " + path + ": " + err);
  }

  struct MaterialInfo {
    int texture_id = -1;
    glm::vec4 base_color{1.0f, 1.0f, 1.0f, 1.0f};
  };

  std::vector<MaterialInfo> material_infos(materials.size());
  for (size_t i = 0; i < materials.size(); ++i) {
    const auto &material = materials[i];
    MaterialInfo info;
    info.base_color = {material.diffuse[0], material.diffuse[1],
                       material.diffuse[2], material.dissolve};

    if (!material.diffuse_texname.empty()) {
      const fs::path texture_path =
          obj_dir / normalizeTextureReference(material.diffuse_texname);
      try {
        info.texture_id = getOrLoadTexture(state, texture_path);
        info.base_color = {1.0f, 1.0f, 1.0f, material.dissolve};
      } catch (const std::exception &ex) {
        std::cerr << "Failed to load diffuse texture for material '"
                  << material.name << "' in " << path << ": " << ex.what()
                  << '\n';
      }
    }

    material_infos[i] = info;
  }

  std::vector<PartBuilder> builders;
  std::unordered_map<int, size_t> builder_indices;

  auto &fallback_builder = builders.emplace_back();
  fallback_builder.material_id = -1;
  builder_indices.emplace(-1, 0);

  auto getBuilder = [&](int material_id) -> PartBuilder & {
    const auto found = builder_indices.find(material_id);
    if (found != builder_indices.end()) {
      return builders[found->second];
    }

    const size_t new_index = builders.size();
    builder_indices.emplace(material_id, new_index);
    auto &builder = builders.emplace_back();
    builder.material_id = material_id;
    return builder;
  };

  for (const auto &shape : shapes) {
    size_t index_offset = 0;
    for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
      const int face_vertex_count = shape.mesh.num_face_vertices[face];
      int material_id = -1;
      if (face < shape.mesh.material_ids.size()) {
        material_id = shape.mesh.material_ids[face];
      }

      auto &builder = getBuilder(material_id);
      for (int v = 0; v < face_vertex_count; ++v) {
        const tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
        if (idx.vertex_index < 0) {
          continue;
        }

        Vertex vertex{};
        vertex.pos = {attrib.vertices[3 * idx.vertex_index + 0],
                      attrib.vertices[3 * idx.vertex_index + 1],
                      attrib.vertices[3 * idx.vertex_index + 2]};

        if (idx.texcoord_index >= 0 &&
            2 * idx.texcoord_index + 1 <
                static_cast<int>(attrib.texcoords.size())) {
          vertex.uv = {attrib.texcoords[2 * idx.texcoord_index + 0],
                       1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]};
        }

        const auto [it, inserted] = builder.vertex_map.emplace(
            vertex, static_cast<unsigned int>(builder.unique_vertices.size()));
        if (inserted) {
          builder.unique_vertices.push_back(vertex);
        }
        builder.indices.push_back(it->second);
      }

      index_offset += static_cast<size_t>(face_vertex_count);
    }
  }

  ObjRenderer::RendererState::MeshAsset mesh{};
  try {
    for (const auto &builder : builders) {
      if (builder.indices.empty()) {
        continue;
      }

      int default_texture_id = -1;
      glm::vec4 base_color{1.0f, 1.0f, 1.0f, 1.0f};
      if (builder.material_id >= 0 &&
          builder.material_id < static_cast<int>(material_infos.size())) {
        default_texture_id = material_infos[builder.material_id].texture_id;
        base_color = material_infos[builder.material_id].base_color;
      }

      mesh.parts.push_back(
          createMeshPart(builder, default_texture_id, base_color));
    }
  } catch (...) {
    for (auto &part : mesh.parts) {
      if (part.ebo != 0) {
        glDeleteBuffers(1, &part.ebo);
      }
      if (part.vbo != 0) {
        glDeleteBuffers(1, &part.vbo);
      }
      if (part.vao != 0) {
        glDeleteVertexArrays(1, &part.vao);
      }
    }
    throw;
  }

  if (mesh.parts.empty()) {
    throw std::runtime_error("OBJ contained no renderable geometry: " + path);
  }

  return mesh;
}

void destroyMeshPart(ObjRenderer::RendererState::MeshPart &part) {
  if (part.ebo != 0) {
    glDeleteBuffers(1, &part.ebo);
  }
  if (part.vbo != 0) {
    glDeleteBuffers(1, &part.vbo);
  }
  if (part.vao != 0) {
    glDeleteVertexArrays(1, &part.vao);
  }
  part = {};
}

void destroyMeshAsset(ObjRenderer::RendererState::MeshAsset &mesh) {
  for (auto &part : mesh.parts) {
    destroyMeshPart(part);
  }
  mesh.parts.clear();
}

void releaseSceneModelInstanceStorage(ObjRenderer::RendererState::SceneModel &model) {
  model.instances.clear();
  model.matrices.clear();
}

} // namespace

bool ObjRenderer::init() {
  shutdown();

  try {
    const GLuint vertex_shader =
        compileShader("shaders/obj_vertex.glsl", GL_VERTEX_SHADER);
    const GLuint fragment_shader =
        compileShader("shaders/obj_fragment.glsl", GL_FRAGMENT_SHADER);

    state_->shader_program = linkProgram(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    state_->model_location = glGetUniformLocation(state_->shader_program, "model");
    state_->view_location = glGetUniformLocation(state_->shader_program, "view");
    state_->projection_location =
        glGetUniformLocation(state_->shader_program, "projection");
    state_->texture_location =
        glGetUniformLocation(state_->shader_program, "diffuseTexture");
    state_->use_texture_location =
        glGetUniformLocation(state_->shader_program, "useTexture");
    state_->base_color_location =
        glGetUniformLocation(state_->shader_program, "baseColor");
    state_->instance_tint_location =
        glGetUniformLocation(state_->shader_program, "instanceTint");

    return true;
  } catch (const std::exception &ex) {
    std::cerr << "ObjRenderer init failed: " << ex.what() << '\n';
    shutdown();
    return false;
  }
}

int ObjRenderer::loadMesh(const std::string &path) {
  try {
    RendererState::SceneModel scene_model{};
    scene_model.mesh = loadMeshAsset(*state_, path);
    state_->models.push_back(std::move(scene_model));
    return static_cast<int>(state_->models.size()) - 1;
  } catch (const std::exception &ex) {
    std::cerr << "loadMesh failed for " << path << ": " << ex.what() << '\n';
    return -1;
  }
}

int ObjRenderer::loadTexture(const std::string &path) {
  try {
    return getOrLoadTexture(*state_, fs::path(path));
  } catch (const std::exception &ex) {
    std::cerr << "loadTexture failed for " << path << ": " << ex.what() << '\n';
    return -1;
  }
}

void ObjRenderer::shutdown() {
  if (!state_) {
    state_ = std::make_unique<RendererState>();
    return;
  }

  for (auto &model : state_->models) {
    releaseSceneModelInstanceStorage(model);
    destroyMeshAsset(model.mesh);
  }
  state_->models.clear();

  for (GLuint texture_id : state_->texture_ids) {
    if (texture_id != 0) {
      glDeleteTextures(1, &texture_id);
    }
  }
  state_->texture_ids.clear();
  state_->texture_paths.clear();
  state_->texture_lookup.clear();

  if (state_->shader_program != 0) {
    glDeleteProgram(state_->shader_program);
    state_->shader_program = 0;
  }

  state_->model_location = -1;
  state_->view_location = -1;
  state_->projection_location = -1;
  state_->texture_location = -1;
  state_->use_texture_location = -1;
  state_->base_color_location = -1;
  state_->instance_tint_location = -1;
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
  scene_instance.transform.dirty = false;
  scene_instance.material = material;

  glm::mat4 matrix(1.0f);
  matrix = glm::translate(matrix, position);
  matrix = glm::scale(matrix, scale);

  model.instances.push_back(scene_instance);
  model.matrices.push_back(matrix);
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
  glUniform1i(state_->texture_location, 0);
  glActiveTexture(GL_TEXTURE0);

  for (const auto &model : state_->models) {
    if (model.instances.empty()) {
      continue;
    }

    for (const auto &part : model.mesh.parts) {
      glBindVertexArray(part.vao);

      for (size_t instance_index = 0; instance_index < model.instances.size();
           ++instance_index) {
        const auto &scene_instance = model.instances[instance_index];
        const glm::mat4 &matrix = model.matrices[instance_index];
        const int texture_index = scene_instance.material.texture_layer >= 0
                                      ? scene_instance.material.texture_layer
                                      : part.default_texture_id;
        const bool use_texture =
            texture_index >= 0 &&
            texture_index < static_cast<int>(state_->texture_ids.size());

        glUniformMatrix4fv(state_->model_location, 1, GL_FALSE,
                           glm::value_ptr(matrix));
        glUniform4fv(state_->base_color_location, 1,
                     glm::value_ptr(part.base_color));
        glUniform4fv(state_->instance_tint_location, 1,
                     glm::value_ptr(scene_instance.material.tint));
        glUniform1i(state_->use_texture_location, use_texture ? 1 : 0);
        glBindTexture(GL_TEXTURE_2D,
                      use_texture ? state_->texture_ids[texture_index] : 0);
        glDrawElements(GL_TRIANGLES, part.index_count, GL_UNSIGNED_INT,
                       nullptr);
      }
    }
  }

  glBindTexture(GL_TEXTURE_2D, 0);
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
      instance.dirty = false;
    }
  }
}

void ObjRenderer::uploadInstanceData() {}

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
