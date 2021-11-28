#include "Renderer.hpp"

#include <fstream>
#include <stdexcept>

#include "ShaderModule.hpp"
#include "SingtonManager.hpp"
#include "config/static_config.hpp"
#include "model/Cube.hpp"
#include "spdlog/spdlog.h"

namespace volume_restir {

Renderer::Renderer() {
  float aspect_ratio =
      static_config::kWindowWidth * 1.0f / static_config::kWindowHeight;

  render_context_ = std::make_unique<RenderContext>();
  swapchain_      = std::make_unique<SwapChain>(RenderContextPtr());
  camera_         = std::make_unique<Camera>(
      RenderContextPtr(), static_config::kFOVInDegrees, aspect_ratio);
  scene_ = std::make_unique<Scene>(RenderContextPtr());

  SingletonManager::GetWindow().BindCamera(camera_.get());

  // CreateSwapChain();  // CreateImageViews()
  InitQueues();
  CreateRenderPass();
  CreateCameraDiscriptorSetLayout();
  CreateDescriptorPool();
  CreateCameraDescriptorSet();
  CreateFrameResources();

  CreateGraphicsPipeline();
  CreateCommandPools();
  RecordCommandBuffers();
};

void Renderer::SetFrameBufferResized(bool val) {
  this->frame_buffer_resized_ = val;
}

Renderer::~Renderer() {
  vkDeviceWaitIdle(render_context_->Device().device);
  vkDestroyCommandPool(render_context_->Device().device, graphics_command_pool_,
                       nullptr);
  for (auto framebuffer : framebuffers_) {
    vkDestroyFramebuffer(render_context_->Device().device, framebuffer,
                         nullptr);
  }
  vkDestroyPipeline(render_context_->Device().device, graphics_pipeline_,
                    nullptr);
  vkDestroyPipelineLayout(render_context_->Device().device,
                          graphics_pipeline_layout_, nullptr);

  vkDestroyDescriptorSetLayout(render_context_->Device().device,
                               camera_descriptorset_layout_, nullptr);

  vkDestroyDescriptorPool(render_context_->Device().device, descriptor_pool_,
                          nullptr);

  vkDestroyRenderPass(render_context_->Device().device, render_pass_, nullptr);
  swapchain_->GetVkBSwapChain().destroy_image_views(swapchain_image_views_);
}

void Renderer::InitQueues() {
  // get graphics queue
  auto graphics_queue =
      render_context_->Device().get_queue(vkb::QueueType::graphics);
  if (!graphics_queue.has_value()) {
    spdlog::error("Failed to get graphics queue: {}",
                  graphics_queue.error().message());
    throw std::runtime_error("Failed to get graphics queue");
  }
  queues_[vkq_index::kGraphicsIdx] = graphics_queue.value();
  spdlog::debug("Initialized graphics queue");

  // get present queue
  auto present_queue =
      render_context_->Device().get_queue(vkb::QueueType::present);
  if (!present_queue.has_value()) {
    spdlog::error("Failed to get present queue: {}",
                  present_queue.error().message());
    throw std::runtime_error("Failed to get present queue");
  }
  queues_[vkq_index::kPresentIdx] = present_queue.value();
  spdlog::debug("Initialized present queue");
}

void Renderer::CreateRenderPass() {
  VkAttachmentDescription color_attachment{};
  color_attachment.format         = swapchain_->GetVkBSwapChain().image_format;
  color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment            = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &color_attachment_ref;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments    = &color_attachment;
  render_pass_info.subpassCount    = 1;
  render_pass_info.pSubpasses      = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies   = &dependency;

  if (vkCreateRenderPass(render_context_->Device().device, &render_pass_info,
                         nullptr, &render_pass_) != VK_SUCCESS) {
    spdlog::error("Failed to create render pass!");
    throw std::runtime_error("Failed to create render pass");
  }
  spdlog::debug("Created render pass");
}

void Renderer::CreateGraphicsPipeline() {
  // Create shader module
  VkShaderModule vert_module = ShaderModule::Create(
      std::string(BUILD_DIRECTORY) + "/shaders/graphics.vert.spv",
      render_context_->Device().device);
  VkShaderModule frag_module = ShaderModule::Create(
      std::string(BUILD_DIRECTORY) + "/shaders/graphics.frag.spv",
      render_context_->Device().device);
  if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
    spdlog::error("Failed to create shader module!");
    throw std::runtime_error("Failed to create shader module");
  }
  spdlog::debug("Created shader module");

  // Create Pipelines
  VkPipelineShaderStageCreateInfo vert_stage_info = {};
  vert_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vert_stage_info.module = vert_module;
  vert_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo frag_stage_info = {};
  frag_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_stage_info.module = frag_module;
  frag_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info,
                                                     frag_stage_info};

  // Vertex Input
  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount   = 0;
  vertex_input_info.vertexAttributeDescriptionCount = 0;

  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions    = &bindingDescription;
  vertex_input_info.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

  // Input Assembly
  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  // Viewports and Scissors (rectangles that define in which regions pixels are
  // stored)
  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = (float)swapchain_->GetVkBSwapChain().extent.width;
  viewport.height     = (float)swapchain_->GetVkBSwapChain().extent.height;
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  VkRect2D scissor = {};
  scissor.offset   = {0, 0};
  scissor.extent   = swapchain_->GetVkBSwapChain().extent;

  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports    = &viewport;
  viewport_state.scissorCount  = 1;
  viewport_state.pScissors     = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // Color blending (turned off here, but showing options for learning)
  // --> Configuration per attached framebuffer
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable         = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

  // --> Global color blending settings
  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable     = VK_FALSE;
  color_blending.logicOp           = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount   = 1;
  color_blending.pAttachments      = &colorBlendAttachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  std::vector<VkDescriptorSetLayout> descriptor_set_layouts{
      camera_descriptorset_layout_};

  VkPipelineLayoutCreateInfo pipeline_layout_info = {};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount =
      static_cast<uint32_t>(descriptor_set_layouts.size());
  pipeline_layout_info.pSetLayouts            = descriptor_set_layouts.data();
  pipeline_layout_info.pushConstantRangeCount = 0;
  pipeline_layout_info.pPushConstantRanges    = VK_NULL_HANDLE;

  if (vkCreatePipelineLayout(render_context_->Device().device,
                             &pipeline_layout_info, nullptr,
                             &graphics_pipeline_layout_) != VK_SUCCESS) {
    spdlog::error("Failed to create pipeline layout!");
    throw std::runtime_error("Failed to create pipeline layout");
  }
  spdlog::debug("Created graphics pipeline layout");

  std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamic_info = {};
  dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_info.pDynamicStates    = dynamic_states.data();

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages    = shader_stages;
  pipeline_info.pVertexInputState   = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState      = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState   = &multisampling;
  pipeline_info.pColorBlendState    = &color_blending;
  pipeline_info.pDynamicState       = &dynamic_info;
  pipeline_info.layout              = graphics_pipeline_layout_;
  pipeline_info.renderPass          = render_pass_;
  pipeline_info.subpass             = 0;
  pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(render_context_->Device().device,
                                VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                &graphics_pipeline_) != VK_SUCCESS) {
    spdlog::error("Failed to create pipline!");
    throw std::runtime_error("Failed to create pipline");
  }
  spdlog::debug("Created graphics pipeline");

  vkDestroyShaderModule(render_context_->Device().device, frag_module, nullptr);
  vkDestroyShaderModule(render_context_->Device().device, vert_module, nullptr);

  spdlog::debug(
      "Destroyed unused shader module after creating graphics pipeline");
}

void Renderer::CreateFrameResources() {
  swapchain_image_views_ =
      swapchain_->GetVkBSwapChain().get_image_views().value();

  framebuffers_.resize(swapchain_image_views_.size());

  for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
    VkImageView attachments[] = {swapchain_image_views_[i]};

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments    = attachments;
    framebuffer_info.width  = swapchain_->GetVkBSwapChain().extent.width;
    framebuffer_info.height = swapchain_->GetVkBSwapChain().extent.height;
    framebuffer_info.layers = 1;

    if (vkCreateFramebuffer(render_context_->Device().device, &framebuffer_info,
                            nullptr, &framebuffers_[i]) != VK_SUCCESS) {
      spdlog::error("Failed to create frame buffer {} of {}", i,
                    swapchain_image_views_.size());
      throw std::runtime_error("Failed to create frame buffer");
    }
    spdlog::debug("Created frame buffer [{}] of {}", i,
                  swapchain_image_views_.size());
  }
}

void Renderer::CreateCommandPools() {
  // Create graphics command pool
  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = render_context_->Device()
                                   .get_queue_index(vkb::QueueType::graphics)
                                   .value();

  if (vkCreateCommandPool(render_context_->Device().device, &pool_info, nullptr,
                          &graphics_command_pool_) != VK_SUCCESS) {
    spdlog::error("Failed to create graphics command pool!");
    throw std::runtime_error("Failed to create graphics command pool");
  }
  spdlog::debug("Created graphics command pool");
}

void Renderer::RecordCommandBuffers() {
  command_buffers_.resize(framebuffers_.size());

  // Specify the command pool and number of buffers to allocate
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = graphics_command_pool_;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)command_buffers_.size();

  if (vkAllocateCommandBuffers(render_context_->Device().device, &allocInfo,
                               command_buffers_.data()) != VK_SUCCESS) {
    spdlog::error("Failed to allocate command buffers!");
    throw std::runtime_error("Failed to allocate command buffers");
  }

  // Start command buffer recording
  for (size_t i = 0; i < command_buffers_.size(); i++) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffers_[i], &begin_info) != VK_SUCCESS) {
      spdlog::error("Failed to begin command buffer!");
      throw std::runtime_error("Failed to begin command buffer");
    }

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass  = render_pass_;
    render_pass_info.framebuffer = framebuffers_[i];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_->GetVkBSwapChain().extent;

    VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues    = &clearColor;

    VkViewport viewport = {};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = (float)swapchain_->GetVkBSwapChain().extent.width;
    viewport.height     = (float)swapchain_->GetVkBSwapChain().extent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor = {};
    scissor.offset   = {0, 0};
    scissor.extent   = swapchain_->GetVkBSwapChain().extent;

    vkCmdSetViewport(command_buffers_[i], 0, 1, &viewport);
    vkCmdSetScissor(command_buffers_[i], 0, 1, &scissor);

    // Bind the camera descriptor set. This is set 0 in all pipelines so it will
    // be inherited
    vkCmdBindDescriptorSets(
        command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphics_pipeline_layout_, 0, 1, &camera_descriptorset_, 0, nullptr);

    vkCmdBeginRenderPass(command_buffers_[i], &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline_);

    for (size_t j = 0; j < scene_->GetObjects().size(); ++j) {
      // Bind the vertex and index buffers
      VkBuffer vertexBuffers[] = {scene_->GetObjects()[j]->GetVertexBuffer()};
      VkDeviceSize offsets[]   = {0};
      vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, vertexBuffers, offsets);

      vkCmdBindIndexBuffer(command_buffers_[i],
                           scene_->GetObjects()[j]->GetIndexBuffer(), 0,
                           VK_INDEX_TYPE_UINT32);

      vkCmdDrawIndexed(
          command_buffers_[i],
          static_cast<uint32_t>(scene_->GetObjects()[j]->GetIndices().size()),
          1, 0, 0, 0);
    }

    /*vkCmdDraw(command_buffers_[i],
             static_cast<uint32_t>(vertex_manager_->vertices_.size()), 1, 0,
             0);*/

    vkCmdEndRenderPass(command_buffers_[i]);

    if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS) {
      spdlog::error("Failed to record command buffer {} of {}", i,
                    command_buffers_.size());
      throw std::runtime_error("Failed to record command buffer");
    }
  }
}

void Renderer::RecreateSwapChain() {
  int width  = 0;
  int height = 0;
  do {
    glfwGetFramebufferSize(SingletonManager::GetWindow().WindowPtr(), &width,
                           &height);
    glfwWaitEvents();
  } while (width == 0 || height == 0);

  vkDeviceWaitIdle(render_context_->Device().device);

  // CleanupSwapChain();

  vkDestroyCommandPool(render_context_->Device().device, graphics_command_pool_,
                       nullptr);
  for (auto framebuffer : framebuffers_) {
    vkDestroyFramebuffer(render_context_->Device().device, framebuffer,
                         nullptr);
  }
  swapchain_->GetVkBSwapChain().destroy_image_views(swapchain_image_views_);
  spdlog::debug("Destroyed old swapchain in frame");

  swapchain_->Recreate();
  CreateFrameResources();
  CreateCommandPools();
  RecordCommandBuffers();
}

void Renderer::CleanupSwapChain() {
  for (size_t i = 0; i < static_config::kMaxFrameInFlight; i++) {
    vkDestroyFramebuffer(render_context_->Device().device, framebuffers_[i],
                         nullptr);
  }

  vkFreeCommandBuffers(render_context_->Device().device, graphics_command_pool_,
                       static_cast<uint32_t>(command_buffers_.size()),
                       command_buffers_.data());

  vkDestroyPipeline(render_context_->Device().device, graphics_pipeline_,
                    nullptr);
  vkDestroyPipelineLayout(render_context_->Device().device,
                          graphics_pipeline_layout_, nullptr);
  vkDestroyRenderPass(render_context_->Device().device, render_pass_, nullptr);
  for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
    vkDestroyImageView(render_context_->Device().device,
                       swapchain_image_views_[i], nullptr);
  }

  vkDestroySwapchainKHR(render_context_->Device().device,
                        swapchain_->GetVkBSwapChain().swapchain, nullptr);
}

void Renderer::Draw() {
  vkWaitForFences(render_context_->Device().device, 1,
                  &swapchain_->fences_in_flight_[current_frame_idx_], VK_TRUE,
                  UINT64_MAX);

  VkResult result = swapchain_.get()->Acquire(current_frame_idx_);
  if ((result == VK_ERROR_OUT_OF_DATE_KHR) ||
      (result == VK_SUBOPTIMAL_KHR || frame_buffer_resized_)) {
    frame_buffer_resized_ = false;
    RecreateSwapChain();
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    spdlog::error("Failed to acquire swapchain image. Error {}", result);
    throw std::runtime_error("Failed to acquire swapchain image");
  }
  uint32_t image_index = swapchain_.get()->GetIndex();

  if (swapchain_->images_in_flight_[image_index] != VK_NULL_HANDLE) {
    vkWaitForFences(render_context_->Device().device, 1,
                    &swapchain_->images_in_flight_[image_index], VK_TRUE,
                    UINT64_MAX);
  }
  swapchain_->images_in_flight_[image_index] =
      swapchain_->fences_in_flight_[current_frame_idx_];

  VkSubmitInfo submitInfo = {};
  submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {
      swapchain_->available_semaphores_[current_frame_idx_]};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = wait_semaphores;
  submitInfo.pWaitDstStageMask  = wait_stages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &command_buffers_[image_index];

  VkSemaphore signal_semaphores[] = {
      swapchain_->finished_semaphores_[current_frame_idx_]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signal_semaphores;

  vkResetFences(render_context_->Device().device, 1,
                &swapchain_->fences_in_flight_[current_frame_idx_]);

  if (vkQueueSubmit(queues_[vkq_index::kGraphicsIdx], 1, &submitInfo,
                    swapchain_->fences_in_flight_[current_frame_idx_]) !=
      VK_SUCCESS) {
    spdlog::error("Failed to submit draw command buffer");
    throw std::runtime_error("Failed to submit draw command buffer");
  }

  VkPresentInfoKHR present_info = {};
  present_info.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;

  VkSwapchainKHR swapChains[] = {swapchain_->GetVkBSwapChain().swapchain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains    = swapChains;

  present_info.pImageIndices = &image_index;

  result = vkQueuePresentKHR(queues_[vkq_index::kPresentIdx], &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    RecreateSwapChain();
  } else if (result != VK_SUCCESS) {
    spdlog::error("Failed to present swapchain image");
    throw std::runtime_error("Failed to present swapchain image");
  }

  current_frame_idx_ =
      (current_frame_idx_ + 1) % static_config::kMaxFrameInFlight;
}

RenderContext* Renderer::RenderContextPtr() const noexcept {
  return render_context_.get();
}

void Renderer::CreateCameraDiscriptorSetLayout() {
  // Describe the binding of the descriptor set layout
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding                      = 0;
  uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount    = 1;
  uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_ALL;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  std::vector<VkDescriptorSetLayoutBinding> bindings = {uboLayoutBinding};

  // Create the descriptor set layout
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  if (vkCreateDescriptorSetLayout(render_context_->Device().device, &layoutInfo,
                                  nullptr, &camera_descriptorset_layout_) !=
      VK_SUCCESS) {
    spdlog::error("Failed to create camera descriptor set layout");
    throw std::runtime_error("Failed to create camera descriptor set layout");
  }
}

void Renderer::CreateDescriptorPool() {
  // Describe which descriptor types that the descriptor sets will contain
  std::vector<VkDescriptorPoolSize> poolSizes = {
      // Camera
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = 5;

  if (vkCreateDescriptorPool(render_context_->Device().device, &poolInfo,
                             nullptr, &descriptor_pool_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool");
  }
}

void Renderer::CreateCameraDescriptorSet() {
  // Describe the descriptor set
  VkDescriptorSetLayout layouts[]       = {camera_descriptorset_layout_};
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = descriptor_pool_;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = layouts;

  // Allocate descriptor sets
  if (vkAllocateDescriptorSets(render_context_->Device().device, &allocInfo,
                               &camera_descriptorset_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate camera descriptor set");
  }

  // Configure the descriptors to refer to buffers
  VkDescriptorBufferInfo cameraBufferInfo = {};
  cameraBufferInfo.buffer                 = camera_->GetBuffer();
  cameraBufferInfo.offset                 = 0;
  cameraBufferInfo.range                  = sizeof(CameraBufferObject);

  std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
  descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet           = camera_descriptorset_;
  descriptorWrites[0].dstBinding       = 0;
  descriptorWrites[0].dstArrayElement  = 0;
  descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount  = 1;
  descriptorWrites[0].pBufferInfo      = &cameraBufferInfo;
  descriptorWrites[0].pImageInfo       = nullptr;
  descriptorWrites[0].pTexelBufferView = nullptr;

  // Update descriptor sets
  vkUpdateDescriptorSets(render_context_->Device().device,
                         static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

}  // namespace volume_restir
