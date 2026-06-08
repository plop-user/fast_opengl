#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

// File order: +X, -X, +Y, -Y, +Z, -Z  (right, left, top, bottom, back, front)
void initskybox(const std::vector<std::string> &paths);
void drawskybox(const glm::mat4 &view, const glm::mat4 &projection);
