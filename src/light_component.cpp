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
#include "light_component.h"

// Most of the LightComponent class implementation is in the header file

// Returns the light with its position and direction in world space
ExtractedLight LightComponent::GetLight() const
{
	ExtractedLight light;
	light.type           = type;
	light.color          = color;
	light.intensity      = intensity;
	light.range          = range;
	light.innerConeAngle = innerConeAngle;
	light.outerConeAngle = outerConeAngle;

	auto transform = GetOwner()->GetComponent<TransformComponent>();
	if (transform)
	{
		const glm::mat4 &W = transform->GetModelMatrix();
		// Position from world transform origin
		light.position = glm::vec3(W * glm::vec4(0, 0, 0, 1));

		// Direction for directional/spot: transform -Z
		glm::mat3 rot   = glm::mat3(W);
		light.direction = glm::normalize(rot * glm::vec3(0.0f, 0.0f, -1.0f));
	}
	return light;
}
