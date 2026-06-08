#include <skybox.h>

#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glslread.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <iostream>
#include <stdexcept>

namespace {

GLuint program = 0;
GLuint vao = 0;
GLuint vbo = 0;
GLuint texture = 0;
GLint view_loc = -1;
GLint proj_loc = -1;
GLint skybox_loc = -1;

const float cube_vertices[] = {
    // back
    -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
    // front
    -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
    // left
    -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,
    // right
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    // bottom
    -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,
    // top
    -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
};

GLuint loadCubeMap(const std::vector<std::string> &paths) {
  if (paths.size() != 6) {
    throw std::runtime_error(
        "Skybox requires exactly 6 textures (+X, -X, +Y, -Y, +Z, -Z).");
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    throw std::runtime_error(std::string("SDL_image init failed: ") +
                             IMG_GetError());
  }

  GLuint tex = 0;
  glGenTextures(1, &tex);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

  const std::array<GLenum, 6> targets = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

  for (int i = 0; i < 6; ++i) {
    SDL_Surface *surface = IMG_Load(paths[i].c_str());
    if (!surface) {
      glDeleteTextures(1, &tex);
      throw std::runtime_error("Failed to load skybox face " + paths[i] +
                               ": " + IMG_GetError());
    }

    SDL_Surface *formatted =
        SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);
    if (!formatted) {
      glDeleteTextures(1, &tex);
      throw std::runtime_error("Failed to convert " + paths[i] + " to RGBA32");
    }

    glTexImage2D(targets[i], 0, GL_RGBA8, formatted->w, formatted->h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, formatted->pixels);
    SDL_FreeSurface(formatted);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  return tex;
}

GLuint compileShader(const std::string &path, GLenum type) {
  std::string source = readFile(path);
  if (source.empty()) {
    throw std::runtime_error("Failed to read shader: " + path);
  }

  const char *csource = source.c_str();
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &csource, nullptr);
  glCompileShader(shader);

  GLint success = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success) {
    return shader;
  }

  GLint log_length = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
  std::string log(log_length > 1 ? log_length - 1 : 0, '\0');
  if (log_length > 0) {
    glGetShaderInfoLog(shader, log_length, nullptr, log.data());
  }
  glDeleteShader(shader);
  throw std::runtime_error("Skybox shader compile failed (" + path + "): " +
                           log);
}

GLuint linkProgram(GLuint vs, GLuint fs) {
  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);

  GLint success = GL_FALSE;
  glGetProgramiv(prog, GL_LINK_STATUS, &success);
  if (success) {
    return prog;
  }

  GLint log_length = 0;
  glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
  std::string log(log_length > 1 ? log_length - 1 : 0, '\0');
  if (log_length > 0) {
    glGetProgramInfoLog(prog, log_length, nullptr, log.data());
  }
  glDeleteProgram(prog);
  throw std::runtime_error("Skybox program link failed: " + log);
}

} // namespace

void initskybox(const std::vector<std::string> &paths) {
  try {
    GLuint vs =
        compileShader("shaders/skybox_vertex.glsl", GL_VERTEX_SHADER);
    GLuint fs =
        compileShader("shaders/skybox_fragment.glsl", GL_FRAGMENT_SHADER);
    program = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    view_loc = glGetUniformLocation(program, "view");
    proj_loc = glGetUniformLocation(program, "projection");
    skybox_loc = glGetUniformLocation(program, "skybox");

    texture = loadCubeMap(paths);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

  } catch (const std::exception &ex) {
    std::cerr << "initskybox failed: " << ex.what() << '\n';
  }
}

void drawskybox(const glm::mat4 &view, const glm::mat4 &projection) {
  if (program == 0) {
    return;
  }

  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LEQUAL);

  glUseProgram(program);
  glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
  glUniform1i(skybox_loc, 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
}
