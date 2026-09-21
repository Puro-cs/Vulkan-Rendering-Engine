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
#include "camera_component.h"
#include "crash_reporter.h"
#include "engine.h"
#include "light_component.h"
#include "scene_loading.h"
#include "transform_component.h"

#include <iostream>
#include <stdexcept>
#include <thread>

// Constants
constexpr int WINDOW_WIDTH  = 800;
constexpr int WINDOW_HEIGHT = 600;
#if defined(NDEBUG)
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

/**
 * @brief Set up a simple scene with a camera and some objects.
 * @param engine The engine to set up the scene in.
 */
void SetupScene(Engine *engine)
{
	// Create a camera entity
	Entity *cameraEntity = engine->CreateEntity("Camera");
	if (!cameraEntity)
	{
		throw std::runtime_error("Failed to create camera entity");
	}

	// Add a transform component to the camera
	auto *cameraTransform = cameraEntity->AddComponent<TransformComponent>();
	cameraTransform->SetPosition(glm::vec3(2.0f, 0.5f, -2.0f));
	cameraTransform->SetRotation(glm::vec3(glm::radians(0.0f), glm::radians(135.0f), 0.0f));

	// Add a camera component to the camera entity
	auto *camera = cameraEntity->AddComponent<CameraComponent>();
    // Camera aspect ratio will be set by the engine during initialization or resize events.

    // Set the camera as the active camera
	engine->SetActiveCamera(camera);

	// Create a light entity
	Entity *sunEntity = engine->CreateEntity("Sun");
	if (!sunEntity)
	{
		throw std::runtime_error("Failed to create sun entity");
	}

	// Add a transform component to the light. A light shines along the -Z axis of its transform.
	auto *sunTransform = sunEntity->AddComponent<TransformComponent>();
	sunTransform->SetRotation(glm::vec3(glm::radians(-45.0f), glm::radians(45.0f), 0.0f));

	// Add a light component to the light entity
	auto *sun = sunEntity->AddComponent<LightComponent>();
	sun->SetType(ExtractedLight::Type::Directional);
	sun->SetIntensity(3.0f);

	// Kick off GLTF model loading on a background thread so the main loop
	// can start and render the UI/progress bar while the scene is being
	// constructed. Engine::Update will avoid updating entities while
	// loading is in progress to prevent data races.
	if (auto *renderer = engine->GetRenderer())
	{
		renderer->SetLoading(true);
		renderer->SetLoadingPhase(Renderer::LoadingPhase::Textures);

		// TEMPORARY TEST of planned change 4 (remove again): an own pipeline from the template shader
		renderer->CreatePipeline("template", "shaders/template.slang");
	}
	std::thread([engine] {
      LoadGLTFModel(engine, "../assets/viking_room/viking_room.gltf", glm::vec3(0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(1.0f));

      // TEMPORARY TEST of planned change 4 (remove again): draw every part of the room with "template"
      for (const auto &entity : engine->GetEntities())
      {
        if (entity->GetComponent<MeshComponent>())
        {
          engine->GetRenderer()->AddToPipeline("template", entity.get());
        }
      }
    }).detach();
}

/**
 * @brief Desktop entry point.
 * @return The exit code.
 */
int main(int, char *[])
{
	try
	{
		// Enable minidump generation for Release-only crashes (e.g., stack cookie failures / fast-fail).
		// Writes dumps under the current working directory (the build/run directory).
		CrashReporter::GetInstance().Initialize("crashes", "SimpleEngine", "1.0.0");

		// Create the engine
		Engine engine;

		// Initialize the engine
		if (!engine.Initialize("Simple Engine", WINDOW_WIDTH, WINDOW_HEIGHT, ENABLE_VALIDATION_LAYERS))
		{
			throw std::runtime_error("Failed to initialize engine");
		}

		// Set up the scene
		SetupScene(&engine);

		// Run the engine
		engine.Run();

		CrashReporter::GetInstance().Cleanup();

		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		CrashReporter::GetInstance().Cleanup();
		return 1;
	}
}
