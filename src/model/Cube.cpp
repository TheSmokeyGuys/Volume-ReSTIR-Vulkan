#include "Cube.hpp"

namespace volume_restir {

void Cube::DefaultInit() {
  std::vector<Vertex> cube_vertices = {
      {{-0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.f}, {1.0f, 1.0f, 1.0f}}};

  std::vector<uint32_t> cube_indices = {0, 1, 2, 2, 3, 0};

  vertices_ = cube_vertices;
  indices_  = cube_indices;
}

}  // namespace volume_restir
