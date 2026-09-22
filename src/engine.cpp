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
#include "engine.h"
#include "mesh_component.h"
#include "scene_loading.h"
#include <cmath>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <ranges>
#include <stdexcept>

// This implementation corresponds to the Engine_Architecture chapter in the tutorial:
// @see en/Building_a_Simple_Engine/Engine_Architecture/02_architectural_patterns.adoc

Engine::Engine() : resourceManager(std::make_unique<ResourceManager>()) {
}

bool Engine::IsMainThread() const {
  return std::this_thread::get_id() == mainThreadId;
}

void Engine::ProcessPendingEntityRemovals() {
  std::vector<std::string> names; {
    std::lock_guard<std::mutex> lk(pendingEntityRemovalsMutex);
    if (pendingEntityRemovalNames.empty())
      return;
    names.swap(pendingEntityRemovalNames);
  }

  // Process on the main thread only (safety)
  if (!IsMainThread()) {
    // Put them back; we'll retry next main-thread tick
    std::lock_guard<std::mutex> lk(pendingEntityRemovalsMutex);
    pendingEntityRemovalNames.insert(pendingEntityRemovalNames.end(), names.begin(), names.end());
    return;
  }

  // Apply removals using the normal API (which takes the appropriate locks).
  for (const auto& name : names) {
    (void) RemoveEntity(name);
  }
}

Engine::~Engine() {
  Cleanup();
}

// The initialization chain: the body of the tutorial's Initialize() in eight calls (see engine.h).
// Calls 2 to 8 hand the work to the renderer; call 2 creates it, call 8 completes the engine.

// Initialization chain, call 1: the platform with its window and the input callbacks
bool Engine::InitializeWindow(const std::string& appName, int width, int height) {
  // Create platform
  // Record main thread identity for deferring destructive operations from background threads
  mainThreadId = std::this_thread::get_id();

  platform = CreatePlatform();
  if (!platform->Initialize(appName, width, height)) {
    return false;
  }

  // Set resize callback
  platform->SetResizeCallback([this](int width, int height) {
    HandleResize(width, height);
  });

  // Set mouse callback
  platform->SetMouseCallback([this](float x, float y, uint32_t buttons) {
    handleMouseInput(x, y, buttons);
  });

  // Set keyboard callback
  platform->SetKeyboardCallback([this](uint32_t key, bool pressed) {
    handleKeyInput(key, pressed);
  });

  // Set char callback
  platform->SetCharCallback([this](uint32_t c) {
    if (imguiSystem) {
      imguiSystem->HandleChar(c);
    }
  });

  return true;
}

// Initialization chain, call 2: the renderer with the Vulkan instance, the debug messenger and the surface
bool Engine::CreateInstance(const std::string& appName, bool enableValidationLayers) {
  // Create renderer
  renderer = std::make_unique<Renderer>(platform.get());
  if (!renderer->CreateInstance(appName, enableValidationLayers)) {
    return false;
  }

  return true;
}

// Initialization chain, calls 3 to 7: the renderer's
bool Engine::PickDevice(bool enableValidationLayers) {
  return renderer->PickDevice(enableValidationLayers);
}

bool Engine::CreateSwapChain() {
  return renderer->CreateSwapChain();
}

bool Engine::InitializeRendering() {
  return renderer->InitializeRendering();
}

bool Engine::CreatePipelines() {
  return renderer->CreatePipelines();
}

bool Engine::CreateCommandBuffers() {
  return renderer->CreateCommandBuffers();
}

// Initialization chain, call 8: the renderer's sync objects and threads, then the model loader and ImGui
bool Engine::CreateSyncObjects() {
  if (!renderer->CreateSyncObjects()) {
    return false;
  }

  try {
    // Model loader via constructor; also wire into renderer
    modelLoader = std::make_unique<ModelLoader>(renderer.get());
    renderer->SetModelLoader(modelLoader.get());

    // ImGui via constructor
    imguiSystem = std::make_unique<ImGuiSystem>(renderer.get(), platform->GetWindowWidth(), platform->GetWindowHeight());
  } catch (const std::exception& e) {
    std::cerr << "Subsystem initialization failed: " << e.what() << std::endl;
    return false;
  }

  initialized = true;
  return true;
}

// The render loop (planned change 8b): the body of the tutorial's Run() as the loop condition plus the six
// frame calls of the sandbox file, which the sandbox file calls in this order once per frame.

// The loop condition: the window events, the delta time and the FPS title of one frame
bool Engine::IsRunning() {
  if (!initialized) {
    throw std::runtime_error("Engine not initialized");
  }

  running = true;

  // Process platform events
  if (!platform->ProcessEvents()) {
    running = false;
    return false;
  }

  // Calculate delta time
  deltaTimeMs = CalculateDeltaTimeMs();

  // Update frame counter and FPS
  frameCount++;
  fpsUpdateTimer += deltaTimeMs.count() * 0.001f;

  // Update window title with FPS and frame time every second
  if (fpsUpdateTimer >= 1.0f) {
    uint64_t framesSinceLastUpdate = frameCount - lastFPSUpdateFrame;
    double avgMs = 0.0;
    if (framesSinceLastUpdate > 0 && fpsUpdateTimer > 0.0f) {
      currentFPS = static_cast<float>(static_cast<double>(framesSinceLastUpdate) / static_cast<double>(fpsUpdateTimer));
      avgMs = (fpsUpdateTimer / static_cast<double>(framesSinceLastUpdate)) * 1000.0;
    } else {
      // Avoid divide-by-zero; keep previous FPS and estimate avgMs from last delta
      currentFPS = std::max(currentFPS, 1.0f);
      avgMs = static_cast<double>(deltaTimeMs.count());
    }

    // Update window title with frame count, FPS, and frame time
    std::string title = "Simple Engine - Frame: " + std::to_string(frameCount) +
        " | FPS: " + std::to_string(static_cast<int>(currentFPS)) +
        " | ms: " + std::to_string(static_cast<int>(avgMs));
    platform->SetWindowTitle(title);

    // Reset timer and frame counter for next update
    fpsUpdateTimer = 0.0f;
    lastFPSUpdateFrame = frameCount;
  }

  return running;
}

// Frame call 1
void Engine::BeginFrame() {
  renderer->BeginFrame();
}

// Frame call 2: the tutorial's Update() (camera controls, ImGui frame, entity updates), then the renderer's
// part with a snapshot of the entities, as the tutorial's Render() took it
void Engine::UpdateScene() {
  // Update
  Update(deltaTimeMs);

  // Ensure renderer is ready
  if (!renderer || !renderer->IsInitialized()) {
    return;
  }

  // Check if we have an active camera
  if (!activeCamera) {
    return;
  }

  // Apply any entity removals requested by background threads before taking a snapshot.
  ProcessPendingEntityRemovals();

  // Snapshot entity pointers under a short shared lock, then release the lock
  // before rendering. This prevents starving the background loader threads
  // that need the unique lock to create entities/components.
  std::vector<Entity *> snapshot; {
    std::shared_lock<std::shared_mutex> lk(entitiesMutex);
    snapshot.reserve(entities.size());
    for (auto& uptr : entities) {
      snapshot.push_back(uptr.get());
    }
  }

  renderer->UpdateScene(snapshot, activeCamera);
}

// Frame calls 3 to 6: the renderer's (ImGui will be rendered within the render pass)
void Engine::BeginRendering() {
  renderer->BeginRendering(imguiSystem.get());
}

void Engine::DrawScene() {
  renderer->DrawScene();
}

void Engine::EndRendering() {
  renderer->EndRendering(imguiSystem.get());
}

void Engine::EndFrame() {
  renderer->EndFrame(imguiSystem.get());
}

void Engine::Cleanup() {
  if (initialized) {
    // Wait for the device to be idle before cleaning up
    if (renderer) {
      renderer->WaitIdle();
    }

    // Clear entities
    {
      std::unique_lock<std::shared_mutex> lk(entitiesMutex);
      entities.clear();
      entityMap.clear();
    }

    // Clean up subsystems in reverse order of creation
    imguiSystem.reset();
    modelLoader.reset();
    renderer.reset();
    platform.reset();

    initialized = false;
  }
}

Entity* Engine::CreateEntity(const std::string& name) {
  std::unique_lock<std::shared_mutex> lk(entitiesMutex);
  // Always allow duplicate names; map stores a representative entity
  // Create the entity
  auto entity = std::make_unique<Entity>(name);
  // Add to the vector and map
  entities.push_back(std::move(entity));
  Entity* rawPtr = entities.back().get();
  // Update the map to point to the most recently created entity with this name
  entityMap[name] = rawPtr;

  return rawPtr;
}

Entity* Engine::GetEntity(const std::string& name) {
  std::shared_lock<std::shared_mutex> lk(entitiesMutex);
  auto it = entityMap.find(name);
  if (it != entityMap.end()) {
    return it->second;
  }
  return nullptr;
}

bool Engine::RemoveEntity(Entity* entity) {
  if (!entity) {
    return false;
  }

  // If called from a background thread, defer removal to avoid deleting entities
  // while the render thread may be iterating a snapshot.
  if (!IsMainThread()) {
    std::lock_guard<std::mutex> lk(pendingEntityRemovalsMutex);
    pendingEntityRemovalNames.push_back(entity->GetName());
    return true;
  }

  std::unique_lock<std::shared_mutex> lk(entitiesMutex);

  // Remember the name before erasing ownership
  std::string name = entity->GetName();

  // Find the entity in the vector
  auto it = std::ranges::find_if(entities,
                                 [entity](const std::unique_ptr<Entity>& e) {
                                   return e.get() == entity;
                                 });

  if (it != entities.end()) {
    // Remove from the vector (ownership)
    entities.erase(it);

    // Update the map: point to another entity with the same name if one exists
    auto remainingIt = std::ranges::find_if(entities,
                                            [&name](const std::unique_ptr<Entity>& e) {
                                              return e->GetName() == name;
                                            });

    if (remainingIt != entities.end()) {
      entityMap[name] = remainingIt->get();
    } else {
      entityMap.erase(name);
    }

    return true;
  }

  return false;
}

bool Engine::RemoveEntity(const std::string& name) {
  // If called from a background thread, defer removal to avoid deleting entities
  // while the render thread may be iterating a snapshot.
  if (!IsMainThread()) {
    std::lock_guard<std::mutex> lk(pendingEntityRemovalsMutex);
    pendingEntityRemovalNames.push_back(name);
    return true;
  }

  std::unique_lock<std::shared_mutex> lk(entitiesMutex);
  auto it = entityMap.find(name);
  if (it == entityMap.end())
    return false;
  Entity* entity = it->second;
  if (!entity)
    return false;

  // Find the entity in the vector
  auto vecIt = std::ranges::find_if(entities,
                                    [entity](const std::unique_ptr<Entity>& e) {
                                      return e.get() == entity;
                                    });
  if (vecIt == entities.end()) {
    entityMap.erase(name);
    return false;
  }

  entities.erase(vecIt);

  // Update the map: point to another entity with the same name if one exists
  auto remainingIt = std::ranges::find_if(entities,
                                          [&name](const std::unique_ptr<Entity>& e) {
                                            return e && e->GetName() == name;
                                          });
  if (remainingIt != entities.end()) {
    entityMap[name] = remainingIt->get();
  } else {
    entityMap.erase(name);
  }
  return true;
}

void Engine::SetActiveCamera(CameraComponent* cameraComponent) {
  activeCamera = cameraComponent;
}

const CameraComponent* Engine::GetActiveCamera() const {
  return activeCamera;
}

const ResourceManager* Engine::GetResourceManager() const {
  return resourceManager.get();
}

const Platform* Engine::GetPlatform() const {
  return platform.get();
}

Renderer* Engine::GetRenderer() {
  return renderer.get();
}

ModelLoader* Engine::GetModelLoader() {
  return modelLoader.get();
}

const ImGuiSystem* Engine::GetImGuiSystem() const {
  return imguiSystem.get();
}

void Engine::handleMouseInput(float x, float y, uint32_t buttons) {
  // Update ImGui system with current mouse state immediately.
  // This pushes events to the ImGui IO queue for processing in NewFrame().
  if (imguiSystem) {
    imguiSystem->HandleMouse(x, y, buttons);
  }

  // Handle LEFT button (Touch DOWN/MOVE/UP)
  if (buttons & 1) {
    if (!cameraControl.mouseLeftPressed) {
      // Finger just went down
      cameraControl.mouseLeftPressed = true;
      cameraControl.firstMouse = true;
    }

    if (cameraControl.firstMouse) {
      cameraControl.lastMouseX = x;
      cameraControl.lastMouseY = y;
      cameraControl.firstMouse = false;
    }

    // Accumulate movement deltas. These will be applied in UpdateCameraControls
    // AFTER ImGui has updated its capture state (post-NewFrame).
    float dx = (x - cameraControl.lastMouseX);
    float dy = (y - cameraControl.lastMouseY);
    cameraControl.pendingXOffset += dx;
    cameraControl.pendingYOffset += dy;

    cameraControl.lastMouseX = x;
    cameraControl.lastMouseY = y;
  } else {
    // Finger lifted
    cameraControl.mouseLeftPressed = false;
  }

  // Update hover detection
  HandleMouseHover(x, y);
}
void Engine::handleKeyInput(uint32_t key, bool pressed) {
  switch (key) {
    case GLFW_KEY_W:
    case GLFW_KEY_UP:
      cameraControl.moveForward = pressed;
      break;
    case GLFW_KEY_S:
    case GLFW_KEY_DOWN:
      cameraControl.moveBackward = pressed;
      break;
    case GLFW_KEY_A:
    case GLFW_KEY_LEFT:
      cameraControl.moveLeft = pressed;
      break;
    case GLFW_KEY_D:
    case GLFW_KEY_RIGHT:
      cameraControl.moveRight = pressed;
      break;
    case GLFW_KEY_Q:
    case GLFW_KEY_PAGE_UP:
      cameraControl.moveUp = pressed;
      break;
    case GLFW_KEY_E:
    case GLFW_KEY_PAGE_DOWN:
      cameraControl.moveDown = pressed;
      break;
    default:
      break;
  }

  if (imguiSystem) {
    imguiSystem->HandleKeyboard(key, pressed);
  }
}

void Engine::Update(TimeDelta deltaTime) {
  // Apply any entity removals requested by background threads.
  ProcessPendingEntityRemovals();

  // During background scene loading we avoid touching the live entity
  // list from the main thread. This lets the loading thread construct
  // entities/components safely while the main thread only drives the
  // UI/loading overlay.
  if (renderer && renderer->IsLoading()) {
    if (imguiSystem) {
      uint32_t rw, rh;
      renderer->GetSwapChainExtent(&rw, &rh);
      if (rw > 0 && rh > 0) {
        imguiSystem->HandleResize(rw, rh);
      }
      imguiSystem->NewFrame();
    }
    return;
  }

  // Update ImGui system
  if (imguiSystem) {
    uint32_t rw, rh;
    renderer->GetSwapChainExtent(&rw, &rh);
    if (rw > 0 && rh > 0) {
      imguiSystem->HandleResize(rw, rh);
    }
    imguiSystem->NewFrame();
  }

  // Update camera controls
  if (activeCamera) {
    UpdateCameraControls(deltaTime);
  }

  // Update all entities.
  // Do not hold `entitiesMutex` while calling `Entity::Update()`.
  // Background threads may need the unique lock to add entities during loading,
  // and holding a shared lock for a long time can starve them.
  std::vector<Entity *> snapshot; {
    std::shared_lock<std::shared_mutex> lk(entitiesMutex);
    snapshot.reserve(entities.size());
    for (auto& uptr : entities) {
      snapshot.push_back(uptr.get());
    }
  }
  for (Entity* entity : snapshot) {
    if (!entity || !entity->IsActive())
      continue;
    entity->Update(deltaTime);
  }
}

std::chrono::milliseconds Engine::CalculateDeltaTimeMs() {
  // Get current time using a steady clock to avoid system time jumps
  uint64_t currentTime = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch())
    .count());

  // Initialize lastFrameTimeMs on first call
  if (lastFrameTimeMs == 0) {
    lastFrameTimeMs = currentTime;
    return std::chrono::milliseconds(16); // ~16ms as a sane initial guess
  }

  // Calculate delta time in milliseconds
  uint64_t delta = currentTime - lastFrameTimeMs;

  // Update last frame time
  lastFrameTimeMs = currentTime;

  return std::chrono::milliseconds(static_cast<long long>(delta));
}

void Engine::HandleResize(int width, int height) const {
  if (height <= 0 || width <= 0) {
    return;
  }
  LOGI("Engine: HandleResize %dx%d", width, height);

  // Update the active camera's aspect ratio
  if (activeCamera) {
    activeCamera->SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
  }

  // Notify the renderer that the framebuffer has been resized
  if (renderer) {
    renderer->SetFramebufferResized();
  }

  // Notify ImGui system about the resize
  if (imguiSystem) {
    imguiSystem->HandleResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  }
}

void Engine::UpdateCameraControls(TimeDelta deltaTime) {
  if (!activeCamera)
    return;

  // Get a camera transform component
  auto* cameraTransform = activeCamera->GetOwner()->GetComponent<TransformComponent>();
  if (!cameraTransform)
    return;

  // Manual camera controls
  // Calculate movement speed
  float velocity = cameraControl.cameraSpeed * deltaTime.count() * .001f;

  // Check if ImGui wants to capture mouse input (updated in NewFrame)
  bool imguiWantsMouse = imguiSystem && imguiSystem->WantCaptureMouse();

  // INTERACTION LOCKING LOGIC:
  // If a touch began, we wait until ImGui has processed the first DOWN event (in NewFrame)
  // before deciding whether this drag belongs to the GUI or the 3D Scene.
  if (cameraControl.mouseLeftPressed) {
    if (cameraControl.isFirstFrameOfInteraction) {
      // This is the first frame (Update call) where the finger is DOWN.
      // ImGui's WantCaptureMouse now accurately reflects if the tap was on a window.
      cameraControl.startedOnImGui = imguiWantsMouse;
      cameraControl.isFirstFrameOfInteraction = false;
    }

    // Only apply rotation if the interaction started on the scene background
    if (!cameraControl.startedOnImGui) {
      float xOffset = cameraControl.pendingXOffset * cameraControl.mouseSensitivity;
      float yOffset = cameraControl.pendingYOffset * cameraControl.mouseSensitivity;

      cameraControl.yaw -= xOffset;
      cameraControl.pitch -= yOffset;
    }
  } else {
    // Reset locking state when finger is lifted
    cameraControl.isFirstFrameOfInteraction = true;
    cameraControl.startedOnImGui = false;
  }

  // Constrain pitch to avoid gimbal lock
  if (cameraControl.pitch > 89.0f)
    cameraControl.pitch = 89.0f;
  if (cameraControl.pitch < -89.0f)
    cameraControl.pitch = -89.0f;

  // Clear accumulated offsets after processing
  cameraControl.pendingXOffset = 0.0f;
  cameraControl.pendingYOffset = 0.0f;

  // Capture base orientation from GLTF camera once and then apply mouse deltas relative to it
  if (!cameraControl.baseOrientationCaptured) {
    // TransformComponent stores Euler in radians; convert to quaternion
    glm::vec3 baseEuler = cameraTransform->GetRotation();
    const glm::quat qx = glm::angleAxis(baseEuler.x, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat qy = glm::angleAxis(baseEuler.y, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat qz = glm::angleAxis(baseEuler.z, glm::vec3(0.0f, 0.0f, 1.0f));
    // Match CameraComponent::UpdateViewMatrix composition (q = qz * qy * qx)
    cameraControl.baseOrientation = qz * qy * qx;
    cameraControl.baseOrientationCaptured = true;
  }

  // Build delta orientation from yaw/pitch mouse deltas (degrees -> radians)
  const float yawRad = glm::radians(cameraControl.yaw);
  const float pitchRad = glm::radians(cameraControl.pitch);
  const glm::quat qDeltaY = glm::angleAxis(yawRad, glm::vec3(0.0f, 1.0f, 0.0f));
  const glm::quat qDeltaX = glm::angleAxis(pitchRad, glm::vec3(1.0f, 0.0f, 0.0f));
  // Apply yaw then pitch in the same convention as CameraComponent (ZYX overall), so delta = Ry * Rx
  glm::quat qDelta = qDeltaY * qDeltaX;
  glm::quat qFinal = cameraControl.baseOrientation * qDelta;

  // Derive camera basis directly from rotated axes to avoid ambiguity
  glm::vec3 right = glm::normalize(qFinal * glm::vec3(1.0f, 0.0f, 0.0f));
  glm::vec3 up = glm::normalize(qFinal * glm::vec3(0.0f, 1.0f, 0.0f));
  // Camera forward in world space.
  // Our view/projection conventions assume the camera looks down -Z in its local space.
  glm::vec3 front = glm::normalize(qFinal * glm::vec3(0.0f, 0.0f, -1.0f));

  // Get the current camera position
  glm::vec3 position = cameraTransform->GetPosition();

  // Apply movement based on input
  if (cameraControl.moveForward) {
    position += front * velocity;
  }
  if (cameraControl.moveBackward) {
    position -= front * velocity;
  }
  if (cameraControl.moveLeft) {
    position -= right * velocity;
  }
  if (cameraControl.moveRight) {
    position += right * velocity;
  }
  if (cameraControl.moveUp) {
    position += up * velocity;
  }
  if (cameraControl.moveDown) {
    position -= up * velocity;
  }

  // Update camera position
  cameraTransform->SetPosition(position);
  // Apply rotation to the camera transform based on GLTF base orientation plus mouse deltas
  // TransformComponent expects radians Euler (ZYX order in our CameraComponent).
  cameraTransform->SetRotation(glm::eulerAngles(qFinal));

  // Update camera target based on a direction
  glm::vec3 target = position + front;
  activeCamera->SetTarget(target);

  // Ensure the camera view matrix reflects the new transform immediately this frame
  activeCamera->ForceViewMatrixUpdate();
}

void Engine::HandleMouseHover(float mouseX, float mouseY) {
  // Update current mouse position for any systems that might need it
  currentMouseX = mouseX;
  currentMouseY = mouseY;
}
