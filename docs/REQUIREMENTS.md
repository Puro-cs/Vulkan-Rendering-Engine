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
Every gap that needs code the tutorial does not have falls under Rule 1 of `docs/CODE_CHANGE_RULES.md` (proposal,
owner approval); none of them can be closed by deletion alone.

## Overview

| ID | Priority | Requirement | Status |
|---|---|---|---|
| `architektur_techstack` | Must | Component architecture; C++20, Vulkan SDK, CMake, GLFW, GLM, tinyobjloader, Slang; Windows 11 | partly (tinyobjloader missing) |
| `basis_infrastruktur` | Must | Instance, swap chain, render pass, frame and depth buffers preconfigured and hidden | met (dynamic rendering, see note) |
| `synchronisation_speicherverwaltung` | Must | Frames in flight, command buffers, sync objects, GPU memory hidden | met |
| `pipeline_shader_automatisierung` | Must | Abstract pipeline configurations translated to Vulkan; shaders compiled to `VkShaderModule` | partly (no configurable pipelines) |
| `descriptor` | Must | Descriptor pools, set layouts, UBO mapping hidden | met |
| `input_handling` | Must | GLFW input captured and turned into scene navigation internally | met |
| `texture_pipeline` | Must | Whole texture life cycle hidden | met (KTX2 only) |
| `lifecycle` | Must | Students bundle draw commands and drive the frame through simplified calls | not met |
| `einfache_pipeline` | Must | Shader assignment and pipeline state through simplified abstractions | not met |
| `tr:szenenmanagement` | Must | Load and transform models, cameras, lights without manual buffers; auto-translate to UBO / push constants | partly |
| `shader_verwaltung` | Must | Generic shader integration; own C++ structs passed through set methods | not met |
| `ascii_pipeline` | Could | ASCII rendering of the pipeline in the terminal | not met |
| `validation_layer` | Must | Validation layers wired in, messages readable | met |
| `keine_audio_physik` | Won't | No audio, no physics | met |

## Requirements in detail

### `architektur_techstack` — Architecture and technology stack (Must)

Thesis refs: `ap:component`, WB 1-7, `app:bestandsaufnahme_pc-pool`.

**Requirement.** Component-based architecture. Mandatory technologies: C++20, the current Vulkan SDK, CMake, GLFW, GLM,
tinyobjloader and Slang as shader language. Development must be possible on Windows 11.

**Code today.** `Entity` + `Component` with `TransformComponent`, `CameraComponent`, `MeshComponent` (`src/entity.h`,
`src/component.h`). `CXX_STANDARD 20` in `CMakeLists.txt`; Vulkan SDK 1.4 through `VULKAN_SDK`; CMake presets for
Visual Studio 2026; GLFW in `DesktopPlatform`; GLM throughout; every shader in `src/shaders/` is Slang and compiled by
`slangc`. Builds and runs on Windows 11 (`docs/BUILD.md`).

**Gap.** tinyobjloader is not used anywhere. Models are parsed with tinygltf (`src/model_loader.cpp`, glTF 2.0 only)
and `vcpkg.json` does not list tinyobjloader. The upstream tutorial only ships a find module for it and no OBJ loader
code, so an OBJ path would be new code (Rule 1). Alternative: amend the thesis requirement to tinygltf.

### `basis_infrastruktur` — Encapsulated base infrastructure (Must)

Thesis refs: `ki:1_swapchain`, `ki:2_render pass`.

**Requirement.** Instance creation, swap chain management, render pass setup and the frame and depth buffers are
preconfigured statically and managed in the background.

**Code today.** `Renderer::Initialize()` (`src/renderer_core.cpp`) creates instance, debug messenger, surface, device,
swap chain, image views, depth image, command pools and sync objects; a resize recreates the swap chain
(`src/renderer_rendering.cpp`). `main.cpp` only calls `Engine::Initialize()`.

**Gap.** None in code. Note for the thesis: the engine uses Vulkan 1.3 dynamic rendering. There are no `VkRenderPass`
or `VkFramebuffer` objects; the colour attachment (swap-chain image) and the depth attachment are bound per pass through
`vk::RenderingInfo`. The requirement text should say "render passes (dynamic rendering) and attachments" or explain
the difference.

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
descriptor layout and fixed-function state. The unused `Pipeline` class (`src/pipeline.h`) hard-codes the same
pipelines and is no abstraction either. Slang compilation is a build step, not something the engine does at run time;
acceptable, but the thesis should say so.

### `descriptor` — Hidden descriptor management (Must)

Thesis refs: `ki:6_descriptor_management`.

**Requirement.** Descriptor pools, descriptor set layouts and the memory mapping of UBOs that inject CPU data into the
shaders are handled in the background.

**Code today.** The descriptor pool, the basic and PBR set layouts, per-entity per-frame uniform
buffers with persistent mapping, deferred descriptor updates (`MarkEntityDescriptorsDirty()`,
`ProcessDirtyDescriptorsForFrame()`) and refreshes when textures stream in all live in `src/renderer_pipelines.cpp`
and `src/renderer_resources.cpp`. The standalone `DescriptorManager` is unused.

**Gap.** None for the hiding. Feeding student data into those UBOs is `shader_verwaltung`.

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

**Requirement.** Students control the rendering flow explicitly: they bundle draw commands in a command buffer and
define the frame life cycle through simplified calls.

**Code today.** `Engine::Run()` owns the loop; `Engine::Render()` is private and hands an entity snapshot to
`Renderer::Render()` (`src/renderer_rendering.cpp`, roughly lines 681-1554), which acquires the image, uploads
lights, culls, records every pass and ImGui, submits and presents. `main.cpp` calls `Initialize`, `SetupScene`, `Run`
and nothing else. `GetCurrentCommandBuffer()` returns the raw `vk::raii::CommandBuffer`, the native object, not a
simplified call.

**Gap.** Not met. Needed: a small frame API (begin frame, draw an entity or mesh with a pipeline, end frame and
present) that the student calls from an own loop or a per-frame callback, while acquire, submit and synchronisation
stay inside `Renderer`. The tutorial has no such API (Rule 1).

### `einfache_pipeline` — Explicit but simplified pipeline configuration (Must)

Thesis refs: `ns:2_pipeline_konfiguration`.

**Requirement.** Students assign shaders and define pipeline state through simplified abstractions, without writing
native Vulkan structures.

**Code today.** Pipelines are chosen internally. The only choice left to the user is the basic / PBR toggle in the
ImGui "Renderer" panel (`RenderMode`, Forward+ and planar reflections were removed on 2026-09-20 / 2026-09-21);
transparent and glass materials pick their pipeline from the glTF material. Shader names are literals
(`"shaders/pbr.spv"`, `"shaders/texturedMesh.spv"`) in `src/renderer_pipelines.cpp`. Using an own shader means
editing `createGraphicsPipeline()` / `createPBRPipeline()` and their `vk::GraphicsPipelineCreateInfo`.

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
- The raster shaders `texturedMesh.slang` and `pbr.slang` ignore the light buffer and use a fixed direction (1, 1, 1)
  plus ambient. The engine side is in place: the scene lights are uploaded every frame to a storage buffer bound at
  PBR set 0, binding 6, with the count in `ubo.lightCount`. But no shader reads it. `ray_query.slang` was the one
  shader that shaded with the lights (removed 2026-09-20) and Forward+ only culled them (removed 2026-09-21), so
  "light data provided to the shaders" holds for no shading pass. Closing this needs a raster shader that declares
  binding 6 and loops over the lights (Rule 1); it is also an obvious first exercise for students.
- Meshes built in code (`MeshComponent::CreateSphere()`, `SetVertices()`) get GPU buffers only if the caller also
  invokes `Renderer::EnqueueEntityPreallocationBatch()`; entities without resources are skipped by `Render()`.
- Model format is glTF only (see `architektur_techstack`).

### `shader_verwaltung` — Generic shader and parameter management (Must)

Thesis refs: `fa:2_shader`, `ns:4_material_shader`.

**Requirement.** An abstract interface to integrate and manage shaders, flexible enough that own C++ structs (light or
material parameters) are handed over through intuitive set methods; the engine passes them to the GPU and hides the
Vulkan concepts underneath.

**Code today.** Fixed UBO layout (`UniformBufferObject` in `src/renderer.h`, mirrored in
`shaders/common_types.slang`), fixed `MaterialProperties` push constants, fixed descriptor set layouts. Public setters
exist only for built-in values: `SetGamma`, `SetExposure`, `SetStaticLights` (`SetReflectionIntensity` and
`SetRenderMode` were removed on 2026-09-21). Material values come from glTF and are read-only for the application
(`ModelLoader::GetMaterial()` returns `const Material*`).

**Gap.** Not met. Needed: a way to register a shader together with its parameter struct and to update that struct
through a set method (a `SetUniform<T>(...)` style call or a material object), backed by engine-managed UBO or
push-constant ranges and the matching Slang declaration. Rule 1.

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
so there was never a student-facing frame, pipeline or parameter API to keep. Those three (`lifecycle`,
`einfache_pipeline`, `shader_verwaltung`) need new code and therefore owner approval under Rule 1, and they are the
ones a student would touch first. Smaller items: tinyobjloader (or a thesis amendment), raster shaders that use the
scene lights, a `LightComponent`, PNG textures, the remaining start-up validation message, the optional ASCII view.
