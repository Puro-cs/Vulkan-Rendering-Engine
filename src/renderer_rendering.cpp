/* Copyright (c) 2025 Holochip Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
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
 */
#include "imgui/imgui.h"
#include "imgui_system.h"
#include "light_component.h"
#include "mesh_component.h"
#include "model_loader.h"
#include "renderer.h"
#include "transform_component.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/gtx/norm.hpp>
#include <iomanip>
#include <iostream>
#include <map>
#include <ranges>
#include <sstream>
#include <stdexcept>

// ===================== Culling helpers implementation =====================

Renderer::FrustumPlanes Renderer::extractFrustumPlanes(const glm::mat4& vp) {
  // Work in row-major form for standard plane extraction by transposing GLM's column-major matrix
  glm::mat4 m = glm::transpose(vp);
  FrustumPlanes fp{};
  // Left   : m[3] + m[0]
  fp.planes[0] = m[3] + m[0];
  // Right  : m[3] - m[0]
  fp.planes[1] = m[3] - m[0];
  // Bottom : m[3] + m[1]
  fp.planes[2] = m[3] + m[1];
  // Top    : m[3] - m[1]
  fp.planes[3] = m[3] - m[1];
  // Near   : m[2] (matches Vulkan [0, 1] clip range)
  fp.planes[4] = m[2];
  // Far    : m[3] - m[2]
  fp.planes[5] = m[3] - m[2];

  // Normalize planes
  for (auto& p : fp.planes) {
    glm::vec3 n(p.x, p.y, p.z);
    float len = glm::length(n);
    if (len > 0.0f) {
      p /= len;
    }
  }
  return fp;
}

void Renderer::transformAABB(const glm::mat4& M,
                             const glm::vec3& localMin,
                             const glm::vec3& localMax,
                             glm::vec3& outMin,
                             glm::vec3& outMax) {
  // OBB (from model) to world AABB using center/extents and absolute 3x3
  const glm::vec3 c = 0.5f * (localMin + localMax);
  const glm::vec3 e = 0.5f * (localMax - localMin);

  const glm::vec3 worldCenter = glm::vec3(M * glm::vec4(c, 1.0f));
  // Upper-left 3x3
  const glm::mat3 A = glm::mat3(M);
  const glm::mat3 AbsA = glm::mat3(glm::abs(A[0]), glm::abs(A[1]), glm::abs(A[2]));
  const glm::vec3 worldExtents = AbsA * e; // component-wise combination

  outMin = worldCenter - worldExtents;
  outMax = worldCenter + worldExtents;
}

bool Renderer::aabbIntersectsFrustum(const glm::vec3& worldMin,
                                     const glm::vec3& worldMax,
                                     const FrustumPlanes& frustum) {
  // Use the p-vertex test against each plane; if outside any plane → culled
  for (const auto& p : frustum.planes) {
    const glm::vec3 n(p.x, p.y, p.z);
    // Choose positive vertex (furthest in direction of normal)
    glm::vec3 v{
      n.x >= 0.0f ? worldMax.x : worldMin.x,
      n.y >= 0.0f ? worldMax.y : worldMin.y,
      n.z >= 0.0f ? worldMax.z : worldMin.z
    };

    // If the most positive vertex is still on the negative side of the plane,
    // then the entire box is on the negative side.
    // Use a small epsilon to avoid numerical issues.
    if (glm::dot(n, v) + p.w < -0.01f) {
      return false; // completely outside
    }
  }
  return true;
}

// This file contains rendering-related methods from the Renderer class

// Create swap chain
bool Renderer::createSwapChain() {
  try {
    // Query swap chain support
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    // Choose swap surface format, present mode, and extent
    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Choose image count
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Choose preTransform. On Android, eIdentity is preferred if supported to let the system handle rotation.
    vk::SurfaceTransformFlagBitsKHR preTransform;
    if (swapChainSupport.capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
      preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    } else {
      preTransform = swapChainSupport.capabilities.currentTransform;
    }

    // Create swap chain info
    vk::SwapchainCreateInfoKHR createInfo{
      .surface = *surface,
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
      .preTransform = preTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = nullptr
    };

    // Find queue families
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    std::array<uint32_t, 2> queueFamilyIndicesLoc = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    // Set sharing mode
    if (indices.graphicsFamily != indices.presentFamily) {
      createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndicesLoc.size());
      createInfo.pQueueFamilyIndices = queueFamilyIndicesLoc.data();
    } else {
      createInfo.imageSharingMode = vk::SharingMode::eExclusive;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices = nullptr;
    }

    // Create swap chain
    swapChain = vk::raii::SwapchainKHR(device, createInfo);

    // Get swap chain images
    swapChainImages = swapChain.getImages();

    // Swapchain images start in UNDEFINED layout; track per-image layout for correct barriers.
    swapChainImageLayouts.assign(swapChainImages.size(), vk::ImageLayout::eUndefined);

    // Store swap chain format and extent
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create swap chain: " << e.what() << std::endl;
    return false;
  }
}

// Create image views
bool Renderer::createImageViews() {
  try {
    opaqueSceneColorImages.clear();
    opaqueSceneColorImageAllocations.clear();
    opaqueSceneColorImageViews.clear();
    opaqueSceneColorImageLayouts.clear();
    opaqueSceneColorSampler.clear();
    // Resize image views vector
    swapChainImageViews.clear();
    swapChainImageViews.reserve(swapChainImages.size());

    // Create image view info template (image will be set per iteration)
    vk::ImageViewCreateInfo createInfo{
      .viewType = vk::ImageViewType::e2D,
      .format = swapChainImageFormat,
      .components = {
        .r = vk::ComponentSwizzle::eIdentity,
        .g = vk::ComponentSwizzle::eIdentity,
        .b = vk::ComponentSwizzle::eIdentity,
        .a = vk::ComponentSwizzle::eIdentity
      },
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
    };

    // Create image view for each swap chain image
    for (const auto& image : swapChainImages) {
      createInfo.image = image;
      swapChainImageViews.emplace_back(device, createInfo);
    }

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create image views: " << e.what() << std::endl;
    return false;
  }
}

// Setup dynamic rendering
bool Renderer::setupDynamicRendering() {
  try {
    // Create color attachment
    colorAttachments = {
      vk::RenderingAttachmentInfo{
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}) // Black default

      }
    };

    // Create depth attachment
    depthAttachment = vk::RenderingAttachmentInfo{
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearDepthStencilValue(1.0f, 0)
    };

    // Create rendering info
    renderingInfo = vk::RenderingInfo{
      .renderArea = vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent),
      .layerCount = 1,
      .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
      .pColorAttachments = colorAttachments.data(),
      .pDepthAttachment = &depthAttachment
    };

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to setup dynamic rendering: " << e.what() << std::endl;
    return false;
  }
}

// Create command pool
bool Renderer::createCommandPool() {
  try {
    // Find queue families
    QueueFamilyIndices queueFamilyIndicesLoc = findQueueFamilies(physicalDevice);

    // Create command pool info
    vk::CommandPoolCreateInfo poolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueFamilyIndicesLoc.graphicsFamily.value()
    };

    // Create command pool
    commandPool = vk::raii::CommandPool(device, poolInfo);

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create command pool: " << e.what() << std::endl;
    return false;
  }
}

// Create command buffers
bool Renderer::createCommandBuffers() {
  try {
    // Resize command buffers vector
    commandBuffers.clear();
    commandBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

    // Create command buffer allocation info
    vk::CommandBufferAllocateInfo allocInfo{
      .commandPool = *commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
    };

    // Allocate command buffers
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create command buffers: " << e.what() << std::endl;
    return false;
  }
}

// Create sync objects
bool Renderer::createSyncObjects() {
  try {
    // Resize semaphores and fences vectors
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();

    // Semaphores per swapchain image (indexed by imageIndex from acquireNextImage)
    // The presentation engine holds semaphores until the image is re-acquired, so we need
    // one semaphore per swapchain image to avoid reuse conflicts. See Vulkan spec:
    // https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    const auto semaphoreCount = static_cast<uint32_t>(swapChainImages.size());
    imageAvailableSemaphores.reserve(semaphoreCount);
    renderFinishedSemaphores.reserve(semaphoreCount);

    // Fences per frame-in-flight for CPU-GPU synchronization (indexed by currentFrame)
    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    // Create semaphore info
    vk::SemaphoreCreateInfo semaphoreInfo{};

    // Create semaphores per swapchain image (indexed by imageIndex for presentation sync)
    for (uint32_t i = 0; i < semaphoreCount; i++) {
      imageAvailableSemaphores.emplace_back(device, semaphoreInfo);
      renderFinishedSemaphores.emplace_back(device, semaphoreInfo);
    }

    // Create fences per frame-in-flight (indexed by currentFrame for CPU-GPU pacing)
    vk::FenceCreateInfo fenceInfo{
      .flags = vk::FenceCreateFlagBits::eSignaled
    };
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      inFlightFences.emplace_back(device, fenceInfo);
    }

    // Ensure uploads timeline semaphore exists (created early in createLogicalDevice)
    // No action needed here unless reinitializing after swapchain recreation.
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create sync objects: " << e.what() << std::endl;
    return false;
  }
}

// Clean up swap chain
void Renderer::cleanupSwapChain() {
  // Clean up depth resources
  depthImageView = vk::raii::ImageView(nullptr);
  depthImage = vk::raii::Image(nullptr);
  depthImageAllocation = nullptr;

  // Clean up swap chain image views
  swapChainImageViews.clear();

  // Note: Keep descriptor pool alive here to ensure descriptor sets remain valid during swapchain recreation.
  // descriptorPool is preserved; it will be managed during full renderer teardown.

  // Clean up pipelines
  pbrGraphicsPipeline = vk::raii::Pipeline(nullptr);

  // Clean up pipeline layouts
  pbrPipelineLayout = vk::raii::PipelineLayout(nullptr);

  // Clean up sync objects (they need to be recreated with new swap chain image count)
  imageAvailableSemaphores.clear();
  renderFinishedSemaphores.clear();
  inFlightFences.clear();

  // Clean up swap chain
  swapChain = vk::raii::SwapchainKHR(nullptr);
}

// Recreate swap chain
void Renderer::recreateSwapChain() {
  // Prevent background uploads worker from mutating descriptors while we rebuild
  StopUploadsWorker();

  // Block descriptor writes while we rebuild swapchain and descriptor pools
  descriptorSetsValid.store(false, std::memory_order_relaxed); {
    // Drop any deferred descriptor updates that target old descriptor sets
    std::lock_guard<std::mutex> lk(pendingDescMutex);
    pendingDescOps.clear();
    descriptorRefreshPending.store(false, std::memory_order_relaxed);
  }

  // Wait for all frames in flight to complete before recreating the swap chain
  std::vector<vk::Fence> allFences;
  allFences.reserve(inFlightFences.size());
  for (const auto& fence : inFlightFences) {
    allFences.push_back(*fence);
  }
  if (!allFences.empty()) {
    vk::Result result = waitForFencesSafe(allFences, VK_TRUE);
    if (result != vk::Result::eSuccess) {
      std::cerr << "Error: Failed to wait for in-flight fences during swap chain recreation: " << vk::to_string(result) << std::endl;
    }
  }

  // Wait for the device to be idle before recreating the swap chain
  // External synchronization required (VVL): serialize against queue submits/present.
  WaitIdle();

  // Clean up old swap chain resources
  cleanupSwapChain();

  // Recreate swap chain and related resources
  createSwapChain();
  createImageViews();
  setupDynamicRendering();
  createDepthResources();

  // Recreate sync objects with correct sizing for new swap chain
  createSyncObjects();

  // Recreate off-screen opaque scene color and descriptor sets needed by transparent pass
  createOpaqueSceneColorResources();
  createTransparentDescriptorSets();
  createTransparentFallbackDescriptorSets();

  // Wait for all command buffers to complete before clearing resources
  for (const auto& fence : inFlightFences) {
    vk::Result result = waitForFencesSafe(*fence, VK_TRUE);
    if (result != vk::Result::eSuccess) {
      std::cerr << "Error: Failed to wait for fence before clearing resources: " << vk::to_string(result) << std::endl;
    }
  }

  // Clear all entity descriptor sets since they're now invalid (allocated from the old pool)
  {
    // Serialize descriptor frees against any other descriptor operations
    std::lock_guard<std::mutex> lk(descriptorMutex);
    for (auto& kv : entityResources) {
      auto& resources = kv.second;
      resources.pbrDescriptorSets.clear();
      // Descriptor initialization flags must be reset because new descriptor sets
      // will be allocated and only the current frame will be initialized at runtime.
      resources.pbrUboBindingWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
      resources.pbrImagesWritten.assign(MAX_FRAMES_IN_FLIGHT, false);
    }
  }

  createPBRPipeline();
  createCompositePipeline();
  // Rebuild the named pipelines from their descriptions (they use the PBR pipeline layouts created above)
  for (auto& namedPipeline : namedPipelines) {
    createNamedPipeline(namedPipeline);
  }

  // Re-create command buffers to ensure fresh recording against new swapchain state
  commandBuffers.clear();
  createCommandBuffers();
  currentFrame = 0;

  // Recreate descriptor sets for all entities after swapchain/pipeline rebuild
  for (const auto& kv : entityResources) {
    const auto& entity = kv.first;
    if (!entity)
      continue;
    auto meshComponent = entity->GetComponent<MeshComponent>();
    if (!meshComponent)
      continue;

    std::string texturePath = meshComponent->GetTexturePath();
    // Fallback: use baseColor when legacy path is empty
    if (texturePath.empty()) {
      const std::string& baseColor = meshComponent->GetBaseColorTexturePath();
      if (!baseColor.empty()) {
        texturePath = baseColor;
      }
    }
    // Recreate PBR descriptor sets
    createDescriptorSets(entity, texturePath, true);
  }

  // Descriptor sets are now valid again
  descriptorSetsValid.store(true, std::memory_order_relaxed);

  // Resume background uploads worker now that swapchain and descriptors are recreated
  StartUploadsWorker();
}

void Renderer::prepareFrameUboTemplate(CameraComponent* camera) {
  frameUboTemplate = UniformBufferObject{};
  if (!camera) return;

  frameUboTemplate.view = camera->GetViewMatrix();
  frameUboTemplate.proj = camera->GetProjectionMatrix();
  frameUboTemplate.proj[1][1] *= -1; // Flip Y for Vulkan
  frameUboTemplate.camPos = glm::vec4(camera->GetPosition(), 1.0f);

  frameUboTemplate.lightCount = static_cast<int>(lastFrameLightCount);
  frameUboTemplate.exposure = std::clamp(this->exposure, 0.2f, 4.0f);
  frameUboTemplate.gamma = this->gamma;
  // Match raster convention: ambient scale factor for simple IBL/ambient term.
  frameUboTemplate.scaleIBLAmbient = 1.0f;
  frameUboTemplate.screenDimensions = glm::vec2(swapChainExtent.width, swapChainExtent.height);
  frameUboTemplate.nearZ = camera->GetNearPlane();
  frameUboTemplate.farZ = camera->GetFarPlane();

  int outputIsSRGB = (swapChainImageFormat == vk::Format::eR8G8B8A8Srgb ||
                       swapChainImageFormat == vk::Format::eB8G8R8A8Srgb) ? 1 : 0;
  frameUboTemplate.padding0 = outputIsSRGB;
}

// Update uniform buffer
void Renderer::updateUniformBuffer(uint32_t currentImage, Entity* entity, EntityResources* entityRes, CameraComponent* camera, TransformComponent* tc) {
  if (!entityRes) {
    return;
  }

  // Get transform component
  auto transformComponent = tc ? tc : (entity ? entity->GetComponent<TransformComponent>() : nullptr);
  if (!transformComponent) {
    return;
  }

  // Create uniform buffer object
  UniformBufferObject ubo{};
  ubo.model = transformComponent->GetModelMatrix();
  ubo.view = camera->GetViewMatrix();
  ubo.proj = camera->GetProjectionMatrix();
  ubo.proj[1][1] *= -1; // Flip Y for Vulkan

  // Continue with the rest of the uniform buffer setup
  updateUniformBufferInternal(currentImage, entity, entityRes, camera, ubo);
}

// Overloaded version that accepts a custom transform matrix
void Renderer::updateUniformBuffer(uint32_t currentImage, Entity* entity, EntityResources* entityRes, CameraComponent* camera, const glm::mat4& customTransform) {
  if (!entityRes) return;
  // Create the uniform buffer object with custom transform
  UniformBufferObject ubo{};
  ubo.model = customTransform;
  ubo.view = camera->GetViewMatrix();
  ubo.proj = camera->GetProjectionMatrix();
  ubo.proj[1][1] *= -1; // Flip Y for Vulkan

  // Continue with the rest of the uniform buffer setup
  updateUniformBufferInternal(currentImage, entity, entityRes, camera, ubo);
}

// Internal helper function to complete uniform buffer setup
void Renderer::updateUniformBufferInternal(uint32_t currentImage, Entity* entity, EntityResources* entityRes, CameraComponent* camera, UniformBufferObject& ubo) {
  if (!entityRes) {
    return;
  }

  // Use frame template for most fields
  UniformBufferObject finalUbo = frameUboTemplate;
  finalUbo.model = ubo.model;

  // Copy to uniform buffer (guard against null mapped pointer)
  void* dst = entityRes->uniformBuffersMapped[currentImage];
  if (!dst) {
    std::cerr << "Warning: UBO mapped ptr null for entity '" << (entity ? entity->GetName() : "unknown") << "' frame " << currentImage << std::endl;
    return;
  }
  std::memcpy(dst, &finalUbo, sizeof(UniformBufferObject));
}

void Renderer::ensureEntityMaterialCache(Entity* entity, EntityResources& res) {
  if (!entity)
    return;

  if (res.materialCacheValid)
    return;

  res.materialCacheValid = true;
  res.cachedMaterial = nullptr;
  res.cachedIsBlended = false;
  res.cachedIsGlass = false;
  res.cachedIsLiquid = false;

  // Defaults represent the common case (no explicit material); textures come from descriptor bindings.
  MaterialProperties mp{};
  // Sensible defaults for entities without explicit material
  mp.baseColorFactor = glm::vec4(1.0f);
  mp.metallicFactor = 0.0f;
  mp.roughnessFactor = 1.0f;
  mp.baseColorTextureSet = 0;
  mp.physicalDescriptorTextureSet = 0;
  mp.normalTextureSet = -1;
  mp.occlusionTextureSet = -1;
  mp.emissiveTextureSet = -1;
  mp.alphaMask = 0.0f;
  mp.alphaMaskCutoff = 0.5f;
  mp.emissiveFactor = glm::vec3(0.0f);
  mp.emissiveStrength = 1.0f;
  mp.transmissionFactor = 0.0f;
  mp.useSpecGlossWorkflow = 0;
  mp.glossinessFactor = 0.0f;
  mp.specularFactor = glm::vec3(1.0f);
  mp.ior = 1.5f;
  mp.hasEmissiveStrengthExtension = 0;

  if (modelLoader) {
    const std::string& entityName = entity->GetName();
    const size_t tagPos = entityName.find("_Material_");
    if (tagPos != std::string::npos) {
      const size_t afterTag = tagPos + std::string("_Material_").size();
      if (afterTag < entityName.length()) {
        // Entity name format: "modelName_Material_<index>_<materialName>"
        const std::string remainder = entityName.substr(afterTag);
        const size_t nextUnderscore = remainder.find('_');
        if (nextUnderscore != std::string::npos && nextUnderscore + 1 < remainder.length()) {
          const std::string materialName = remainder.substr(nextUnderscore + 1);
          if (const Material* material = modelLoader->GetMaterial(materialName)) {
            res.cachedMaterial = material;
            res.cachedIsGlass = material->isGlass;
            res.cachedIsLiquid = material->isLiquid;

            // Base factors
            mp.baseColorFactor = glm::vec4(material->albedo, material->alpha);
            mp.metallicFactor = material->metallic;
            mp.roughnessFactor = material->roughness;

            // Texture set flags (-1 = no texture)
            mp.baseColorTextureSet = material->albedoTexturePath.empty() ? -1 : 0;
            // physical descriptor: MR or SpecGloss
            if (material->useSpecularGlossiness) {
              mp.useSpecGlossWorkflow = 1;
              mp.physicalDescriptorTextureSet = material->specGlossTexturePath.empty() ? -1 : 0;
              mp.glossinessFactor = material->glossinessFactor;
              mp.specularFactor = material->specularFactor;
            } else {
              mp.useSpecGlossWorkflow = 0;
              mp.physicalDescriptorTextureSet = material->metallicRoughnessTexturePath.empty() ? -1 : 0;
            }
            mp.normalTextureSet = material->normalTexturePath.empty() ? -1 : 0;
            mp.occlusionTextureSet = material->occlusionTexturePath.empty() ? -1 : 0;
            mp.emissiveTextureSet = material->emissiveTexturePath.empty() ? -1 : 0;

            // Emissive and transmission/IOR
            mp.emissiveFactor = material->emissive;
            mp.emissiveStrength = material->emissiveStrength;
            // Heuristic: consider emissive strength extension present when strength != 1.0
            mp.hasEmissiveStrengthExtension = (std::abs(material->emissiveStrength - 1.0f) > 1e-6f) ? 1 : 0;
            mp.transmissionFactor = material->transmissionFactor;
            mp.ior = material->ior;

            // Alpha mask handling
            mp.alphaMask = (material->alphaMode == "MASK") ? 1.0f : 0.0f;
            mp.alphaMaskCutoff = material->alphaCutoff;

            // Blended classification (opaque materials stay in the opaque pass)
            const bool alphaBlend = (material->alphaMode == "BLEND");
            const bool highTransmission = (material->transmissionFactor > 0.2f);
            res.cachedIsBlended = alphaBlend || highTransmission || res.cachedIsGlass || res.cachedIsLiquid;
          }
        }
      }
    }
  }

  res.cachedMaterialProps = mp;
}

// Render the scene (unique_ptr container overload)
// Convert to a raw-pointer snapshot so callers can safely release their container locks.
void Renderer::Render(const std::vector<std::unique_ptr<Entity>>& entities, CameraComponent* camera, ImGuiSystem* imguiSystem) {
  std::vector<Entity *> snapshot;
  snapshot.reserve(entities.size());
  for (const auto& uptr : entities) {
    snapshot.push_back(uptr.get());
  }
  Render(snapshot, camera, imguiSystem);
}

// Render the scene (raw pointer snapshot overload)
void Renderer::Render(const std::vector<Entity *>& entities, CameraComponent* camera, ImGuiSystem* imguiSystem) {
  // Update watchdog timestamp to prove frame is progressing
  lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
  watchdogProgressLabel.store("Render: frame begin", std::memory_order_relaxed);

  if (memoryPool)
    memoryPool->setRenderingActive(true);
  struct RenderingStateGuard {
    MemoryPool* pool;
    explicit RenderingStateGuard(MemoryPool* p) : pool(p) {
    }
    ~RenderingStateGuard() {
      if (pool)
        pool->setRenderingActive(false);
    }
  } guard(memoryPool.get());

  // --- Extract lights for the frame ---
  // Build a single light list once per frame (emissive lights only for this scene)
  std::vector<ExtractedLight> lightsSubset;
  if (!staticLights.empty()) {
    lightsSubset.reserve(std::min(staticLights.size(), static_cast<size_t>(MAX_ACTIVE_LIGHTS)));
    for (const auto& L : staticLights) {
      // Include all lights (Directional, Point, Emissive) up to the limit
      lightsSubset.push_back(L);
      if (lightsSubset.size() >= MAX_ACTIVE_LIGHTS)
        break;
    }
  }
  // Append the lights of all entities with a LightComponent
  for (Entity* entity : entities) {
    if (!entity || !entity->IsActive())
      continue;
    auto lightComponent = entity->GetComponent<LightComponent>();
    if (!lightComponent)
      continue;
    if (lightsSubset.size() >= MAX_ACTIVE_LIGHTS)
      break;
    lightsSubset.push_back(lightComponent->GetLight());
  }
  lastFrameLightCount = static_cast<uint32_t>(lightsSubset.size());
  if (!lightsSubset.empty()) {
    updateLightStorageBuffer(currentFrame, lightsSubset, camera);
  }

  // Pre-calculate frame-constant UBO data
  prepareFrameUboTemplate(camera);

  // Wait for the previous frame's work on this frame slot to complete
  // Use a finite timeout loop so we can keep the watchdog alive during long GPU work
  watchdogProgressLabel.store("Render: wait inFlightFence", std::memory_order_relaxed);
  vk::Result fenceResult = waitForFencesSafe(*inFlightFences[currentFrame], VK_TRUE);
  if (fenceResult != vk::Result::eSuccess) {
    std::cerr << "Error: Failed to wait for in-flight fence: " << vk::to_string(fenceResult) << std::endl;
  }

  // Reset the fence immediately after successful wait, before any new work
  watchdogProgressLabel.store("Render: reset inFlightFence", std::memory_order_relaxed);
  device.resetFences(*inFlightFences[currentFrame]);

  // Execute any pending GPU uploads (enqueued by worker/loading threads) on the render thread
  // at this safe point to ensure all Vulkan submits happen on a single thread.
  // This prevents validation/GPU-AV PostSubmit crashes due to cross-thread queue usage.
  watchdogProgressLabel.store("Render: ProcessPendingMeshUploads", std::memory_order_relaxed);
  ProcessPendingMeshUploads();
  // Execute any pending per-entity GPU resource preallocation requested by the scene loader.
  // This prevents background threads from mutating `entityResources`/`meshResources` concurrently
  // with rendering (which can corrupt unordered_map internals and crash).
  watchdogProgressLabel.store("Render: ProcessPendingEntityPreallocations", std::memory_order_relaxed);
  ProcessPendingEntityPreallocations();
  watchdogProgressLabel.store("Render: after ProcessPendingEntityPreallocations", std::memory_order_relaxed);

  // Safe point: the previous work referencing this frame's descriptor sets is complete.
  // Apply any deferred descriptor set updates for entities whose textures finished streaming.
  watchdogProgressLabel.store("Render: ProcessDirtyDescriptorsForFrame", std::memory_order_relaxed);
  ProcessDirtyDescriptorsForFrame(currentFrame);
  watchdogProgressLabel.store("Render: after ProcessDirtyDescriptorsForFrame", std::memory_order_relaxed);

  // --- 1. PREPARATION PASS ---
  // Gather active entities with mesh resources, perform per-frame descriptor initialization,
  // and execute culling. This single pass replaces multiple redundant scans and reduces map lookups.
  std::vector<RenderJob> opaqueJobs;
  std::vector<RenderJob> transparentJobs;
  opaqueJobs.reserve(entities.size());

  {
    watchdogProgressLabel.store("Render: preparation pass", std::memory_order_relaxed);

    // Prepare frustum once per frame for culling
    FrustumPlanes frustum{};
    const bool doCulling = enableFrustumCulling && camera;
    if (doCulling && camera) {
      glm::mat4 proj = camera->GetProjectionMatrix();
      proj[1][1] *= -1.0f;
      const glm::mat4 vp = proj * camera->GetViewMatrix();
      frustum = extractFrustumPlanes(vp);
    }
    lastCullingVisibleCount = 0;
    lastCullingCulledCount = 0;

    uint32_t entityProcessCount = 0;
    for (Entity* entity : entities) {
      if (!entity || !entity->IsActive())
        continue;
      auto meshComponent = entity->GetComponent<MeshComponent>();
      if (!meshComponent)
        continue;

      auto entityIt = entityResources.find(entity);
      if (entityIt == entityResources.end())
        continue;

      auto meshIt = meshResources.find(meshComponent);
      if (meshIt == meshResources.end())
        continue;

      EntityResources& entityRes = entityIt->second;
      MeshResources& meshRes = meshIt->second;

      // Ensure material cache is valid once per frame
      ensureEntityMaterialCache(entity, entityRes);

      // --- Per-frame Descriptor Cold-Init (Integrated) ---
      if (entityRes.pbrDescriptorSets.empty()) {
        std::string texPath = meshComponent->GetBaseColorTexturePath();
        if (texPath.empty()) texPath = meshComponent->GetTexturePath();
        if (entityRes.pbrDescriptorSets.empty()) createDescriptorSets(entity, entityRes, texPath, true);
      }

      // Initialize binding 0 (UBO) for the current frame slot if not already done.
      if (!entityRes.pbrUboBindingWritten[currentFrame]) {
        std::string texPath = meshComponent->GetBaseColorTexturePath();
        if (texPath.empty()) texPath = meshComponent->GetTexturePath();
        if (!entityRes.pbrUboBindingWritten[currentFrame]) {
          updateDescriptorSetsForFrame(entity, entityRes, texPath, true, currentFrame, false, true);
        }
      }

      // Initialize images for the current frame slot if not already done.
      if (!entityRes.pbrImagesWritten[currentFrame]) {
        std::string texPath = meshComponent->GetBaseColorTexturePath();
        if (texPath.empty()) texPath = meshComponent->GetTexturePath();
        if (!entityRes.pbrImagesWritten[currentFrame]) {
          updateDescriptorSetsForFrame(entity, entityRes, texPath, true, currentFrame, true, false);
          entityRes.pbrImagesWritten[currentFrame] = true;
        }
      }

      // --- Culling & Classification ---
      auto* tc = entity->GetComponent<TransformComponent>();
      bool useBlended = entityRes.cachedIsBlended;

      if (meshComponent->HasLocalAABB()) {
        const glm::mat4 model = tc ? tc->GetModelMatrix() : glm::mat4(1.0f);
        glm::vec3 wmin, wmax;
        transformAABB(model, meshComponent->GetLocalAABBMin(), meshComponent->GetLocalAABBMax(), wmin, wmax);

        // 1. Frustum Culling
        if (doCulling && !aabbIntersectsFrustum(wmin, wmax, frustum)) {
          lastCullingCulledCount++;
          continue;
        }

        // 2. Distance-based LOD
        if (enableDistanceLOD && camera) {
          glm::vec3 camPos = camera->GetPosition();
          bool cameraInside = (camPos.x >= wmin.x && camPos.x <= wmax.x &&
                               camPos.y >= wmin.y && camPos.y <= wmax.y &&
                               camPos.z >= wmin.z && camPos.z <= wmax.z);
          if (!cameraInside) {
            float dx = std::max({0.0f, wmin.x - camPos.x, camPos.x - wmax.x});
            float dy = std::max({0.0f, wmin.y - camPos.y, camPos.y - wmax.y});
            float dz = std::max({0.0f, wmin.z - camPos.z, camPos.z - wmax.z});
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            float z_eff = std::max(0.1f, dist);
            float fov = glm::radians(camera->GetFieldOfView());
            float radius = glm::length(0.5f * (wmax - wmin));
            float pixelDiameter = (radius * 2.0f * static_cast<float>(swapChainExtent.height)) / (z_eff * 2.0f * std::tan(fov * 0.5f));
            float threshold = useBlended ? lodPixelThresholdTransparent : lodPixelThresholdOpaque;
            if (pixelDiameter < threshold) {
              lastCullingCulledCount++;
              continue;
            }
          }
        }
      }

      lastCullingVisibleCount++;
      bool isAlphaMasked = false;
      if (entityRes.materialCacheValid) {
        isAlphaMasked = (entityRes.cachedMaterialProps.alphaMask > 0.5f);
      }

      // Update UBO for visible entity once per frame (shared across all main passes)
      updateUniformBuffer(currentFrame, entity, &entityRes, camera, tc);

      RenderJob job{entity, &entityRes, &meshRes, meshComponent, tc, isAlphaMasked};
      // An entity that was added to pipelines gets one job per pipeline, in the order in which the
      // pipelines were created. A pipeline with blending is drawn in the transparent pass.
      bool addedToPipelines = false;
      {
        std::lock_guard<std::mutex> lk(entityPipelinesMutex);
        auto pipelinesIt = entityPipelines.find(entity);
        if (pipelinesIt != entityPipelines.end()) {
          addedToPipelines = true;
          for (int pipelineIndex : pipelinesIt->second) {
            RenderJob pipelineJob = job;
            bool pipelineBlended = useBlended; // "pbr": the material decides, as for every other entity
            if (pipelineIndex != PBR_PIPELINE) {
              pipelineJob.pipeline = &namedPipelines[pipelineIndex].pipeline;
              pipelineBlended = namedPipelines[pipelineIndex].settings.blending;
            }
            if (pipelineBlended) {
              transparentJobs.push_back(pipelineJob);
            } else {
              opaqueJobs.push_back(pipelineJob);
            }
          }
        }
      }
      if (addedToPipelines) {
        // jobs were queued above
      } else
      if (useBlended) {
        transparentJobs.push_back(job);
      } else {
        opaqueJobs.push_back(job);
      }

      // Update watchdog periodically
      if (++entityProcessCount % 100 == 0) {
        lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
      }
    }
    watchdogProgressLabel.store("Render: after preparation pass", std::memory_order_relaxed);
  }

  // If the scene loader has finished and there are no remaining blocking tasks,
  // hide the fullscreen loading overlay.
  if (IsLoading() && GetLoadingPhase() == LoadingPhase::Finalizing) {
    const bool loaderDone = !loadingFlag.load(std::memory_order_relaxed);
    const bool criticalDone = (criticalJobsOutstanding.load(std::memory_order_relaxed) == 0u);
    const bool noPreallocPending = !pendingEntityPreallocQueued.load(std::memory_order_relaxed);
    const bool noDirtyEntities = descriptorDirtyEntities.empty();
    const bool noDeferredDescOps = !descriptorRefreshPending.load(std::memory_order_relaxed);

    if (loaderDone && criticalDone && noPreallocPending && noDirtyEntities && noDeferredDescOps) {
      LOGI("Renderer: Transitioning from Loading to Active scene");
      MarkInitialLoadComplete();
    }
  }

  // Safe point: flush any descriptor updates that were deferred while a command buffer
  // was recording in a prior frame. Only apply ops for the current frame to avoid
  // update-after-bind on pending frames.
  if (descriptorRefreshPending.load(std::memory_order_relaxed)) {
    watchdogProgressLabel.store("Render: flush deferred descriptor ops", std::memory_order_relaxed);
    std::vector<PendingDescOp> ops; {
      std::lock_guard<std::mutex> lk(pendingDescMutex);
      ops.swap(pendingDescOps);
      descriptorRefreshPending.store(false, std::memory_order_relaxed);
    }
    uint32_t opCount = 0;
    for (auto& op : ops) {
      // Kick watchdog periodically during potentially heavy descriptor update bursts
      if ((++opCount % 50u) == 0u) {
        lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
      }

      if (op.frameIndex == currentFrame) {
        // Now not recording; safe to apply updates for this frame
        updateDescriptorSetsForFrame(op.entity, op.texPath, op.usePBR, op.frameIndex, op.imagesOnly);
      } else {
        // Keep other frame ops queued for next frame’s safe point
        std::lock_guard<std::mutex> lk(pendingDescMutex);
        pendingDescOps.push_back(op);
        descriptorRefreshPending.store(true, std::memory_order_relaxed);
      }
    }
    watchdogProgressLabel.store("Render: after deferred descriptor ops", std::memory_order_relaxed);
  }

  // Acquire next swapchain image
  // acquireNextImage returns imageIndex (which swapchain image is available).
  // Use currentFrame to select an imageAvailableSemaphore for acquire.
  // Use imageIndex to select renderFinishedSemaphore for present (ties semaphore to the specific image).
  const uint32_t acquireSemaphoreIndex = currentFrame % static_cast<uint32_t>(imageAvailableSemaphores.size());

  uint32_t imageIndex;
  vk::Result acquireResultCode = vk::Result::eSuccess;
  // Helper overloads to normalize acquireNextImage return across Vulkan-Hpp versions
  auto extractAcquire = [](auto const& ret, vk::Result& code, uint32_t& idx) {
    using RetT = std::decay_t<decltype(ret)>;
    if constexpr (std::is_same_v<RetT, vk::ResultValue<uint32_t>>) {
      code = ret.result;
      idx = ret.value;
    } else {
      // Assume older std::pair<vk::Result, uint32_t>
      code = ret.first;
      idx = ret.second;
    }
  };
  try {
    watchdogProgressLabel.store("Render: acquireNextImage", std::memory_order_relaxed);
    auto acquireRet = swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[acquireSemaphoreIndex]);
    // Vulkan-Hpp changed the return type of acquireNextImage for RAII swapchain across versions.
    // Support both vk::ResultValue<uint32_t> (newer) and std::pair<vk::Result, uint32_t> (older).
    extractAcquire(acquireRet, acquireResultCode, imageIndex);
  } catch (const vk::OutOfDateKHRError&) {
    watchdogProgressLabel.store("Render: acquireNextImage out-of-date", std::memory_order_relaxed);
    // Swapchain is out of date (e.g., window resized) before we could
    // query the result. Trigger recreation and exit this frame cleanly.
    framebufferResized.store(true, std::memory_order_relaxed);
    if (imguiSystem)
      ImGui::EndFrame();
    // IMPORTANT: We already reset the in-flight fence at the start of the frame.
    // Because we're exiting early (no submit), signal it via an empty submit so
    // swapchain recreation won't hang waiting for an unsignaled fence.
    {
      vk::SubmitInfo2 emptySubmit2{};
      std::lock_guard<std::mutex> lock(queueMutex);
      graphicsQueue.submit2(emptySubmit2, *inFlightFences[currentFrame]);
    }
    recreateSwapChain();
    return;
  }

  // imageIndex already populated above
  watchdogProgressLabel.store("Render: acquired swapchain image", std::memory_order_relaxed);

  bool isLoading = IsLoading();
  bool flag = loadingFlag.load();
  uint32_t critical = criticalJobsOutstanding.load();
  bool initDone = initialLoadComplete.load();

  if (acquireResultCode == vk::Result::eSuboptimalKHR || framebufferResized.load(std::memory_order_relaxed)) {
    framebufferResized.store(false, std::memory_order_relaxed);
    if (imguiSystem)
      ImGui::EndFrame();
    // Fence was reset earlier; ensure it is signaled before we bail out
    // to avoid a deadlock in swapchain recreation.
    {
      vk::SubmitInfo2 emptySubmit2{};
      std::lock_guard<std::mutex> lock(queueMutex);
      graphicsQueue.submit2(emptySubmit2, *inFlightFences[currentFrame]);
    }
    recreateSwapChain();
    return;
  }
  if (acquireResultCode != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to acquire swap chain image");
  }

  if (framebufferResized.load(std::memory_order_relaxed)) {
    // Signal the fence via empty submit since no real work will be submitted
    // this frame, preventing a wait on an unsignaled fence during resize.
    {
      vk::SubmitInfo2 emptySubmit2{};
      std::lock_guard<std::mutex> lock(queueMutex);
      graphicsQueue.submit2(emptySubmit2, *inFlightFences[currentFrame]);
    }
    recreateSwapChain();
    return;
  }

  // Ensure light buffers are sufficiently large before recording to avoid resizing while in use
  {
    // Reserve capacity based on emissive lights only (punctual lights disabled for now)
    size_t desiredLightCapacity = 0;
    if (!staticLights.empty()) {
      size_t emissiveCount = 0;
      for (const auto& L : staticLights) {
        if (L.type == ExtractedLight::Type::Emissive) {
          ++emissiveCount;
          if (emissiveCount >= MAX_ACTIVE_LIGHTS)
            break;
        }
      }
      desiredLightCapacity = emissiveCount;
    }
    if (desiredLightCapacity > 0) {
      createOrResizeLightStorageBuffers(desiredLightCapacity);
    }
  }

  commandBuffers[currentFrame].reset();
  // Begin command buffer recording for this frame
  commandBuffers[currentFrame].begin(vk::CommandBufferBeginInfo());
  isRecordingCmd.store(true, std::memory_order_relaxed);
  if (framebufferResized.load(std::memory_order_relaxed)) {
    commandBuffers[currentFrame].end();
    recreateSwapChain();
    return;
  }

  // Process texture streaming uploads (see Renderer::ProcessPendingTextureJobs)

  vk::raii::Pipeline* currentPipeline = nullptr;
  vk::raii::PipelineLayout* currentLayout = nullptr;

  // Incrementally process pending texture uploads on the main thread so that
  // all Vulkan submits happen from a single place while worker threads only
  // handle CPU-side decoding. While the loading screen is up, prioritize
  // critical textures so the first rendered frame looks mostly correct.
  if (IsLoading()) {
    // Larger budget while loading screen is visible so we don't stall
    // streaming of near-field baseColor textures.
    ProcessPendingTextureJobs(/*maxJobs=*/16, /*includeCritical=*/true, /*includeNonCritical=*/false);
  } else {
    // After loading screen disappears, we want the scene to remain
    // responsive (~20 fps) while textures stream in. Limit the number
    // of non-critical uploads per frame so we don't tank frame time.
    static uint32_t streamingFrameCounter = 0;
    streamingFrameCounter++;
    {
      // Raster path: keep previous throttling to avoid stalls.
      if ((streamingFrameCounter % 3) == 0) {
        ProcessPendingTextureJobs(/*maxJobs=*/1, /*includeCritical=*/false, /*includeNonCritical=*/true);
      }
    }
  }

  // Renderer UI
  // Hide UI during loading; the progress overlay is handled by ImGuiSystem::NewFrame().
  if (imguiSystem && !imguiSystem->IsFrameRendered() && !IsLoading()) {
    if (ImGui::Begin("Renderer")) {
      // === SHARED OPTIONS ===
      ImGui::Text("Culling & LOD:");
      if (ImGui::Checkbox("Frustum culling", &enableFrustumCulling)) {
        // no-op, takes effect immediately
      }
      if (ImGui::Checkbox("Distance LOD (projected-size skip)", &enableDistanceLOD)) {
      }
      ImGui::SliderFloat("LOD threshold opaque (px)", &lodPixelThresholdOpaque, 0.5f, 8.0f, "%.1f");
      ImGui::SliderFloat("LOD threshold transparent (px)", &lodPixelThresholdTransparent, 0.5f, 12.0f, "%.1f");
      // Anisotropy control (recreate samplers on change)
      {
        float deviceMaxAniso = physicalDevice.getProperties().limits.maxSamplerAnisotropy;
        if (ImGui::SliderFloat("Sampler max anisotropy", &samplerMaxAnisotropy, 1.0f, deviceMaxAniso, "%.1f")) {
          // Recreate samplers for all textures to apply new anisotropy
          std::unique_lock<std::shared_mutex> texLock(textureResourcesMutex);
          for (auto& kv : textureResources) {
            createTextureSampler(kv.second);
          }
          // Default texture
          createTextureSampler(defaultTextureResources);
        }
      }
      if (lastCullingVisibleCount + lastCullingCulledCount > 0) {
        ImGui::Text("Culling: visible=%u, culled=%u", lastCullingVisibleCount, lastCullingCulledCount);
      }

      // Basic tone mapping controls
      ImGui::Separator();
      ImGui::Text("Tone Mapping & Tuning:");
      ImGui::SliderFloat("Exposure", &exposure, 0.1f, 4.0f, "%.2f");
    }
    ImGui::End();

    // Invoke any registered Course module / plugin ImGui panel
    {
      std::lock_guard<std::mutex> lock(imguiPanelCallbackMutex);
      if (imguiPanelCallback) {
        imguiPanelCallback(this);
      }
    }
  }

  // Rasterization rendering
  {
    // Sort transparent entities back-to-front for correct blending of nested glass/liquids
    if (!transparentJobs.empty()) {
      glm::vec3 camPos = camera ? camera->GetPosition() : glm::vec3(0.0f);
      std::ranges::stable_sort(transparentJobs,
                        [camPos](const RenderJob& a, const RenderJob& b) {
                          glm::vec3 pa = a.transformComp ? a.transformComp->GetPosition() : glm::vec3(0.0f);
                          glm::vec3 pb = b.transformComp ? b.transformComp->GetPosition() : glm::vec3(0.0f);
                          float da2 = glm::length2(pa - camPos);
                          float db2 = glm::length2(pb - camPos);
                          if (da2 != db2) return da2 > db2;
                          if (a.entityRes->cachedIsLiquid != b.entityRes->cachedIsLiquid) return a.entityRes->cachedIsLiquid;
                          return a.entity < b.entity;
                        });
    }

    // PASS 1: RENDER OPAQUE OBJECTS TO OFF-SCREEN TEXTURE
    // Transition off-screen color to attachment write (Sync2). On first use after creation or after switching
    // from a mode that never produced this image, the layout may still be UNDEFINED.
    vk::ImageLayout oscOldLayout = vk::ImageLayout::eUndefined;
    vk::PipelineStageFlags2 oscSrcStage = vk::PipelineStageFlagBits2::eTopOfPipe;
    vk::AccessFlags2 oscSrcAccess = vk::AccessFlagBits2::eNone;
    if (currentFrame < opaqueSceneColorImageLayouts.size()) {
      oscOldLayout = opaqueSceneColorImageLayouts[currentFrame];
      if (oscOldLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        oscSrcStage = vk::PipelineStageFlagBits2::eFragmentShader;
        oscSrcAccess = vk::AccessFlagBits2::eShaderRead;
      } else if (oscOldLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        oscSrcStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        oscSrcAccess = vk::AccessFlagBits2::eColorAttachmentWrite;
      } else {
        oscOldLayout = vk::ImageLayout::eUndefined;
        oscSrcStage = vk::PipelineStageFlagBits2::eTopOfPipe;
        oscSrcAccess = vk::AccessFlagBits2::eNone;
      }
    }
    vk::ImageMemoryBarrier2 oscToColor2{
      .srcStageMask = oscSrcStage,
      .srcAccessMask = oscSrcAccess,
      .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
      .oldLayout = oscOldLayout,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = *opaqueSceneColorImages[currentFrame],
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };
    vk::DependencyInfo depOscToColor{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &oscToColor2};
    commandBuffers[currentFrame].pipelineBarrier2(depOscToColor);
    if (currentFrame < opaqueSceneColorImageLayouts.size()) {
      opaqueSceneColorImageLayouts[currentFrame] = vk::ImageLayout::eColorAttachmentOptimal;
    }
    // PASS 1: OFF-SCREEN COLOR (Opaque)
    // Clear the off-screen target at the start of opaque rendering to a neutral black background
    vk::RenderingAttachmentInfo colorAttachment{.imageView = *opaqueSceneColorImageViews[currentFrame], .imageLayout = vk::ImageLayout::eColorAttachmentOptimal, .loadOp = vk::AttachmentLoadOp::eClear, .storeOp = vk::AttachmentStoreOp::eStore, .clearValue = vk::ClearColorValue(std::array < float, 4 >{0.0f, 0.0f, 0.0f, 1.0f})};
    depthAttachment.imageView = *depthImageView;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    vk::RenderingInfo passInfo{.renderArea = vk::Rect2D({0, 0}, swapChainExtent), .layerCount = 1, .colorAttachmentCount = 1, .pColorAttachments = &colorAttachment, .pDepthAttachment = &depthAttachment};
    commandBuffers[currentFrame].beginRendering(passInfo);
    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
    commandBuffers[currentFrame].setViewport(0, viewport);
    vk::Rect2D scissor({0, 0}, swapChainExtent);
    commandBuffers[currentFrame].setScissor(0, scissor); {
      uint32_t opaqueDrawsThisPass = 0;
      for (const auto& job : opaqueJobs) {
        vk::raii::Pipeline* selectedPipeline = nullptr;
        vk::raii::PipelineLayout* selectedLayout = nullptr;
        {
          selectedPipeline = &pbrGraphicsPipeline; // writes depth, compare Less
          selectedLayout = &pbrPipelineLayout;
        }
        if (job.pipeline) {
          selectedPipeline = job.pipeline; // a named pipeline the entity was added to; same layout
        }
        if (currentPipeline != selectedPipeline) {
          commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, **selectedPipeline);
          currentPipeline = selectedPipeline;
          currentLayout = selectedLayout;
        }

        std::array<vk::Buffer, 2> buffers = {*job.meshRes->vertexBuffer, *job.entityRes->instanceBuffer};
        std::array<vk::DeviceSize, 2> offsets = {0, 0};
        commandBuffers[currentFrame].bindVertexBuffers(0, buffers, offsets);
        commandBuffers[currentFrame].bindIndexBuffer(*job.meshRes->indexBuffer, 0, vk::IndexType::eUint32);

        auto* descSetsPtr = &job.entityRes->pbrDescriptorSets;
        if (descSetsPtr->empty() || currentFrame >= descSetsPtr->size()) {
          continue;
        }

        {
          vk::DescriptorSet set1Opaque = (transparentDescriptorSets.empty() || IsLoading())
                                           ? *transparentFallbackDescriptorSets[currentFrame]
                                           : *transparentDescriptorSets[currentFrame];
          commandBuffers[currentFrame].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            **selectedLayout,
            0,
            {*(*descSetsPtr)[currentFrame], set1Opaque},
            {});

          commandBuffers[currentFrame].pushConstants<MaterialProperties>(**selectedLayout, vk::ShaderStageFlagBits::eFragment, 0, {job.entityRes->cachedMaterialProps});
        }
        uint32_t instanceCount = std::max(1u, static_cast<uint32_t>(job.meshComp->GetInstanceCount()));
        commandBuffers[currentFrame].drawIndexed(job.meshRes->indexCount, instanceCount, 0, 0, 0);
        ++opaqueDrawsThisPass;
      }
    }
    commandBuffers[currentFrame].endRendering();
    // PASS 1b: PRESENT – composite path
    {
      // Transition off-screen to SHADER_READ for sampling (Sync2)
      vk::ImageMemoryBarrier2 opaqueToSample2{
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = *opaqueSceneColorImages[currentFrame],
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
      };
      vk::DependencyInfo depOpaqueToSample{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &opaqueToSample2};
      commandBuffers[currentFrame].pipelineBarrier2(depOpaqueToSample);
      if (currentFrame < opaqueSceneColorImageLayouts.size()) {
        opaqueSceneColorImageLayouts[currentFrame] = vk::ImageLayout::eShaderReadOnlyOptimal;
      }

      // Make the swapchain image ready for color attachment output and clear it (Sync2)
      vk::ImageMemoryBarrier2 swapchainToColor2{
        .srcStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .srcAccessMask = vk::AccessFlagBits2::eNone,
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages[imageIndex],
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
      };
      vk::DependencyInfo depSwapchainToColor{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &swapchainToColor2};
      commandBuffers[currentFrame].pipelineBarrier2(depSwapchainToColor);

      // Begin rendering to swapchain for composite
      colorAttachments[0].imageView = *swapChainImageViews[imageIndex];
      colorAttachments[0].loadOp = vk::AttachmentLoadOp::eClear; // clear before composing base layer (full-screen composite overwrites all pixels)
      colorAttachments[0].clearValue = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}); // Neutral black
      depthAttachment.loadOp = vk::AttachmentLoadOp::eDontCare; // no depth for composite
      renderingInfo.renderArea = vk::Rect2D({0, 0}, swapChainExtent);
      // IMPORTANT: Composite pass does not use a depth attachment. Avoid binding it to satisfy dynamic rendering VUIDs.
      auto savedDepthPtr = renderingInfo.pDepthAttachment; // save to restore later
      renderingInfo.pDepthAttachment = nullptr;
      commandBuffers[currentFrame].beginRendering(renderingInfo);

      // Bind composite pipeline
      if (!!*compositePipeline) {
        commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *compositePipeline);
      }
      vk::Viewport vp(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
      commandBuffers[currentFrame].setViewport(0, vp);
      vk::Rect2D sc({0, 0}, swapChainExtent);
      commandBuffers[currentFrame].setScissor(0, sc);

      // Bind descriptor set 0 for the composite. During loading, force fallback to avoid sampling uninitialized off-screen color.
      vk::DescriptorSet setComposite = (transparentDescriptorSets.empty() || IsLoading())
                                         ? *transparentFallbackDescriptorSets[currentFrame]
                                         : *transparentDescriptorSets[currentFrame];
      commandBuffers[currentFrame].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *compositePipelineLayout,
        0,
        {setComposite},
        {});

      // Push exposure/gamma and sRGB flag
      struct CompositePush {
        float exposure;
        float gamma;
        int outputIsSRGB;
        float _pad;
      } pc{};
      pc.exposure = std::clamp(this->exposure, 0.2f, 4.0f);
      pc.gamma = this->gamma;
      pc.outputIsSRGB = (swapChainImageFormat == vk::Format::eR8G8B8A8Srgb || swapChainImageFormat == vk::Format::eB8G8R8A8Srgb) ? 1 : 0;

      commandBuffers[currentFrame].pushConstants<CompositePush>(*compositePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);

      // Draw fullscreen triangle
      commandBuffers[currentFrame].draw(3, 1, 0, 0);

      commandBuffers[currentFrame].endRendering();
      // Restore depth attachment pointer for subsequent passes
      renderingInfo.pDepthAttachment = savedDepthPtr;
    }
    // PASS 2: RENDER TRANSPARENT OBJECTS TO THE SWAPCHAIN
    {
      // Ensure depth attachment is bound again for the transparent pass
      renderingInfo.pDepthAttachment = &depthAttachment;
      colorAttachments[0].imageView = *swapChainImageViews[imageIndex];
      colorAttachments[0].loadOp = vk::AttachmentLoadOp::eLoad;
      depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
      renderingInfo.renderArea = vk::Rect2D({0, 0}, swapChainExtent);
      commandBuffers[currentFrame].beginRendering(renderingInfo);
      commandBuffers[currentFrame].setViewport(0, viewport);
      commandBuffers[currentFrame].setScissor(0, scissor);

      if (!transparentJobs.empty()) {
        currentLayout = &pbrTransparentPipelineLayout;
        vk::raii::Pipeline* activeTransparentPipeline = nullptr;

        for (const auto& job : transparentJobs) {
          vk::raii::Pipeline* desiredPipeline = job.entityRes->cachedIsGlass ? &glassGraphicsPipeline : &pbrBlendGraphicsPipeline;
          if (job.pipeline) {
            desiredPipeline = job.pipeline; // a named pipeline with blending the entity was added to
          }
          if (desiredPipeline != activeTransparentPipeline) {
            commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, **desiredPipeline);
            activeTransparentPipeline = desiredPipeline;
          }

          std::array<vk::Buffer, 2> buffers = {*job.meshRes->vertexBuffer, *job.entityRes->instanceBuffer};
          std::array<vk::DeviceSize, 2> offsets = {0, 0};
          commandBuffers[currentFrame].bindVertexBuffers(0, buffers, offsets);
          commandBuffers[currentFrame].bindIndexBuffer(*job.meshRes->indexBuffer, 0, vk::IndexType::eUint32);

          vk::DescriptorSet set1 = (transparentDescriptorSets.empty() || IsLoading())
                                     ? *transparentFallbackDescriptorSets[currentFrame]
                                     : *transparentDescriptorSets[currentFrame];
          commandBuffers[currentFrame].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            **currentLayout,
            0,
            {*job.entityRes->pbrDescriptorSets[currentFrame], set1},
            {});

          MaterialProperties pushConstants = job.entityRes->cachedMaterialProps;
          if (job.entityRes->cachedIsLiquid) {
            pushConstants.transmissionFactor = 0.0f;
          }
          commandBuffers[currentFrame].pushConstants < MaterialProperties > (**currentLayout, vk::ShaderStageFlagBits::eFragment, 0,  {
            pushConstants
          }
          )
          ;
          uint32_t instanceCountT = std::max(1u, static_cast<uint32_t>(job.meshComp->GetInstanceCount()));
          commandBuffers[currentFrame].drawIndexed(job.meshRes->indexCount, instanceCountT, 0, 0, 0);
        }
      }
      // End transparent rendering pass before any layout transitions (even if no transparent draws)
      commandBuffers[currentFrame].endRendering();
    } {
      // Screenshot and final present transition are handled in rasterization path only

      // Final layout transition for present (rasterization path only)
      {
        vk::ImageMemoryBarrier2 presentBarrier2{
          .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
          .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
          .dstStageMask = vk::PipelineStageFlagBits2::eNone,
          .dstAccessMask = {},
          .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
          .newLayout = vk::ImageLayout::ePresentSrcKHR,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = swapChainImages[imageIndex],
          .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        };
        vk::DependencyInfo depToPresentFinal{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &presentBarrier2};
        commandBuffers[currentFrame].pipelineBarrier2(depToPresentFinal);
        if (imageIndex < swapChainImageLayouts.size())
          swapChainImageLayouts[imageIndex] = presentBarrier2.newLayout;
      }
    }
  }

  // Render ImGui UI overlay AFTER rasterization (must always execute regardless of render mode)
  // ImGui expects Render() to be called every frame after NewFrame() - skipping it causes hangs
  if (imguiSystem && !imguiSystem->IsFrameRendered()) {
    // When rasterization renders, swapchain is in PRESENT layout with valid content.
    // Transition to COLOR_ATTACHMENT with loadOp=eLoad to preserve existing pixels for ImGui overlay.
    vk::ImageMemoryBarrier2 presentToColor{
      .srcStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
      .srcAccessMask = vk::AccessFlagBits2::eNone,
      .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
      .oldLayout = (imageIndex < swapChainImageLayouts.size()) ? swapChainImageLayouts[imageIndex] : vk::ImageLayout::eUndefined,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapChainImages[imageIndex],
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };
    vk::DependencyInfo depInfo{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &presentToColor};
    commandBuffers[currentFrame].pipelineBarrier2(depInfo);
    if (imageIndex < swapChainImageLayouts.size())
      swapChainImageLayouts[imageIndex] = presentToColor.newLayout;

    // Begin a dedicated render pass for ImGui (UI overlay)
    vk::RenderingAttachmentInfo imguiColorAttachment{
      .imageView = *swapChainImageViews[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad, // Load existing content
      .storeOp = vk::AttachmentStoreOp::eStore
    };
    vk::RenderingInfo imguiRenderingInfo{
      .renderArea = vk::Rect2D({0, 0}, swapChainExtent),
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &imguiColorAttachment,
      .pDepthAttachment = nullptr
    };
    commandBuffers[currentFrame].beginRendering(imguiRenderingInfo);

    imguiSystem->Render(commandBuffers[currentFrame], currentFrame);

    commandBuffers[currentFrame].endRendering();

    // Transition swapchain back to PRESENT layout after ImGui renders
    vk::ImageMemoryBarrier2 colorToPresent{
      .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
      .dstAccessMask = vk::AccessFlagBits2::eNone,
      .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .newLayout = vk::ImageLayout::ePresentSrcKHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapChainImages[imageIndex],
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };
    vk::DependencyInfo depInfoBack{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &colorToPresent};
    commandBuffers[currentFrame].pipelineBarrier2(depInfoBack);
    if (imageIndex < swapChainImageLayouts.size())
      swapChainImageLayouts[imageIndex] = colorToPresent.newLayout;
  }

  commandBuffers[currentFrame].end();
  isRecordingCmd.store(false, std::memory_order_relaxed);

  // Submit and present (Synchronization 2)
  uint64_t uploadsValueToWait = uploadTimelineLastSubmitted.load(std::memory_order_relaxed);

  // Use acquireSemaphoreIndex for imageAvailable semaphore (same as we used in acquireNextImage)
  // Use imageIndex for renderFinished semaphore (matches the image being presented)

  std::array<vk::SemaphoreSubmitInfo, 2> waitInfos = {
    vk::SemaphoreSubmitInfo{
      .semaphore = *imageAvailableSemaphores[acquireSemaphoreIndex],
      .value = 0,
      .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .deviceIndex = 0
    },
    vk::SemaphoreSubmitInfo{
      .semaphore = *uploadsTimeline,
      .value = uploadsValueToWait,
      .stageMask = vk::PipelineStageFlagBits2::eFragmentShader,
      .deviceIndex = 0
    }
  };

  vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = *commandBuffers[currentFrame], .deviceMask = 0};
  vk::SemaphoreSubmitInfo signalInfo{.semaphore = *renderFinishedSemaphores[imageIndex], .value = 0, .stageMask = vk::PipelineStageFlagBits2::eAllGraphics, .deviceIndex = 0};
  vk::SubmitInfo2 submit2{
    .waitSemaphoreInfoCount = static_cast<uint32_t>(waitInfos.size()),
    .pWaitSemaphoreInfos = waitInfos.data(),
    .commandBufferInfoCount = 1,
    .pCommandBufferInfos = &cmdInfo,
    .signalSemaphoreInfoCount = 1,
    .pSignalSemaphoreInfos = &signalInfo
  };

  if (framebufferResized.load(std::memory_order_relaxed)) {
    vk::SubmitInfo2 emptySubmit2{}; {
      std::lock_guard<std::mutex> lock(queueMutex);
      graphicsQueue.submit2(emptySubmit2, *inFlightFences[currentFrame]);
    }
    recreateSwapChain();
    return;
  }

  // Update watchdog BEFORE queue submit because submit can block waiting for GPU
  // This proves frame CPU work is complete even if GPU queue is busy
  lastFrameUpdateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed); {
    std::lock_guard<std::mutex> lock(queueMutex);
    graphicsQueue.submit2(submit2, *inFlightFences[currentFrame]);
  }

  vk::PresentInfoKHR presentInfo{.waitSemaphoreCount = 1, .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex], .swapchainCount = 1, .pSwapchains = &*swapChain, .pImageIndices = &imageIndex};
  vk::Result presentResult = vk::Result::eSuccess;
  try {
    std::lock_guard<std::mutex> lock(queueMutex);
    presentResult = presentQueue.presentKHR(presentInfo);
  } catch (const vk::OutOfDateKHRError&) {
    framebufferResized.store(true, std::memory_order_relaxed);
  }
  if (presentResult == vk::Result::eSuboptimalKHR || framebufferResized.load(std::memory_order_relaxed)) {
    framebufferResized.store(false, std::memory_order_relaxed);
    recreateSwapChain();
  } else if (presentResult != vk::Result::eSuccess) {
    throw std::runtime_error("Failed to present swap chain image");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
