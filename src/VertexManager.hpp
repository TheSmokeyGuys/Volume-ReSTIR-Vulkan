#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 col;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2>
		getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2>
			attributeDescriptions{};

		return attributeDescriptions;
	}

} ;

class VertexManager {
private:
	void CreateVertexData();

public:
	VertexManager();
	~VertexManager(); 

	const std::vector<Vertex> vertices_ = {
		{ {-0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}},
		{ {0.5f, -0.5f, 0.f}, {0.0f, 1.0f, 0.0f}},
		{ {0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}},
		{ {-0.5f, 0.5f, 0.f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint32_t> indices_ = {
		0, 1, 2, 2, 3, 0
	};


	int GetVerticesSize();

};