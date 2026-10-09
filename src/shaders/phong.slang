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
// Combined vertex and fragment shader for textured mesh rendering

// Import shared utility modules
import common_types;

// Input from vertex buffer
struct VSInput {
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float2 UV;
    [[vk::location(3)]] float4 Tangent;

    // Per-instance data
    [[vk::location(4)]] column_major float4x4 InstanceModelMatrix;
    [[vk::location(8)]] float4 InstanceNormal0;
    [[vk::location(9)]] float4 InstanceNormal1;
    [[vk::location(10)]] float4 InstanceNormal2;
};

// Output from vertex shader / Input to fragment shader
struct VSOutput {
    float4 Position : SV_POSITION;
    float3 WorldPos;
    float3 Normal;
    float2 UV : TEXCOORD0;
};

// Bindings
[[vk::binding(0, 0)]] ConstantBuffer<UniformBufferObject> ubo;
[[vk::binding(1, 0)]] Sampler2D baseColorMap;
[[vk::binding(6, 0)]] StructuredBuffer<LightData> lightBuffer;

[[vk::push_constant]] PushConstants material;

// Phong model of the lecture:
//   c(x) = c_amb * k_amb + c_light / (k1 + k2 * d) * (k_diff * <n, L> + k_spec * O_spec * <n, H>^n)
// Material constants for gold, from the lecture's table (exponent n, k_diff, k_spec; the table has no k_amb,
// so the ambient term uses k_diff, and k_spec is already the colour of the highlight, so O_spec = 1).
static const float3 kAmbient  = float3(0.5, 0.5, 0.0);    // k_amb = k_diff
static const float3 kDiffuse  = float3(0.5, 0.5, 0.0);    // k_diff
static const float3 kSpecular = float3(1.0, 1.0, 0.0);    // k_spec (times O_spec = 1)
static const float  shininess = 32.0;                     // n
static const float3 cAmbient  = float3(0.2, 0.2, 0.2);    // c_amb: the ambient light of the scene
static const float  k1 = 1.0;                             // distance attenuation c_light / (k1 + k2 * d)
static const float  k2 = 0.0;

// Vertex shader entry point
[[shader("vertex")]]
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4x4 instanceModelMatrix = input.InstanceModelMatrix;
    float4 worldPos = mul(ubo.model, mul(instanceModelMatrix, float4(input.Position, 1.0)));
    output.Position = mul(ubo.proj, mul(ubo.view, worldPos));
    output.WorldPos = worldPos.xyz;

    // Transform normals correctly: first by the per-instance normal matrix,
    // then by the entity model 3x3 (avoid double-applying instance transform).
    float3x3 instNormal = transpose(float3x3(input.InstanceNormal0.xyz, input.InstanceNormal1.xyz, input.InstanceNormal2.xyz));
    float3x3 model3x3 = (float3x3)ubo.model;
    float3 worldNormal = normalize(mul(model3x3, mul(instNormal, input.Normal)));
    output.Normal = worldNormal;

    output.UV = input.UV;

    return output;
}

// Fragment shader entry point
[[shader("fragment")]]
float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 N = normalize(input.Normal);                       // n: the surface normal
    float3 V = normalize(ubo.camPos.xyz - input.WorldPos);    // direction to the camera

    // ambient
    float3 color = cAmbient * kAmbient;

    for (uint i = 0; i < (uint)ubo.lightCount; ++i)
    {
        LightData light = lightBuffer[i];
        float3 L;            // direction to the light
        float  d = 0.0;      // distance to the light
        if (light.lightType == 1)
        {
            L = normalize(-light.position.xyz);    // directional light: position.xyz is the direction it travels
        }
        else
        {
            float3 toLight = light.position.xyz - input.WorldPos;
            d = length(toLight);
            L = toLight / d;
        }
        float3 cLight = light.color.rgb / (k1 + k2 * d);    // c_light with the distance attenuation
        float3 H = normalize(L + V);                         // half vector between light and camera

        // diffuse
        float3 diffuse = kDiffuse * max(dot(N, L), 0.0);

        // specular
        float3 specular = kSpecular * pow(max(dot(N, H), 0.0), shininess);

        color += cLight * (diffuse + specular);
    }

    return float4(color, 1.0);
}
