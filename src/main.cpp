/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

// ImGui - standalone example application for Glfw + Vulkan, using programmable
// pipeline If you are new to ImGui, see examples/README.txt and documentation
// at the top of imgui.cpp.

#include <array>
#include <filesystem>

#include "Renderer.h"
#include "SingletonManager.hpp"
#include "backends/imgui_impl_glfw.h"
#include "config/build_config.h"
#include "config/static_config.hpp"
#include "imgui.h"
#include "imgui/imgui_camera_widget.h"
#include "nvh/cameramanipulator.hpp"
#include "nvh/fileoperations.hpp"
#include "nvpsystem.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/context_vk.hpp"

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////
#define UNUSED(x) (void)(x)
//////////////////////////////////////////////////////////////////////////

// Default search path for shaders
std::vector<std::string> defaultSearchPaths;

const std::string vdb_filename = "cube.vdb";
const fs::path asset_dir = fs::path(PROJECT_DIRECTORY) / fs::path("assets");
const std::string file   = (asset_dir / vdb_filename).string();

const std::string gltf_sponza  = "media/gltf/Sponza/glTF/Sponza.gltf";
const std::string gltf_cornell = "media/gltf/cornellBox/cornellBox.gltf";

// GLFW Callback functions
static void onErrorCallback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Extra UI
void renderUI(Renderer& renderer) {
  ImGuiH::CameraWidget();
  if (ImGui::CollapsingHeader("Light")) {
    ImGui::RadioButton("Point", &renderer.getPushConstant().lightType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Infinite", &renderer.getPushConstant().lightType, 1);

    ImGui::SliderFloat3("Position", &renderer.getPushConstant().lightPosition.x,
                        -20.f, 20.f);
    ImGui::SliderFloat("Intensity", &renderer.getPushConstant().lightIntensity,
                       0.f, 150.f);
  }
  ImGui::Text("Nb Spheres and Cubes: %llu", renderer.getSpheres().size());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static int const SAMPLE_WIDTH  = 1280;
static int const SAMPLE_HEIGHT = 720;

//--------------------------------------------------------------------------------------------------
// Application Entry
//
int main(int argc, char** argv) {
  UNUSED(argc);

#ifdef VOLUME_RESTIR_USE_VDB
  SingletonManager::GetVDBLoader().Load(file);
#endif  // VOLUME_RESTIR_USE_VDB

  // Setup GLFW window
  glfwSetErrorCallback(onErrorCallback);
  if (!glfwInit()) {
    return 1;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(SAMPLE_WIDTH, SAMPLE_HEIGHT,
                                        PROJECT_NAME, nullptr, nullptr);

  // Setup camera
  CameraManip.setWindowSize(SAMPLE_WIDTH, SAMPLE_HEIGHT);
  CameraManip.setLookat(nvmath::vec3f(20, 20, 20), nvmath::vec3f(0, 1, 0),
                        nvmath::vec3f(0, 1, 0));

  // Setup Vulkan
  if (!glfwVulkanSupported()) {
    printf("GLFW: Vulkan Not Supported\n");
    return 1;
  }

  // setup some basic things for the sample, logging file for example
  NVPSystem system(PROJECT_NAME);

  // Search path for shaders and other media
  defaultSearchPaths = {
      NVPSystem::exePath() + PROJECT_RELDIRECTORY,
      NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
      std::string(PROJECT_NAME),
  };

  // Vulkan required extensions
  assert(glfwVulkanSupported() == 1);
  uint32_t count{0};
  auto reqExtensions = glfwGetRequiredInstanceExtensions(&count);

  // Requesting Vulkan extensions and layers
  nvvk::ContextCreateInfo contextInfo;
  contextInfo.setVersion(1, 2);
  // Adding required extensions (surface, win32, linux, ..)
  for (uint32_t ext_id = 0; ext_id < count; ext_id++) {
    contextInfo.addInstanceExtension(reqExtensions[ext_id]);
  }

  contextInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor",
                               true);  // FPS in titlebar
  contextInfo.addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                                   true);  // Allow debug names

  contextInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef WIN32
  contextInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
  contextInfo.addInstanceExtension(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
  contextInfo.addInstanceExtension(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
  contextInfo.addInstanceExtension(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  contextInfo.addDeviceExtension(
      VK_KHR_SWAPCHAIN_EXTENSION_NAME);  // Enabling ability to present,
                                         // rendering

  // #VKRay: Activate the ray tracing extension
  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
  contextInfo.addDeviceExtension(
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false,
      &accelFeature);  // To build acceleration structures

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
  contextInfo.addDeviceExtension(
      VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false,
      &rtPipelineFeature);  // To use vkCmdTraceRaysKHR

  contextInfo.addDeviceExtension(
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);  // Required by ray,
                                                        // tracing pipeline
  contextInfo.addDeviceExtension(
      VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
  contextInfo.addDeviceExtension(
      VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);  // #VKRay: Activate the
                                                         // ray tracing
                                                         // extension
  contextInfo.addDeviceExtension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);

  // Creating Vulkan base application
  nvvk::Context vkctx{};
  vkctx.initInstance(contextInfo);
  // Find all compatible devices
  auto compatibleDevices = vkctx.getCompatibleDevices(contextInfo);
  assert(!compatibleDevices.empty());
  // Use a compatible device
  vkctx.initDevice(compatibleDevices[0], contextInfo);

  // Create example
  Renderer renderer;

  // Window need to be opened to get the surface on which to draw
  const VkSurfaceKHR surface = renderer.getVkSurface(vkctx.m_instance, window);
  vkctx.setGCTQueueWithPresent(surface);

  renderer.setup(vkctx.m_instance, vkctx.m_device, vkctx.m_physicalDevice,
                 vkctx.m_queueGCT.familyIndex);
  renderer.createSwapchain(surface, SAMPLE_WIDTH, SAMPLE_HEIGHT);
  // global things, handled by AppBase
  renderer.createDepthBuffer();
  renderer.createRenderPass();
  renderer.createFrameBuffers();

  // Setup Imgui
  renderer.initGUI(0);  // Using sub-pass 0

#ifdef USE_GLTF
  renderer.loadGLTFModel(nvh::findFile(gltf_cornell, defaultSearchPaths, true));
  renderer.createGLTFBuffer();
#else
  // Creation of the example
  //  renderer.loadModel(nvh::findFile("media/scenes/Medieval_building.obj",
  //  defaultSearchPaths, true));
  renderer.loadModel(
      nvh::findFile("media/scenes/plane.obj", defaultSearchPaths, true));
  renderer.createObjDescriptionBuffer();
#endif

#ifdef VOLUME_RESTIR_USE_VDB
  renderer.createVDBBuffer();
#endif  // VOLUME_RESTIR_USE_VDB

  renderer.createOffscreenRender();
  renderer.createDescriptorSetLayout();
  renderer.createGraphicsPipeline();
  renderer.createUniformBuffer();
  renderer.updateDescriptorSet();

  // GBuffers
  renderer.createGBuffers();

  // #VKRay
  renderer.initRayTracing();
  renderer.createBottomLevelAS();
  renderer.createTopLevelAS();
  renderer.createRtDescriptorSet();
  renderer.createRtPipeline();  // Binding m_rtDescSetLayout, m_descSetLayout
  renderer.createRtShaderBindingTable();

  renderer.createPostDescriptor();
  renderer.createPostPipeline();  // Push Constant float // Binding
                                  // m_postDescSetLayout
  renderer.updatePostDescriptorSet();

  // # ReSTIR pipeline toggle
  // restir lights
  renderer.createRestirLights();
  renderer.createLightDescriptorSet();

  // restir uniforms
  renderer.createRestirUniformBuffer();
  renderer.createRestirUniformDescriptorSet();
  renderer.updateRestirUniformDescriptorSet();

  // restir reservoirs
  renderer.createRestirBuffer();
  renderer.createRestirDescriptorSet();

  // Post descriptor set for ReSTIR
  renderer.createRestirPostDescriptor();
  renderer.updateRestirPostDescriptorSet();

  renderer.createRestirPipeline();  // Binding rtDescSetLayout, descSetLayout,
                                    // uniformDescSetLayout, lightDescSetLayout,
                                    // restirDescSetLayout
  renderer
      .createSpatialReusePipeline();  // Binding rtDescSetLayout, descSetLayout,
                                      // uniformDescSetLayout,
                                      // lightDescSetLayout, restirDescSetLayout
  renderer.createRestirPostPipeline();  // Push Constant PushConstantRestirPost
                                        // Binding m_restirUniformDescSetLayout,
                                        // m_lightDescSetLayout,
                                        // m_restirDescSetLayout,
                                        // m_restirPostDescSetLayout
  renderer.updateRestirDescriptorSet();

  nvmath::vec4f clearColor = nvmath::vec4f(1, 1, 1, 1.00f);
  bool useRaytracer        = true;

  renderer.setupGlfwCallbacks(window);
  ImGui_ImplGlfw_InitForVulkan(window, true);

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    if (renderer.isMinimized()) continue;

    // Start the Dear ImGui frame
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Show UI window.
    if (renderer.showGui()) {
      ImGuiH::Panel::Begin();
      ImGui::ColorEdit3("Clear color", reinterpret_cast<float*>(&clearColor));
      ImGui::Checkbox("Ray Tracer mode",
                      &useRaytracer);  // Switch between raster and ray tracing

      renderUI(renderer);
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGuiH::Control::Info("", "", "(F10) Toggle Pane",
                            ImGuiH::Control::Flags::Disabled);
      ImGuiH::Panel::End();
    }

    // Start rendering the scene
    renderer.prepareFrame();

    // Start command buffer of this frame
    auto curFrame                 = renderer.getCurFrame();
    const VkCommandBuffer& cmdBuf = renderer.getCommandBuffers()[curFrame];

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuf, &beginInfo);

    // Updating camera buffer
    renderer.updateUniformBuffer(cmdBuf);
    renderer.updateRestirUniformBuffer(cmdBuf);

    // Clearing screen
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {
        {clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};
    clearValues[1].depthStencil = {1.0f, 0};

#ifdef USE_RT_PIPELINE
    // Offscreen render pass
    {
      VkRenderPassBeginInfo offscreenRenderPassBeginInfo{
          VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      offscreenRenderPassBeginInfo.clearValueCount = 2;
      offscreenRenderPassBeginInfo.pClearValues    = clearValues.data();
      offscreenRenderPassBeginInfo.renderPass =
          renderer.getOffscreenRenderPass();
      offscreenRenderPassBeginInfo.framebuffer =
          renderer.getOffscreenFrameBuffer();
      offscreenRenderPassBeginInfo.renderArea = {{0, 0}, renderer.getSize()};

      // Rendering Scene
      if (useRaytracer) {
        renderer.raytrace(cmdBuf, clearColor);
      } else {
        vkCmdBeginRenderPass(cmdBuf, &offscreenRenderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        renderer.rasterize(cmdBuf);
        vkCmdEndRenderPass(cmdBuf);
      }
    }

    // Ray Tracing Pipeline post processing
    // 2nd rendering pass: tone mapper, UI
    {
      VkRenderPassBeginInfo postRenderPassBeginInfo{
          VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      postRenderPassBeginInfo.clearValueCount = 2;
      postRenderPassBeginInfo.pClearValues    = clearValues.data();
      postRenderPassBeginInfo.renderPass      = renderer.getRenderPass();
      postRenderPassBeginInfo.framebuffer =
          renderer.getFramebuffers()[curFrame];
      postRenderPassBeginInfo.renderArea = {{0, 0}, renderer.getSize()};

      // Rendering tonemapper
      vkCmdBeginRenderPass(cmdBuf, &postRenderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);
      renderer.drawPost(cmdBuf);
      // Rendering UI
      ImGui::Render();
      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
      vkCmdEndRenderPass(cmdBuf);
    }
#endif
#ifdef USE_RESTIR_PIPELINE
    // restir pass
    {
      renderer.getRestirPass().run(
          cmdBuf, renderer.getRtDescSet(), renderer.getDescSet(),
          renderer.getRestirUniformDescSet(), renderer.getLightDescSet(),
          renderer.getRestirDescSet(), clearColor);
      // renderer.getSpatialReusePass().run(
      //     cmdBuf, renderer.getRtDescSet(), renderer.getDescSet(),
      //     renderer.getRestirUniformDescSet(), renderer.getLightDescSet(),
      //     renderer.getRestirDescSet());
    }

    // Restir Pipeline post processing
    {
      VkRenderPassBeginInfo restirPostRenderPassBeginInfo{
          VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      restirPostRenderPassBeginInfo.clearValueCount = 2;
      restirPostRenderPassBeginInfo.pClearValues    = clearValues.data();
      restirPostRenderPassBeginInfo.renderPass      = renderer.getRenderPass();
      restirPostRenderPassBeginInfo.framebuffer =
          renderer.getFramebuffers()[curFrame];
      restirPostRenderPassBeginInfo.renderArea = {{0, 0}, renderer.getSize()};

      // Rendering tonemapper
      vkCmdBeginRenderPass(cmdBuf, &restirPostRenderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);
      renderer.restirDrawPost(cmdBuf);
      // Rendering UI
      ImGui::Render();
      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
      vkCmdEndRenderPass(cmdBuf);
    }
#endif

    // Submit for display
    vkEndCommandBuffer(cmdBuf);
    renderer.submitFrame();
    renderer.updateGBufferFrameIdx();
  }

  // Cleanup
  vkDeviceWaitIdle(renderer.getDevice());

  renderer.destroyResources();
  renderer.destroy();
  vkctx.deinit();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
