#include <obj.h>

#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glslread.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
  struct MeshAsset {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint instance_matrix_vbo = 0;
    GLuint instance_tex_index_vbo = 0;
    GLsizei index_count = 0;
  };

  struct ModelBucket {
    MeshAsset mesh;
    std::vector<RenderInstance> instances;
    std::vector<glm::mat4> matrices;
    std::vector<int> texture_layers;
    size_t matrix_capacity_bytes = 0;
    size_t texture_capacity_bytes = 0;
    bool matrix_upload_dirty = false;
    bool texture_upload_dirty = false;
  };

  GLuint shader_program = 0;
  GLuint texture_array_id = 0;
  GLint view_location = -1;
  GLint projection_location = -1;
  GLint texture_array_location = -1;
  std::vector<ModelBucket> models;
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

      const auto [it, inserted] =
          vertex_map.emplace(vertex, static_cast<unsigned int>(unique_vertices.size()));
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
  glGenBuffers(1, &mesh.instance_matrix_vbo);
  glGenBuffers(1, &mesh.instance_tex_index_vbo);

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

  glBindBuffer(GL_ARRAY_BUFFER, mesh.instance_matrix_vbo);
  const GLsizei vec4_size = sizeof(glm::vec4);
  for (GLuint i = 0; i < 4; ++i) {
    glEnableVertexAttribArray(3 + i);
    glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4_size,
                          (void *)(static_cast<uintptr_t>(i) * vec4_size));
    glVertexAttribDivisor(3 + i, 1);
  }

  glBindBuffer(GL_ARRAY_BUFFER, mesh.instance_tex_index_vbo);
  glEnableVertexAttribArray(7);
  glVertexAttribIPointer(7, 1, GL_INT, sizeof(int), (void *)0);
  glVertexAttribDivisor(7, 1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return mesh;
}

GLuint createTextureArray(const std::vector<std::string> &paths) {
  if (paths.empty()) {
    throw std::runtime_error("Texture array initialization requires at least one texture.");
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
      throw std::runtime_error("Texture dimensions must match for texture array: " +
                               paths[layer]);
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

void destroyMeshAsset(ObjRenderer::RendererState::MeshAsset &mesh) {
  if (mesh.instance_tex_index_vbo != 0) {
    glDeleteBuffers(1, &mesh.instance_tex_index_vbo);
  }
  if (mesh.instance_matrix_vbo != 0) {
    glDeleteBuffers(1, &mesh.instance_matrix_vbo);
  }
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
      RendererState::ModelBucket bucket{};
      bucket.mesh = loadMeshAsset(path);
      state_->models.push_back(std::move(bucket));
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

int ObjRenderer::addModelInstance(int model_id, glm::vec3 position,
                                  glm::vec3 scale, int texture_layer) {
  if (model_id < 0 || model_id >= static_cast<int>(state_->models.size())) {
    return -1;
  }

  auto &model = state_->models[model_id];
  RenderInstance instance;
  instance.position = position;
  instance.scale = scale;

  glm::mat4 matrix(1.0f);
  matrix = glm::translate(matrix, position);
  matrix = glm::scale(matrix, scale);

  model.instances.push_back(instance);
  model.matrices.push_back(matrix);
  model.texture_layers.push_back(texture_layer);
  model.matrix_upload_dirty = true;
  model.texture_upload_dirty = true;
  return static_cast<int>(model.instances.size()) - 1;
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
    bool changed = false;
    for (size_t i = 0; i < model.instances.size(); ++i) {
      auto &instance = model.instances[i];
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
      changed = true;
    }

    if (changed) {
      model.matrix_upload_dirty = true;
    }
  }
}

void ObjRenderer::uploadInstanceData() {
  for (auto &model : state_->models) {
    if (model.instances.empty()) {
      continue;
    }

    if (model.matrix_upload_dirty) {
      const size_t matrix_bytes = model.matrices.size() * sizeof(glm::mat4);
      glBindBuffer(GL_ARRAY_BUFFER, model.mesh.instance_matrix_vbo);
      if (matrix_bytes > model.matrix_capacity_bytes) {
        model.matrix_capacity_bytes = matrix_bytes * 2;
        glBufferData(GL_ARRAY_BUFFER, model.matrix_capacity_bytes, nullptr,
                     GL_DYNAMIC_DRAW);
      }
      glBufferSubData(GL_ARRAY_BUFFER, 0, matrix_bytes, model.matrices.data());
      model.matrix_upload_dirty = false;
    }

    if (model.texture_upload_dirty) {
      const size_t texture_bytes = model.texture_layers.size() * sizeof(int);
      glBindBuffer(GL_ARRAY_BUFFER, model.mesh.instance_tex_index_vbo);
      if (texture_bytes > model.texture_capacity_bytes) {
        model.texture_capacity_bytes = texture_bytes * 2;
        glBufferData(GL_ARRAY_BUFFER, model.texture_capacity_bytes, nullptr,
                     GL_DYNAMIC_DRAW);
      }
      glBufferSubData(GL_ARRAY_BUFFER, 0, texture_bytes,
                      model.texture_layers.data());
      model.texture_upload_dirty = false;
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

  auto &instance = instances[object_id];
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

  return instances[object_id];
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
