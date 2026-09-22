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

	// The initialization chain: eight calls in this order, each one a group of Vulkan steps.
	// A call that is missing or out of order is reported, and the engine does not render.
	sandbox.InitializeWindow("Sandbox", WINDOW_WIDTH, WINDOW_HEIGHT);
	sandbox.CreateInstance();
	sandbox.PickDevice();
	sandbox.CreateSwapChain();
	sandbox.InitializeRendering();
	sandbox.CreatePipelines();
	sandbox.CreateCommandBuffers();
	sandbox.CreateSyncObjects();

	// Set up the scene
	SetupScene(sandbox);

	// The render loop: six calls per frame in this order, until the window is closed. A call that is
	// missing or out of order is reported by the next one, and rendering stops. The engine's loading
	// overlay covers the first frames while the meshes and textures are uploaded.
	while (sandbox.IsRunning())
	{
		sandbox.BeginFrame();        // wait until the GPU is done with this frame slot, acquire a swap chain image
		sandbox.UpdateScene();       // camera controls, terminal commands, lights and transforms into the uniform buffers
		sandbox.BeginRendering();    // begin the command buffer, clear the color and depth attachments
		sandbox.DrawScene();         // per object: bind its pipeline, descriptor sets and buffers, then draw
		sandbox.EndRendering();      // the engine's UI on top, end the command buffer
		sandbox.EndFrame();          // submit the command buffer, present the image
	}

	return 0;
}
