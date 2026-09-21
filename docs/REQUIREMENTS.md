# Technical requirements

The requirements table of Paulo Hoheisel's bachelor thesis (HTWK Leipzig) that this engine has to satisfy, and how far
the code meets each entry. IDs are the `\TR{...}` labels of the thesis so both documents can be cross-referenced;
priorities are the MoSCoW classes used there; "Thesis refs" lists the `\ref` targets of the original entry. The German
wording is summarised, not translated verbatim.

Status: **met**, **partly**, **not met**. Checked on 2026-09-16 against `src/` (file map in `docs/ARCHITECTURE.md`);
revised on 2026-09-20 after the ray-query render mode was removed (no status changed, the light gap of
`tr:szenenmanagement` got wider) and on 2026-09-21 after the acceleration structures, Forward+, planar reflections and
the dead panel controls were removed (no status changed; the engine got closer to the intended use, students writing
simple shaders, because a mesh shader now sees seven bindings instead of fourteen and no hidden depth pre-pass).
Also on 2026-09-21 the owner amended two requirement texts, with no code change: `architektur_techstack` names tinygltf
(glTF 2.0) instead of tinyobjloader and went from partly to met; `basis_infrastruktur` names Vulkan 1.3 dynamic
rendering instead of render pass objects (`VkRenderPass` / `VkFramebuffer`) and is met without a note. The owner
also removed the requirement `shader_verwaltung` (own C++ structs handed to shaders through set methods, thesis refs
`fa:2_shader`, `ns:4_material_shader`): student shaders work with what the engine provides, their own parameters are
constants in the shader file. The owner also amended `lifecycle`: students call the frame steps in a fixed order, and
one `DrawScene()` call replaces bundling the draw commands themselves (status unchanged, not met). The planned
student-facing interface is described in `docs/ROADMAP.md`.
Every gap that needs code the tutorial does not have falls under Rule 1 of `docs/CODE_CHANGE_RULES.md` (proposal,
owner approval); none of them can be closed by deletion alone.

## Overview

| ID | Priority | Requirement | Status |
|---|---|---|---|
| `architektur_techstack` | Must | Component architecture; C++20, Vulkan SDK, CMake, GLFW, GLM, tinygltf (glTF 2.0), Slang; Windows 11 | met |
| `basis_infrastruktur` | Must | Instance, swap chain, dynamic rendering (Vulkan 1.3), colour and depth attachments preconfigured and hidden | met |
| `synchronisation_speicherverwaltung` | Must | Frames in flight, command buffers, sync objects, GPU memory hidden | met |
| `pipeline_shader_automatisierung` | Must | Abstract pipeline configurations translated to Vulkan; shaders compiled to `VkShaderModule` | partly (no configurable pipelines) |
| `descriptor` | Must | Descriptor pools, set layouts, UBO mapping hidden | met |
| `input_handling` | Must | GLFW input captured and turned into scene navigation internally | met |
| `texture_pipeline` | Must | Whole texture life cycle hidden | met (KTX2 only) |
| `lifecycle` | Must | Students drive the frame life cycle through simplified calls in a fixed order | not met |
| `einfache_pipeline` | Must | Shader assignment and pipeline state through simplified abstractions | not met |
| `tr:szenenmanagement` | Must | Load and transform models, cameras, lights without manual buffers; auto-translate to UBO / push constants | partly |
| `ascii_pipeline` | Could | ASCII rendering of the pipeline in the terminal | not met |
| `validation_layer` | Must | Validation layers wired in, messages readable | met |
| `keine_audio_physik` | Won't | No audio, no physics | met |

## Requirements in detail

### `architektur_techstack` — Architecture and technology stack (Must)

Thesis refs: `ap:component`, WB 1-7, `app:bestandsaufnahme_pc-pool`.

**Requirement.** Component-based architecture. Mandatory technologies: C++20, the current Vulkan SDK, CMake, GLFW, GLM,
tinygltf (glTF 2.0 as model format) and Slang as shader language. Development must be possible on Windows 11.
(Amended by the owner on 2026-09-21: the original entry named tinyobjloader.)

**Code today.** `Entity` + `Component` with `TransformComponent`, `CameraComponent`, `MeshComponent` (`src/entity.h`,
`src/component.h`). `CXX_STANDARD 20` in `CMakeLists.txt`; Vulkan SDK 1.4 through `VULKAN_SDK`; CMake presets for
Visual Studio 2026; GLFW in `DesktopPlatform`; GLM throughout; models are parsed with tinygltf 3.0.0 from `vcpkg.json`
(`src/model_loader.cpp`, implementation compiled into that one translation unit, glTF 2.0); every shader in
`src/shaders/` is Slang and compiled by `slangc`. Builds and runs on Windows 11 (`docs/BUILD.md`).

**Gap.** None. tinyobjloader is not used and not listed in `vcpkg.json`; there is no OBJ path, neither here nor
upstream (the tutorial only shipped a find module for it). tinygltf decodes no images here (`TINYGLTF_NO_STB_IMAGE`);
that limitation belongs to `texture_pipeline`.

### `basis_infrastruktur` — Encapsulated base infrastructure (Must)

Thesis refs: `ki:1_swapchain`, `ki:2_render pass`.

**Requirement.** Instance creation, swap chain management, the pass setup with Vulkan 1.3 dynamic rendering
(`vkCmdBeginRendering`, no `VkRenderPass` / `VkFramebuffer` objects) and the colour and depth attachments are
preconfigured statically and managed in the background. (Amended by the owner on 2026-09-21: the original entry named
render pass setup and frame buffers, i.e. render pass objects. The thesis label `ki:2_render pass` is unchanged.)

**Code today.** `Renderer::Initialize()` (`src/renderer_core.cpp`) creates instance, debug messenger, surface, device,
swap chain, image views, depth image, command pools and sync objects; a resize recreates the swap chain
(`src/renderer_rendering.cpp`). `main.cpp` only calls `Engine::Initialize()`. Dynamic rendering is mandatory: device
selection rejects a GPU without the `dynamicRendering` feature and the feature is enabled at device creation
(`src/renderer_core.cpp`). `setupDynamicRendering()` (`src/renderer_rendering.cpp`, run again on every swap-chain
recreation) prepares the `vk::RenderingAttachmentInfo` for colour and depth and the `vk::RenderingInfo`; the four passes
of a frame (opaque to the off-screen image, composite, transparent, ImGui) are each a `beginRendering` /
`endRendering` pair that binds its attachments. Every pipeline is created without a render pass handle, with a
`vk::PipelineRenderingCreateInfo` in its `pNext` chain (`src/renderer_pipelines.cpp`, `src/imgui_system.cpp`). Image
layout transitions are explicit `pipelineBarrier2` calls, since there are no subpass dependencies. No `VkRenderPass`
or `VkFramebuffer` exists in `src/`.

**Gap.** None.

### `synchronisation_speicherverwaltung` — Synchronisation and memory management (Must)

Thesis refs: `ki:3_asynchron`, `ki:4_memory`.

**Requirement.** Frames in flight with per-frame command buffers and sync objects, plus the allocation of GPU memory,
fully hidden inside the engine.

**Code today.** `MAX_FRAMES_IN_FLIGHT = 2`; per-frame command buffers, image-available and render-finished semaphores
and in-flight fences (`createSyncObjects()`); a timeline semaphore for uploads; `MemoryPool` sub-allocates device
memory for all buffers and images. All private to `Renderer`.

**Gap.** None.

### `pipeline_shader_automatisierung` — Pipeline and shader automation (Must)

Thesis refs: `ki:5_pipeline`.

**Requirement.** The engine translates abstracted pipeline configurations into the Vulkan pipeline objects itself and
compiles the shader files into `VkShaderModule` instances.

**Code today.** `CMakeLists.txt` compiles every `src/shaders/*.slang` (except the imported modules) with `slangc` and
`spirv-opt`; a new `.slang` file is picked up by the glob without further edits. `Renderer::createShaderModule()`
(`src/renderer_utils.cpp`) wraps the SPIR-V into `vk::raii::ShaderModule`. All mesh pipelines are created in
`src/renderer_pipelines.cpp` (the ImGui pipeline in `src/imgui_system.cpp`).

**Gap.** The "abstracted pipeline configuration" does not exist. Each pipeline (textured mesh, the PBR variants
opaque / blended / premultiplied / glass, composite, ImGui) is hard-coded with its shader file name, vertex layout,
descriptor layout and fixed-function state. The unused `Pipeline` class (`src/pipeline.h`) hard-coded the same
pipelines and was no abstraction either; it was deleted on 2026-09-21 (planned change 1). Slang compilation is a build step, not something the engine does at run time;
acceptable, but the thesis should say so.

### `descriptor` — Hidden descriptor management (Must)

Thesis refs: `ki:6_descriptor_management`.

**Requirement.** Descriptor pools, descriptor set layouts and the memory mapping of UBOs that inject CPU data into the
shaders are handled in the background.

**Code today.** The descriptor pool, the PBR set layouts (the basic one went on 2026-09-21), per-entity per-frame uniform
buffers with persistent mapping, deferred descriptor updates (`MarkEntityDescriptorsDirty()`,
`ProcessDirtyDescriptorsForFrame()`) and refreshes when textures stream in all live in `src/renderer_pipelines.cpp`
and `src/renderer_resources.cpp`. The standalone `DescriptorManager` is unused.

**Gap.** None.

### `input_handling` — Engine-internal input handling and navigation (Must)

Thesis refs: `ki:7_input_handling`.

**Requirement.** Mouse and keyboard events are captured through GLFW and evaluated inside the engine; navigation in
the scene works without the student implementing or exposing an input interface.

**Code today.** `DesktopPlatform` registers the GLFW callbacks; `Engine::handleKeyInput()` / `handleMouseInput()` map
`W A S D` and the arrows, `Q E` / PageUp PageDown and left-drag; `Engine::UpdateCameraControls()` moves the active
camera every frame and yields to ImGui when the pointer is over a panel (`src/engine.cpp`). `main.cpp` contains no
input code.

**Gap.** None. Bindings are fixed; `Esc` does not quit (close the window instead).

### `texture_pipeline` — Encapsulated texture pipeline (Must)

Thesis refs: `ki:8_texture_pipeline`.

**Requirement.** The full life cycle of textures is managed in the background.

**Code today.** `LoadTexture()` / `LoadTextureAsync()` (`src/renderer_resources.cpp`): thread-pool decode with libktx,
staging upload, layout transitions, mip generation, image view and sampler, sRGB / linear format choice, shared
default textures, descriptor refresh when a texture arrives, cleanup with the renderer. Students only name a texture
file (glTF material or `MeshComponent::SetTexturePath()`).

**Gap.** Only KTX2 is decoded (`TINYGLTF_NO_STB_IMAGE`). PNG / JPEG references load the geometry with the default
albedo and a warning. For student-made assets either PNG decoding (stb is in vcpkg but not wired in, Rule 1) or a
documented `toktx` conversion step is needed. Same limitation upstream.

### `lifecycle` — Control of the render life cycle (Must)

Thesis refs: `ns:1_CB_rendering`.

**Requirement.** Students control the rendering flow explicitly: they define the frame life cycle through simplified
calls in a fixed order (begin the frame, update the scene, begin rendering, draw the scene, end rendering, end the
frame). Recording the individual draw commands into the command buffer stays inside the engine. (Amended by the owner
on 2026-09-21: in the original entry the students bundled the draw commands in a command buffer themselves. The
thesis label `ns:1_CB_rendering` is unchanged.)

**Code today.** `Engine::Run()` owns the loop; `Engine::Render()` is private and hands an entity snapshot to
`Renderer::Render()` (`src/renderer_rendering.cpp`, roughly lines 673-1506), which acquires the image, uploads
lights, culls, records every pass and ImGui, submits and presents. `main.cpp` calls `Initialize`, `SetupScene`, `Run`
and nothing else. `GetCurrentCommandBuffer()` returns the raw `vk::raii::CommandBuffer`, the native object, not a
simplified call.

**Gap.** Not met. Needed: the frame API of the student-facing interface in `docs/ROADMAP.md`: six calls with an order
check (`BeginFrame`, `UpdateScene`, `BeginRendering`, `DrawScene`, `EndRendering`, `EndFrame`) that the student calls
from an own loop, while acquire, the recording of the draw commands, submit and synchronisation stay inside
`Renderer`. The tutorial has no such API (Rule 1).

### `einfache_pipeline` — Explicit but simplified pipeline configuration (Must)

Thesis refs: `ns:2_pipeline_konfiguration`.

**Requirement.** Students assign shaders and define pipeline state through simplified abstractions, without writing
native Vulkan structures.

**Code today.** Pipelines are chosen internally, and the user has no choice left: the basic / PBR toggle of the
ImGui "Renderer" panel was removed on 2026-09-21 (planned change 2; `RenderMode`, Forward+ and planar reflections
went on 2026-09-20 / 2026-09-21). Every opaque object uses the PBR pipeline; transparent and glass materials pick
their pipeline from the glTF material. The shader name is a literal (`"shaders/pbr.spv"`) in
`src/renderer_pipelines.cpp`. Using an own shader means editing `createPBRPipeline()` and its
`vk::GraphicsPipelineCreateInfo`.

**Gap.** Not met. Needed: a pipeline description (vertex and fragment shader, topology, cull mode, depth test,
blending) that the engine turns into a pipeline, and a way to attach it to an entity or material. Rule 1.

### `tr:szenenmanagement` — Scene management and data handling (Must)

Thesis refs: `fa:1_szenenmanagement`, `ns:3_geometrie_assets`, `ns:5_kamera_licht`.

**Requirement.** Load and transform 3D models, cameras and lights without allocating vertex or index buffers by hand;
the engine translates the scene data into UBOs / push constants and provides them to the shaders.

**Code today.** `LoadGLTFModel()` (`src/scene_loading.cpp`) creates one entity per material mesh with
`TransformComponent` and `MeshComponent` (CPU vertices, indices, instances); the renderer allocates vertex, index and
instance buffers at the frame safe point, fills the per-entity UBO (`UniformBufferObject`: model / view / proj /
camPos ...) and the `MaterialProperties` push constants every frame. Camera: `CameraComponent` on an entity,
optionally overwritten by the first glTF camera. Lights: `KHR_lights_punctual` lights and emissive materials become
`ExtractedLight` entries in a storage buffer; `Renderer::SetStaticLights()` also accepts a hand-built vector.

**Gap.** Partly met.

- Lights are not components: no `LightComponent`, no entity-level API to add or move a light, only the
  renderer-level vector.
- The raster shader `pbr.slang` ignores the light buffer and uses a fixed direction (1, 1, 1) plus ambient (the
  second one, `texturedMesh.slang`, did the same and was removed on 2026-09-21). The engine side is in place: the scene lights are uploaded every frame to a storage buffer bound at
  PBR set 0, binding 6, with the count in `ubo.lightCount`. But no shader reads it. `ray_query.slang` was the one
  shader that shaded with the lights (removed 2026-09-20) and Forward+ only culled them (removed 2026-09-21), so
  "light data provided to the shaders" holds for no shading pass. Closing this needs a raster shader that declares
  binding 6 and loops over the lights (Rule 1); it is also an obvious first exercise for students.
- Meshes built in code (`MeshComponent::CreateSphere()`, `SetVertices()`) get GPU buffers only if the caller also
  invokes `Renderer::EnqueueEntityPreallocationBatch()`; entities without resources are skipped by `Render()`.

### `ascii_pipeline` — ASCII pipeline visualisation (Could)

Thesis refs: `vp:pipeline`.

**Requirement.** At run time the Vulkan render pipeline is drawn as ASCII art in the terminal to support learning.

**Code today.** Nothing; no pipeline introspection or printing exists, neither in the port nor upstream.

**Gap.** Not met (optional). Rule 1.

### `validation_layer` — Error handling with validation layers (Must)

Thesis refs: `vl:validation`.

**Requirement.** The Vulkan validation layers are wired in so that wrong commands are caught and reported as
understandable messages.

**Code today.** `VK_LAYER_KHRONOS_validation` is requested when `ENABLE_VALIDATION_LAYERS` is set (`main.cpp`: true
in Debug, false in Release), availability is checked (`checkValidationLayerSupport()`), and a `VK_EXT_debug_utils`
messenger prints `Validation layer: <message>` (warnings and errors to stderr, the rest to stdout) in
`src/renderer_core.cpp`.

**Gap.** Met. Notes: the message is the layer's own text with a prefix, there is no further translation. One
message appears at every Debug start (`robustBufferAccess2` without `robustBufferAccess`, see `docs/BUILD.md`); the
second one, the `Int64` capability in `ray_query.spv`, went away with that shader on 2026-09-20 (confirmed by the
owner's Debug run of 2026-09-21). A clean start-up would help students tell their own errors apart. Validation is a compile-time
switch, not a runtime option.

### `keine_audio_physik` — No audio and physics components (Won't)

Thesis refs: `sec:systemgrenzen_abgrenzung`.

**Requirement.** The engine has no audio and no physics components.

**Code today.** Physics (`physics_system.*`, ball demo), audio (`audio_system.*`, HRTF compute) and every reference
to them are deleted (`docs/DELETIONS.md`).

**Gap.** None.

## Summary of the gaps

Every "hide it" requirement (`ki:*`) is covered by the tutorial engine as ported. Every "expose it to the student"
requirement (`ns:*`, `fa:*`) is open: the tutorial is a demo application whose renderer does the whole frame itself,
so there was never a student-facing frame or pipeline API to keep. Those two (`lifecycle`, `einfache_pipeline`) need
new code and therefore owner approval under Rule 1, and they are the ones a student would touch first; their planned
shape is the student-facing interface in `docs/ROADMAP.md`. Smaller items: raster shaders that use the scene lights,
a `LightComponent`, PNG textures, the remaining start-up validation message, the optional ASCII view.
