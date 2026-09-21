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
#include "sandbox.h"        // the only engine header this file includes; it contains no Vulkan types

// Constants
constexpr int WINDOW_WIDTH  = 800;
constexpr int WINDOW_HEIGHT = 600;

/**
 * @brief Set up the scene. Every scene object is created here, before rendering starts, in any order.
 * @param sandbox The engine to set up the scene in.
 */
void SetupScene(Sandbox &sandbox)
{
	// Create a camera and make it the active one
	Camera *camera = sandbox.CreateCamera("Camera");
	camera->SetPosition({2.0f, 0.5f, -2.0f});
	camera->SetRotation({0.0f, 135.0f, 0.0f});
	sandbox.SetActiveCamera(camera);

	// Create a directional light. A light shines along the -Z axis; the rotation turns it down and sideways.
	Light *sun = sandbox.CreateLight("Sun", LightType::Directional);
	sun->SetRotation({-45.0f, 45.0f, 0.0f});
	sun->SetIntensity(3.0f);

	// Load a model. The call waits until the model is loaded.
	SceneObject *room = sandbox.LoadModel("Room", "../assets/viking_room/viking_room.gltf");
	room->SetRotation({-90.0f, 0.0f, 0.0f});

	// Create a simple mesh that needs no file
	SceneObject *sphere = sandbox.CreateSphere("Sphere", 0.2f);
	sphere->SetPosition({0.5f, 0.3f, -1.2f});
}

/**
 * @brief Desktop entry point.
 * @return The exit code.
 */
int main()
{
	Sandbox sandbox;

	// Initialize the engine
	if (!sandbox.Initialize("Sandbox", WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		return 1;
	}

	// Set up the scene
	SetupScene(sandbox);

	// Run the engine
	sandbox.Run();

	return 0;
}
