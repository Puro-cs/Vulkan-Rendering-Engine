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

#include <glm/glm.hpp>

#include "component.h"
#include "entity.h"
#include "model_loader.h"
#include "transform_component.h"

/**
 * @brief Component that makes an entity a light source.
 *
 * Position and direction come from the entity's TransformComponent. The renderer
 * collects the lights of all entities every frame.
 */
class LightComponent : public Component
{
  private:
	ExtractedLight::Type type = ExtractedLight::Type::Point;

	glm::vec3 color          = glm::vec3(1.0f);
	float     intensity      = 1.0f;
	float     range          = 100.0f;           // For point/spotlights
	float     innerConeAngle = 0.0f;             // For spotlights
	float     outerConeAngle = 0.785398f;        // For spotlights (45 degrees)

  public:
	/**
	 * @brief Constructor with optional name.
	 * @param componentName The name of the component.
	 */
	explicit LightComponent(const std::string &componentName = "LightComponent") :
	    Component(componentName)
	{}

	/**
	 * @brief Set the light type.
	 * @param newType The light type (directional, point or spot).
	 */
	void SetType(ExtractedLight::Type newType)
	{
		type = newType;
	}

	/**
	 * @brief Get the light type.
	 * @return The light type.
	 */
	ExtractedLight::Type GetType() const
	{
		return type;
	}

	/**
	 * @brief Set the light color.
	 * @param newColor The new color.
	 */
	void SetColor(const glm::vec3 &newColor)
	{
		color = newColor;
	}

	/**
	 * @brief Get the light color.
	 * @return The color.
	 */
	const glm::vec3 &GetColor() const
	{
		return color;
	}

	/**
	 * @brief Set the light intensity.
	 * @param newIntensity The new intensity. The color is multiplied by it.
	 */
	void SetIntensity(float newIntensity)
	{
		intensity = newIntensity;
	}

	/**
	 * @brief Get the light intensity.
	 * @return The intensity.
	 */
	float GetIntensity() const
	{
		return intensity;
	}

	/**
	 * @brief Set the range of a point light or spotlight.
	 * @param newRange The new range.
	 */
	void SetRange(float newRange)
	{
		range = newRange;
	}

	/**
	 * @brief Get the range.
	 * @return The range.
	 */
	float GetRange() const
	{
		return range;
	}

	/**
	 * @brief Set the cone angles of a spotlight.
	 * @param innerAngle The inner cone angle in radians.
	 * @param outerAngle The outer cone angle in radians.
	 */
	void SetConeAngles(float innerAngle, float outerAngle)
	{
		innerConeAngle = innerAngle;
		outerConeAngle = outerAngle;
	}

	float GetInnerConeAngle() const
	{
		return innerConeAngle;
	}
	float GetOuterConeAngle() const
	{
		return outerConeAngle;
	}

	/**
	 * @brief Get the light the way the renderer uploads it.
	 * @return The light, with position and direction taken from the entity's transform.
	 */
	ExtractedLight GetLight() const;
};
