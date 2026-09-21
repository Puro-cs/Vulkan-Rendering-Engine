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
#include "sandbox.h"

#include "camera_component.h"
#include "crash_reporter.h"
#include "engine.h"
#include "light_component.h"
#include "scene_loading.h"
#include "transform_component.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

// This file implements what sandbox.h declares. It is the only place where the classes of the
// sandbox file meet the classes of the engine.

// Constants
#if defined(NDEBUG)
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

/**
 * @brief The hidden part of the Sandbox: the engine and the objects that were handed out.
 */
struct Sandbox::Impl
{
	Engine engine;

	std::vector<std::unique_ptr<SceneObject>> objects;
	std::vector<std::unique_ptr<Camera>>      cameras;
	std::vector<std::unique_ptr<Light>>       lights;

	bool modelLoaded = false;
};

// The glTF loader names its entities "<model>_Material_<index>_<materialName>"
static std::string MaterialNameOf(const Entity *entity)
{
	const std::string &entityName = entity->GetName();
	const size_t       tagPos     = entityName.find("_Material_");
	if (tagPos == std::string::npos)
	{
		return std::string();
	}
	const std::string remainder      = entityName.substr(tagPos + std::string("_Material_").size());
	const size_t      nextUnderscore = remainder.find('_');
	if (nextUnderscore == std::string::npos)
	{
		return std::string();
	}
	return remainder.substr(nextUnderscore + 1);
}

// --- SceneObject ---

void SceneObject::SetPosition(const glm::vec3 &position)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->SetPosition(position);
		}
	}
}

void SceneObject::SetRotation(const glm::vec3 &degrees)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->SetRotation(glm::radians(degrees));
		}
	}
}

void SceneObject::SetScale(const glm::vec3 &scale)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->SetScale(scale);
		}
	}
}

void SceneObject::Move(const glm::vec3 &translation)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->Translate(translation);
		}
	}
}

void SceneObject::Rotate(const glm::vec3 &degrees)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->Rotate(glm::radians(degrees));
		}
	}
}

void SceneObject::Scale(const glm::vec3 &factors)
{
	for (Entity *entity : entities)
	{
		if (auto *transform = entity->GetComponent<TransformComponent>())
		{
			transform->Scale(factors);
		}
	}
}

SceneObject *SceneObject::Part(const std::string &materialName)
{
	// A part that was asked for before
	for (const auto &part : parts)
	{
		if (part->name == materialName)
		{
			return part.get();
		}
	}

	auto part  = std::make_unique<SceneObject>();
	part->name = materialName;
	for (Entity *entity : entities)
	{
		if (MaterialNameOf(entity) == materialName)
		{
			part->entities.push_back(entity);
		}
	}

	if (part->entities.empty())
	{
		std::cerr << "Part: the object \"" << name << "\" has no material \"" << materialName << "\". Its materials:";
		for (Entity *entity : entities)
		{
			std::cerr << " \"" << MaterialNameOf(entity) << "\"";
		}
		std::cerr << std::endl;
	}

	parts.push_back(std::move(part));
	return parts.back().get();
}

// --- Camera ---

void Camera::SetPosition(const glm::vec3 &position)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetPosition(position);
	}
}

void Camera::SetRotation(const glm::vec3 &degrees)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetRotation(glm::radians(degrees));
	}
}

void Camera::SetFieldOfView(float degrees)
{
	if (auto *camera = entity->GetComponent<CameraComponent>())
	{
		camera->SetFieldOfView(degrees);
	}
}

// --- Light ---

void Light::SetPosition(const glm::vec3 &position)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetPosition(position);
	}
}

void Light::SetRotation(const glm::vec3 &degrees)
{
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		transform->SetRotation(glm::radians(degrees));
	}
}

void Light::SetDirection(const glm::vec3 &direction)
{
	if (glm::length(direction) == 0.0f)
	{
		return;
	}
	// A light shines along the -Z axis of its transform (LightComponent::GetLight). The rotation
	// around x (pitch) and around y (yaw) that turns (0, 0, -1) into the direction:
	if (auto *transform = entity->GetComponent<TransformComponent>())
	{
		const glm::vec3 d = glm::normalize(direction);
		transform->SetRotation(glm::vec3(std::asin(d.y), std::atan2(-d.x, -d.z), 0.0f));
	}
}

void Light::SetColor(const glm::vec3 &color)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetColor(color);
	}
}

void Light::SetIntensity(float intensity)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetIntensity(intensity);
	}
}

void Light::SetRange(float range)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetRange(range);
	}
}

void Light::SetConeAngles(float innerDegrees, float outerDegrees)
{
	if (auto *light = entity->GetComponent<LightComponent>())
	{
		light->SetConeAngles(glm::radians(innerDegrees), glm::radians(outerDegrees));
	}
}

// --- Sandbox ---

Sandbox::Sandbox() :
    impl(std::make_unique<Impl>())
{}

Sandbox::~Sandbox()
{
	CrashReporter::GetInstance().Cleanup();
}

bool Sandbox::Initialize(const std::string &title, int width, int height)
{
	try
	{
		// Enable minidump generation for Release-only crashes (e.g., stack cookie failures / fast-fail).
		// Writes dumps under the current working directory (the build/run directory).
		CrashReporter::GetInstance().Initialize("crashes", "SimpleEngine", "1.0.0");

		// Initialize the engine
		if (!impl->engine.Initialize(title, width, height, ENABLE_VALIDATION_LAYERS))
		{
			throw std::runtime_error("Failed to initialize engine");
		}
		return true;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return false;
	}
}

void Sandbox::Run()
{
	try
	{
		// The engine shows its loading overlay until a load cycle has ended. A scene without a
		// loaded model never starts one, so end it here.
		if (!impl->modelLoaded)
		{
			if (auto *renderer = impl->engine.GetRenderer())
			{
				renderer->SetLoading(false);
			}
		}

		// Run the engine
		impl->engine.Run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

Camera *Sandbox::CreateCamera(const std::string &name)
{
	// Create a camera entity
	Entity *cameraEntity = impl->engine.CreateEntity(name);

	// Add a transform component to the camera
	cameraEntity->AddComponent<TransformComponent>();

	// Add a camera component to the camera entity
	cameraEntity->AddComponent<CameraComponent>();
	// Camera aspect ratio will be set by the engine during initialization or resize events.

	auto camera    = std::make_unique<Camera>();
	camera->entity = cameraEntity;
	impl->cameras.push_back(std::move(camera));
	return impl->cameras.back().get();
}

void Sandbox::SetActiveCamera(Camera *camera)
{
	if (!camera)
	{
		return;
	}
	// Set the camera as the active camera
	impl->engine.SetActiveCamera(camera->entity->GetComponent<CameraComponent>());
}

Light *Sandbox::CreateLight(const std::string &name, LightType type)
{
	// Create a light entity
	Entity *lightEntity = impl->engine.CreateEntity(name);

	// Add a transform component to the light. A light shines along the -Z axis of its transform.
	lightEntity->AddComponent<TransformComponent>();

	// Add a light component to the light entity
	auto *lightComponent = lightEntity->AddComponent<LightComponent>();
	switch (type)
	{
		case LightType::Directional:
			lightComponent->SetType(ExtractedLight::Type::Directional);
			break;
		case LightType::Point:
			lightComponent->SetType(ExtractedLight::Type::Point);
			break;
		case LightType::Spot:
			lightComponent->SetType(ExtractedLight::Type::Spot);
			break;
	}

	auto light    = std::make_unique<Light>();
	light->entity = lightEntity;
	impl->lights.push_back(std::move(light));
	return impl->lights.back().get();
}

SceneObject *Sandbox::LoadModel(const std::string &name, const std::string &file)
{
	auto object  = std::make_unique<SceneObject>();
	object->name = name;

	Renderer *renderer = impl->engine.GetRenderer();
	if (renderer)
	{
		// The loader replaces the glTF lights of the renderer. Keep the lights of the models that
		// were loaded before, so that they can be put back together with the new ones.
		const std::vector<ExtractedLight> lightsBefore = renderer->GetStaticLights();
		renderer->SetStaticLights({});
		const size_t entityCountBefore = impl->engine.GetEntities().size();

		// The loading flags of the tutorial's main.cpp. The model is loaded right here instead of
		// on a background thread, so the entities exist when the call returns.
		renderer->SetLoading(true);
		renderer->SetLoadingPhase(Renderer::LoadingPhase::Textures);
		impl->modelLoaded = true;
		if (!LoadGLTFModel(&impl->engine, file, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)))
		{
			std::cerr << "LoadModel: \"" << file << "\" could not be loaded; the object \"" << name << "\" is empty" << std::endl;
		}

		// The parts of the object are the mesh entities that the loader created
		const auto &entities = impl->engine.GetEntities();
		for (size_t i = entityCountBefore; i < entities.size(); ++i)
		{
			if (entities[i]->GetComponent<MeshComponent>())
			{
				object->entities.push_back(entities[i].get());
			}
		}

		// A second model appends its glTF lights instead of replacing the list
		std::vector<ExtractedLight> lights = lightsBefore;
		const std::vector<ExtractedLight> &loadedLights = renderer->GetStaticLights();
		lights.insert(lights.end(), loadedLights.begin(), loadedLights.end());
		renderer->SetStaticLights(lights);
	}

	impl->objects.push_back(std::move(object));
	return impl->objects.back().get();
}

SceneObject *Sandbox::CreateSphere(const std::string &name, float radius)
{
	auto object  = std::make_unique<SceneObject>();
	object->name = name;

	Entity *sphereEntity = impl->engine.CreateEntity(name);
	sphereEntity->AddComponent<TransformComponent>();
	auto *mesh = sphereEntity->AddComponent<MeshComponent>();
	mesh->CreateSphere(radius);
	object->entities.push_back(sphereEntity);

	// The renderer creates the GPU buffers of a mesh only when it is asked to. Without this
	// request the sphere would never be drawn.
	if (auto *renderer = impl->engine.GetRenderer())
	{
		renderer->EnqueueEntityPreallocationBatch({sphereEntity});
	}

	impl->objects.push_back(std::move(object));
	return impl->objects.back().get();
}

bool Sandbox::CreatePipeline(const std::string &name, const std::string &shaderFile, const PipelineSettings &settings)
{
	auto *renderer = impl->engine.GetRenderer();
	return renderer && renderer->CreatePipeline(name, shaderFile, settings);
}

bool Sandbox::AddToPipeline(const std::string &name, SceneObject *object)
{
	auto *renderer = impl->engine.GetRenderer();
	if (!renderer || !object)
	{
		return false;
	}
	for (Entity *entity : object->entities)
	{
		// An unknown name is reported by the renderer; one report is enough
		if (!renderer->AddToPipeline(name, entity))
		{
			return false;
		}
	}
	return true;
}
