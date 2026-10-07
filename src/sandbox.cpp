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

// This file is the skeleton for the worksheet: the example scene of the branch `main` with one gap.
// Which calls belong into the gap, and in which order, is in the wiki of the repository:
// https://github.com/Puro-cs/Vulkan-Rendering-Engine/wiki

// Constants
constexpr int WINDOW_WIDTH  = 800;
constexpr int WINDOW_HEIGHT = 600;

/**
 * @brief Set up the scene. Every scene object is created here, before rendering starts, in any order.
 * @param sandbox The engine to set up the scene in.
 */
void SetupScene(Sandbox &sandbox)
{
	// A default camera, so that the engine renders before a scene exists (the scene of task 3 replaces this line)
	sandbox.SetActiveCamera(sandbox.CreateCamera("Camera"));
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

	// 1. The window, the Vulkan instance, the device and the swap chain (calls 1 to 4): this group is the gap.
	//    The first call takes ("Sandbox", WINDOW_WIDTH, WINDOW_HEIGHT); the other three take no arguments.

	// 2. The rendering set-up and the engine's own pipelines (calls 5 and 6)
	sandbox.InitializeRendering();
	sandbox.CreatePipelines();

	// 3. The command buffers and the synchronization (calls 7 and 8)
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
