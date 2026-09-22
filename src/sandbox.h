/* Copyright (c) 2026 Paulo Hoheisel
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
#pragma once

// The only engine header that sandbox.cpp includes. It contains no Vulkan types and no engine classes:
// the Vulkan API, ImGui, descriptor sets, memory management and the swap chain stay hidden behind it.

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "pipeline_settings.h"

class Entity;

/**
 * @brief The type of a light.
 */
enum class LightType
{
	Directional,
	Point,
	Spot
};

/**
 * @brief An object of the scene: a loaded model or a simple mesh.
 *
 * A loaded model consists of one part per material. All parts share the transform of the object,
 * so every call below is forwarded to all parts.
 */
class SceneObject
{
  public:
	/**
	 * @brief Get the name that was given to the object when it was created.
	 * @return The name.
	 */
	const std::string &GetName() const
	{
		return name;
	}

	/**
	 * @brief Set the position of the object.
	 * @param position The new position.
	 */
	void SetPosition(const glm::vec3 &position);

	/**
	 * @brief Set the rotation of the object.
	 * @param degrees The new rotation around the x, y and z axis in degrees.
	 */
	void SetRotation(const glm::vec3 &degrees);

	/**
	 * @brief Set the scale of the object.
	 * @param scale The new scale.
	 */
	void SetScale(const glm::vec3 &scale);

	/**
	 * @brief Move the object relative to its current position.
	 * @param translation The translation to apply.
	 */
	void Move(const glm::vec3 &translation);

	/**
	 * @brief Rotate the object relative to its current rotation.
	 * @param degrees The rotation to apply around the x, y and z axis in degrees.
	 */
	void Rotate(const glm::vec3 &degrees);

	/**
	 * @brief Scale the object relative to its current scale.
	 * @param factors The scale factors to apply.
	 */
	void Scale(const glm::vec3 &factors);

	/**
	 * @brief Get one part of a loaded model.
	 * @param materialName The name of the material of the part.
	 * @return The part. If the model has no such material, the error lists the materials it has and
	 *         an empty object is returned, so that calls on it do nothing.
	 */
	SceneObject *Part(const std::string &materialName);

  private:
	friend class Sandbox;

	std::string                               name;
	std::vector<Entity *>                     entities;
	std::vector<std::unique_ptr<SceneObject>> parts;
};

/**
 * @brief A camera of the scene.
 */
class Camera
{
  public:
	/**
	 * @brief Set the position of the camera.
	 * @param position The new position.
	 */
	void SetPosition(const glm::vec3 &position);

	/**
	 * @brief Set the rotation of the camera. Without a rotation the camera looks along the -Z axis.
	 * @param degrees The new rotation around the x, y and z axis in degrees.
	 */
	void SetRotation(const glm::vec3 &degrees);

	/**
	 * @brief Set the field of view.
	 * @param degrees The field of view in degrees.
	 */
	void SetFieldOfView(float degrees);

  private:
	friend class Sandbox;

	Entity *entity = nullptr;
};

/**
 * @brief A light of the scene.
 */
class Light
{
  public:
	/**
	 * @brief Set the position of the light (point lights and spotlights).
	 * @param position The new position.
	 */
	void SetPosition(const glm::vec3 &position);

	/**
	 * @brief Set the rotation of the light. Without a rotation the light shines along the -Z axis.
	 * @param degrees The new rotation around the x, y and z axis in degrees.
	 */
	void SetRotation(const glm::vec3 &degrees);

	/**
	 * @brief Set the direction in which the light shines (directional lights and spotlights).
	 * It replaces the rotation of the light.
	 * @param direction The direction; it does not have to be normalized.
	 */
	void SetDirection(const glm::vec3 &direction);

	/**
	 * @brief Set the color of the light.
	 * @param color The new color.
	 */
	void SetColor(const glm::vec3 &color);

	/**
	 * @brief Set the intensity of the light.
	 * @param intensity The new intensity. The color is multiplied by it.
	 */
	void SetIntensity(float intensity);

	/**
	 * @brief Set the range of a point light or spotlight.
	 * @param range The new range.
	 */
	void SetRange(float range);

	/**
	 * @brief Set the cone angles of a spotlight.
	 * @param innerDegrees The inner cone angle in degrees.
	 * @param outerDegrees The outer cone angle in degrees.
	 */
	void SetConeAngles(float innerDegrees, float outerDegrees);

  private:
	friend class Sandbox;

	Entity *entity = nullptr;
};

/**
 * @brief The engine as the sandbox file sees it.
 *
 * Every group of Vulkan steps is one call. The engine is initialized by the eight calls of the
 * initialization chain, in the order in which they are declared below; a call that is made out of
 * order, twice, or without the calls before it, reports the error and does nothing. All scene objects
 * are created in SetupScene(), after the chain and before rendering starts. The render loop is
 * `while (IsRunning())` around the six frame calls, in the order in which they are declared below; a
 * frame call that is missing or out of order is reported by the next call, and rendering stops. The
 * objects that the Create... and Load... calls return belong to the engine; they stay valid until the
 * Sandbox is destroyed.
 */
class Sandbox
{
  public:
	Sandbox();
	~Sandbox();

	// --- The initialization chain: eight calls in this order ---

	/**
	 * @brief Initialization call 1: the window and its input callbacks.
	 * @param title The title of the window.
	 * @param width The width of the window.
	 * @param height The height of the window.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool InitializeWindow(const std::string &title, int width, int height);

	/**
	 * @brief Initialization call 2: the Vulkan instance, the debug messenger and the window surface.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool CreateInstance();

	/**
	 * @brief Initialization call 3: the physical device (the GPU), the logical device with its queues
	 * and the memory pool.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool PickDevice();

	/**
	 * @brief Initialization call 4: the swap chain and its image views.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool CreateSwapChain();

	/**
	 * @brief Initialization call 5: dynamic rendering, the depth image and the off-screen color image.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool InitializeRendering();

	/**
	 * @brief Initialization call 6: the engine's own pipelines ("pbr" and the composite pass) with their
	 * descriptor set layouts, and the light buffers. Own pipelines are created after this call.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool CreatePipelines();

	/**
	 * @brief Create an own pipeline from a shader file. Not part of the chain, but it needs
	 * CreatePipelines(), which creates the layout that every pipeline shares.
	 * @param name The name of the pipeline, used by AddToPipeline(). "pbr" is the pipeline of the engine.
	 * @param shaderFile The shader, e.g. "shaders/toon.slang". It needs the entry points VSMain and PSMain.
	 * @param settings Cull mode, depth test and blending.
	 * @return True if the pipeline was created, false otherwise.
	 */
	bool CreatePipeline(const std::string &name, const std::string &shaderFile, const PipelineSettings &settings = {});

	/**
	 * @brief Initialization call 7: the command pool, the descriptor pool, the default textures and the
	 * command buffers.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool CreateCommandBuffers();

	/**
	 * @brief Initialization call 8: the semaphores and fences, the worker threads, the model loader and
	 * the UI. The engine is ready after this call.
	 * @return True if the call was made in order and succeeded, false otherwise.
	 */
	bool CreateSyncObjects();

	// --- The render loop: while (IsRunning()) { the six frame calls in this order } ---

	/**
	 * @brief The loop condition: true until the window is closed. Needs the complete initialization
	 * chain and an active camera. Processes the window events and the frame time of the coming frame.
	 * @return True while the window is open, false when it was closed or when rendering stopped after
	 *         a frame call was made out of order.
	 */
	bool IsRunning();

	/**
	 * @brief Frame call 1: wait until the GPU is done with this frame slot, acquire a swap chain image.
	 */
	void BeginFrame();

	/**
	 * @brief Frame call 2: camera controls, terminal commands, the lights and the transforms of all
	 * objects into the uniform buffers.
	 */
	void UpdateScene();

	/**
	 * @brief Frame call 3: begin the command buffer, clear the color and depth attachments.
	 */
	void BeginRendering();

	/**
	 * @brief Frame call 4: per object, bind its pipeline, descriptor sets and buffers, then draw; the
	 * tone mapping of the opaque scene; the transparent objects on top.
	 */
	void DrawScene();

	/**
	 * @brief Frame call 5: the engine's UI on top, end the command buffer.
	 */
	void EndRendering();

	/**
	 * @brief Frame call 6: submit the command buffer, present the image.
	 */
	void EndFrame();

	// --- The scene: called in SetupScene(), after the initialization chain ---

	/**
	 * @brief Create a camera.
	 * @param name The name of the camera.
	 * @return The camera.
	 */
	Camera *CreateCamera(const std::string &name);

	/**
	 * @brief Set the camera the scene is rendered with.
	 * @param camera The camera.
	 */
	void SetActiveCamera(Camera *camera);

	/**
	 * @brief Create a light.
	 * @param name The name of the light.
	 * @param type The type of the light.
	 * @return The light.
	 */
	Light *CreateLight(const std::string &name, LightType type);

	/**
	 * @brief Load a glTF model. The call waits until the model is loaded; its meshes and textures
	 * are uploaded during the first frames, behind the loading overlay of the engine.
	 * @param name The name of the object.
	 * @param file The path of the .gltf file. Its textures have to be KTX2 files.
	 * @return The object. If the model cannot be loaded, the error is printed and an empty object
	 *         is returned, so that calls on it do nothing.
	 */
	SceneObject *LoadModel(const std::string &name, const std::string &file);

	/**
	 * @brief Create a sphere.
	 * @param name The name of the object.
	 * @param radius The radius of the sphere.
	 * @return The object.
	 */
	SceneObject *CreateSphere(const std::string &name, float radius);

	/**
	 * @brief Add all parts of an object to a pipeline.
	 *
	 * An object that was added to no pipeline is drawn with "pbr". An object that was added to several
	 * pipelines is drawn once per pipeline, in the order in which the pipelines were created.
	 * @param name The name of the pipeline.
	 * @param object The object to add.
	 * @return True if the object was added, false if there is no pipeline with this name.
	 */
	bool AddToPipeline(const std::string &name, SceneObject *object);

  private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};
