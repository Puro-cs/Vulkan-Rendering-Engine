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

// This header contains no Vulkan types, so the sandbox file of the students can include it.

/**
 * @brief Which side of a triangle a pipeline does not draw.
 */
enum class CullMode
{
	None,
	Front,
	Back
};

/**
 * @brief The settings of a pipeline that the user chooses.
 *
 * Together with the shader file these are the three things a pipeline is made of for the user.
 * Everything else (vertex layout, topology, blending, multisampling, pipeline layout, attachment formats)
 * is fixed by the engine.
 */
struct PipelineSettings
{
	CullMode cullMode  = CullMode::None;
	bool     depthTest = true;
};
