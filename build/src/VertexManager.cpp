#include "VertexManager.hpp"

VertexManager::VertexManager() {
	CreateVertexData();
};

VertexManager::~VertexManager() {

};

void VertexManager::CreateVertexData() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

int VertexManager::GetVerticesSize() {
	return this->vertices_.size();
}