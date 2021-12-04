#pragma once

#include <vulkan/vulkan.hpp>

#include "../util.h"
#include "nvh/fileoperations.hpp"
#include "nvvk/shaders_vk.hpp"
#include "../sceneBuffers.h"

#include "nvvk/raytraceKHR_vk.hpp"
#include "nvh/alignment.hpp"
//#include "GBuffer.hpp"


class SpatialReusePass {
public:
	void setup(const vk::Device& device, const vk::PhysicalDevice&, uint32_t graphicsQueueIndex, nvvk::Allocator* allocator);

	void createDescriptorSet() {};
	void createRenderPass(vk::Extent2D outputSize);
	void createPipeline(const vk::DescriptorSetLayout& sceneDescSetLayout, const vk::DescriptorSetLayout& lightDescSetLayout,const vk::DescriptorSetLayout& restirDescSetLayout);

	bool uiSetup() {};
	void run(const vk::CommandBuffer& cmdBuf, const vk::DescriptorSet& sceneDescSet, const vk::DescriptorSet& lightDescSet, const vk::DescriptorSet& restirDescSet);

	void destroy();

private:
	vk::Device m_device;
	vk::PhysicalDevice m_physicalDevice;
	uint32_t m_graphicsQueueIndex;
	nvvk::Allocator* m_alloc;
	vk::Extent2D m_size;

	vk::PipelineLayout m_pipelineLayout;
	vk::Pipeline     m_pipeline;

	vk::RenderPass     m_renderPass;

	const nvh::GltfScene* m_scene = nullptr;
	SceneBuffers* m_sceneBuffers = nullptr;


};