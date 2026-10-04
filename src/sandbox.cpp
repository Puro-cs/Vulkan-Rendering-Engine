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

// This file is the starting point: the engine does not run yet. Which calls are needed to initialize
// the engine, to set up a scene and to render a frame, and in which order, is explained step by step
// in the wiki: docs/wiki

/**
 * @brief Set up the scene. Every scene object is created here, before rendering starts, in any order.
 * @param sandbox The engine to set up the scene in.
 */
void SetupScene(Sandbox &sandbox)
{
}

/**
 * @brief Desktop entry point.
 * @return The exit code.
 */
int main()
{
	Sandbox sandbox;

	while (sandbox.IsRunning())
	{
	}

	return 0;
}
