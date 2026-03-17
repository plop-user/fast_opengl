
#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glslread.h>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <numbers>
#include <string>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct DrawElementsIndirectCommand {
  GLuint count;
  GLuint instanceCount;
  GLuint firstIndex;
  GLuint baseVertex;
  GLuint baseInstance;
};

struct ObjModel {
  GLuint VAO, VBO, EBO;
  GLuint instanceMatrixVBO;
  GLuint instanceTexIndexVBO;

  int indexCount;

  std::vector<glm::mat4> matrices;
  std::vector<float> texIndices;

  size_t matrixCapacity = 0;
  size_t texCapacity = 0;
};
struct Vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	bool operator==(const Vertex& other) const {
		return pos == other.pos && uv == other.uv;
	}
};

struct VertexHash {
    size_t operator()(const Vertex& v) const {
        return ((std::hash<float>()(v.pos.x) ^
                (std::hash<float>()(v.pos.y) << 1)) >> 1) ^
                (std::hash<float>()(v.pos.z) << 1) ^
                (std::hash<float>()(v.uv.x) << 1) ^
                (std::hash<float>()(v.uv.y) << 1);
    }
};



static std::vector<ObjModel> models;

static GLuint shaderprogram;
static GLuint textureArrayID;
static GLuint indirectBuffer;

static GLint viewLoc;
static GLint projLoc;
static GLint texLoc;

static std::vector<DrawElementsIndirectCommand> drawCommands;

GLuint compileShader(const std::string& path, GLenum type) {
  std::string src = readFile(path);
  const char* csrc = src.c_str();

  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &csrc, nullptr);
  glCompileShader(s);

  int success;
  char log[512];
  glGetShaderiv(s, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(s, 512, nullptr, log);
    std::cout << "Shader error " << log << "\n" << std::endl;
  }
  return s;
}

void initShader() {
  GLuint vs = compileShader("shaders/obj_vertex.glsl", GL_VERTEX_SHADER);
  GLuint fs = compileShader("shaders/obj_fragment.glsl", GL_FRAGMENT_SHADER);

  shaderprogram = glCreateProgram();
  glAttachShader(shaderprogram, vs);
  glAttachShader(shaderprogram, fs);
  glLinkProgram(shaderprogram);

  viewLoc = glGetUniformLocation(shaderprogram, "view");
  projLoc = glGetUniformLocation(shaderprogram, "projection");
  texLoc = glGetUniformLocation(shaderprogram, "textureArray");
}

ObjModel loadOBJ(const std::string& path) {
  ObjModel m;

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> mats;

  std::string warn, err;

  tinyobj::LoadObj(&attrib, &shapes, &mats,& warn, &err, path.c_str());

std::vector<Vertex> uniqueVertices;
std::vector<unsigned int> indices;
std::unordered_map<Vertex, unsigned int, VertexHash> vertexMap;

for (const auto& shape : shapes)
{
    for (const auto& idx : shape.mesh.indices)
    {
        Vertex v{};

        // position
        v.pos = {
            attrib.vertices[3*idx.vertex_index+0],
            attrib.vertices[3*idx.vertex_index+1],
            attrib.vertices[3*idx.vertex_index+2]
        };

        // UV
        if(idx.texcoord_index >= 0)
        {
            v.uv = {
                attrib.texcoords[2*idx.texcoord_index+0],
                1.0f - attrib.texcoords[2*idx.texcoord_index+1] // flip Y
            };
        }
        else
        {
            v.uv = {0.0f, 0.0f};
        }

        // check if already exists
        if(vertexMap.count(v) == 0)
        {
            vertexMap[v] = uniqueVertices.size();
            uniqueVertices.push_back(v);
        }

        indices.push_back(vertexMap[v]);
    }
}

	std::vector<float> vertices;
	for(auto& v: uniqueVertices){
		vertices.push_back(v.pos.x);
		vertices.push_back(v.pos.y);
		vertices.push_back(v.pos.z);

		vertices.push_back(v.uv.x);
		vertices.push_back(v.uv.y);
	}

  m.indexCount = indices.size();

  glGenVertexArrays(1, &m.VAO);
  glGenBuffers(1, &m.VBO);
  glGenBuffers(1, &m.EBO);

  glBindVertexArray(m.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void*)(3 * sizeof(float)));

  glGenBuffers(1, &m.instanceMatrixVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m.instanceMatrixVBO);

  size_t vec4Size = sizeof(glm::vec4);

  for (int i = 0; i < 4; i++) {
    glEnableVertexAttribArray(3 + i);
    glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size,
                          (void*)(i * vec4Size));
    glVertexAttribDivisor(3 + i, 1);
  }

  glGenBuffers(1, &m.instanceTexIndexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m.instanceTexIndexVBO);

  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
  glVertexAttribDivisor(7, 1);

  glBindVertexArray(0);

  return m;
}

void createTextureArray(const std::vector<std::string>& paths) {
  glGenTextures(1, &textureArrayID);
  glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayID);
 if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
{
    std::cout << "SDL_image failed: " << IMG_GetError() << std::endl;
}
	int layers = paths.size();
  
  SDL_Surface* first = IMG_Load(paths[0].c_str());
if(!first)
{
    std::cout << "Failed to load texture: " << paths[0] << std::endl;
    std::cout << "SDL_image error: " << IMG_GetError() << std::endl;
    return;
}
  int w = first->w;
  int h = first->h;

  SDL_FreeSurface(first);

  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, w, h, layers, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);

  for (int i = 0; i < layers; i++) {
    SDL_Surface* s = IMG_Load(paths[i].c_str());
    SDL_Surface* f = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGBA32, 0);

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_RGBA,
                    GL_UNSIGNED_BYTE, f->pixels);

    SDL_FreeSurface(s);
    SDL_FreeSurface(f);
  }

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

void initSystem(const std::vector<std::string>& objPaths, const std::vector<std::string>& texPaths) {
  initShader();

  for (auto& p : objPaths) models.push_back(loadOBJ(p));

  createTextureArray(texPaths);

  glGenBuffers(1, &indirectBuffer);
}

int addObject(int modelID, glm::vec3 pos, float scale, float texID) {
  ObjModel& m = models[modelID];

  glm::mat4 mat(1.0f);
  mat = glm::translate(mat, pos);
  mat = glm::scale(mat, glm::vec3(scale));

  m.matrices.push_back(mat);
  m.texIndices.push_back(texID);

  return m.matrices.size() - 1;
}

void uploadInstanceData() {
  for (auto& m : models) {
    size_t msize = m.matrices.size() * sizeof(glm::mat4);

    glBindBuffer(GL_ARRAY_BUFFER, m.instanceMatrixVBO);

    if (msize > m.matrixCapacity) {
      m.matrixCapacity = msize * 2;
      glBufferData(GL_ARRAY_BUFFER, m.matrixCapacity, nullptr, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, msize, m.matrices.data());

    size_t tsize = m.texIndices.size() * sizeof(float);

    glBindBuffer(GL_ARRAY_BUFFER, m.instanceTexIndexVBO);

    if (tsize > m.texCapacity) {
      m.texCapacity = tsize * 2;
      glBufferData(GL_ARRAY_BUFFER, m.texCapacity, nullptr, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, tsize, m.texIndices.data());
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void buildMDI() {
  drawCommands.clear();

  GLuint baseInstance = 0;

  for (auto& m : models) {
    if (m.matrices.empty()) continue;

    DrawElementsIndirectCommand cmd;

    cmd.count = m.indexCount;
    cmd.instanceCount = m.matrices.size();
    cmd.firstIndex = 0;
    cmd.baseVertex = 0;
    cmd.baseInstance = baseInstance;

    drawCommands.push_back(cmd);

    baseInstance += m.matrices.size();
  }

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

  glBufferData(GL_DRAW_INDIRECT_BUFFER,
               drawCommands.size() * sizeof(DrawElementsIndirectCommand),
               drawCommands.data(), GL_STATIC_DRAW);
}

void drawScene(glm::mat4 view, glm::mat4 proj) {
  glUseProgram(shaderprogram);

  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayID);
  
	for(int i = 0; i<models.size(); i++){

		ObjModel& m = models[i];
		if(m.matrices.empty()) continue;
		glBindVertexArray(m.VAO);
		glDrawElementsInstanced(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0, m.matrices.size());

	}

}
