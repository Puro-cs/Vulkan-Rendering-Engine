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
On 2026-10-03 the thesis was brought in line with these amendments (thesis `docs/THESIS_VS_ENGINE.md`): its TR11
`shader_verwaltung` row is deleted (the thesis items `fa:2_shader` and `ns:4_material_shader` were kept and reworded:
template shader plus the fixed shader inputs, parameters as constants), and the owner extended two requirements
with no code change: `architektur_techstack` now also names vcpkg, libktx and Dear ImGui (thesis WB 8-10), and
`keine_audio_physik` now lists every component the port removed as out of scope (animation, ray query, planar
reflections, Forward+, non-Windows platforms), not only audio and physics; and a new Must requirement
`initialisierungskette` (the eight calls as the students make them, order checked) was split out of
`basis_infrastruktur`, which is now about hiding only. The other thesis TR keys are unchanged. The
thesis NS labels were renumbered the same day because a new first item `ns:1_initialisierungskette` (the
initialization chain as the students call it) was inserted: `ns:1_CB_rendering` → `ns:2_render_lebenszyklus`,
`ns:2_pipeline_konfiguration` → `ns:3_pipeline_konfiguration`, `ns:3_geometrie_assets` → `ns:4_geometrie_assets`,
`ns:4_material_shader` → `ns:5_material_shader`, `ns:5_kamera_licht` → `ns:6_kamera_licht`; the "Thesis refs"
lines below use the new keys.
Every gap that needs code the tutorial does not have falls under Rule 1 of `docs/CODE_CHANGE_RULES.md` (proposal,
owner approval); none of them can be closed by deletion alone.

## Overview

| ID | Priority | Requirement | Status |
|---|---|---|---|
| `architektur_techstack` | Must | Component architecture; C++20, Vulkan SDK, CMake, GLFW, GLM, tinygltf (glTF 2.0), Slang, vcpkg, libktx, Dear ImGui; Windows 11 | met |
| `basis_infrastruktur` | Must | Instance, swap chain, dynamic rendering (Vulkan 1.3), colour and depth attachments preconfigured and hidden | met |
| `synchronisation_speicherverwaltung` | Must | Frames in flight, command buffers, sync objects, GPU memory hidden | met |
| `pipeline_shader_automatisierung` | Must | Abstract pipeline configurations translated to Vulkan; shaders compiled to `VkShaderModule` | met since planned change 4 (2026-09-21) |
| `descriptor` | Must | Descriptor pools, set layouts, UBO mapping hidden | met |
| `input_handling` | Must | GLFW input captured and turned into scene navigation internally | met |
| `texture_pipeline` | Must | Whole texture life cycle hidden | met (KTX2 only) |
| `initialisierungskette` | Must | Students set the engine up through a fixed sequence of simplified calls, each a group of Vulkan steps; completeness and order checked, violations reported by name | met since planned change 7 (2026-09-22); added to the thesis on 2026-10-03 |
| `lifecycle` | Must | Students drive the frame life cycle through simplified calls in a fixed order | met since planned change 8 (2026-09-22; the owner's run confirmed it the same day) |
| `einfache_pipeline` | Must | Shader assignment and pipeline state through simplified abstractions | met since planned changes 4 and 5 (2026-09-21): `Sandbox::CreatePipeline()` / `AddToPipeline()` |
| `tr:szenenmanagement` | Must | Load and transform models, cameras, lights without manual buffers; auto-translate to UBO / push constants | met since planned changes 3 and 5 (2026-09-21; the owner's run of planned change 5 confirmed it on 2026-09-22). Since planned change 6 (2026-09-22) objects and lights can also be moved, rotated and scaled while rendering, through terminal commands |
| `ascii_pipeline` | Could | ASCII rendering of the pipeline in the terminal | met since planned change 9 (2026-09-22; the owner's run confirmed it the same day) |
| `validation_layer` | Must | Validation layers wired in, messages readable | met |
| `keine_audio_physik` | Won't | No audio, no physics, no animation, no ray query, no planar reflections, no Forward+, no platform other than Windows | met |

## Requirements in detail

### `architektur_techstack` — Architecture and technology stack (Must)

Thesis refs: `ap:component`, WB 1-10, `app:bestandsaufnahme_pc-pool`.

**Requirement.** Component-based architecture. Mandatory technologies: C++20, the current Vulkan SDK, CMake, GLFW, GLM,
tinygltf (glTF 2.0 as model format), Slang as shader language, the package manager vcpkg (manifest mode), libktx
(KTX2 textures) and Dear ImGui (the engine's panel). Development must be possible on Windows 11.
(Amended by the owner on 2026-09-21: the original entry named tinyobjloader. Amended again on 2026-10-03: vcpkg,
libktx and Dear ImGui added as thesis WB 8-10; they were already in use, see "Code today" and `docs/BUILD.md`.)

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
preconfigured statically and managed in the background. (Since 2026-10-03 the student-made calls that trigger these
steps are their own requirement, `initialisierungskette`.) (Amended by the owner on 2026-09-21: the original entry named
render pass setup and frame buffers, i.e. render pass objects. The thesis label `ki:2_render pass` is unchanged.)

**Code today.** The initialization chain of `Renderer` (`src/renderer_core.cpp`; until planned change 7 one
`Renderer::Initialize()`) creates instance, debug messenger, surface, device,
swap chain, image views, depth image, command pools and sync objects; a resize recreates the swap chain
(`src/renderer_rendering.cpp`). Since planned change 7 (2026-09-22) `sandbox.cpp` makes the eight calls of the chain
(`InitializeWindow`, `CreateInstance`, `PickDevice`, `CreateSwapChain`, `InitializeRendering`, `CreatePipelines`,
`CreateCommandBuffers`, `CreateSyncObjects`); each is one group of Vulkan steps whose details stay in the engine, and
a missing or misplaced call is reported by name. Dynamic rendering is mandatory: device
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

**Status since 2026-09-21 (planned change 4).** Met at the renderer level: `PipelineSettings`
(`src/pipeline_settings.h`: cull mode, depth test, blending) plus a shader file is the abstracted configuration, and
`Renderer::CreatePipeline(name, shaderFile, settings)` turns it into a `vk::raii::Pipeline` (shader module from
`shaders/<name>.spv`, everything else copied from the opaque PBR pipeline). The engine's own pipelines stay
hard-coded. The owner ran a test pipeline from `template.slang` successfully on 2026-09-21. The paragraph below
describes the state before.

**Gap (before planned change 4).** The "abstracted pipeline configuration" does not exist. Each pipeline (textured mesh, the PBR variants
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
camera every frame and yields to ImGui when the pointer is over a panel (`src/engine.cpp`). `sandbox.cpp` contains no
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

### `initialisierungskette` — Control of the initialization (Must)

Thesis refs: `ns:1_initialisierungskette`. Added by the owner on 2026-10-03 as the counterpart of `lifecycle` for the
start-up: until then the chain was a sentence inside `basis_infrastruktur`, which is now about hiding only.

**Requirement.** Students set the engine up through a fixed sequence of simplified initialization calls, each of
which bundles a group of Vulkan steps of the core infrastructure. The engine checks that the sequence is complete
and in order and reports a missing, repeated or misplaced call by name.

**Code today.** Met since planned change 7 (2026-09-22): the eight calls `InitializeWindow`, `CreateInstance`,
`PickDevice`, `CreateSwapChain`, `InitializeRendering`, `CreatePipelines`, `CreateCommandBuffers`,
`CreateSyncObjects` in `src/sandbox.cpp`, the order check in `src/sandbox_impl.cpp` (`Initialization error:
CreateSwapChain() was called, but PickDevice() has not been done.`), the chain view of planned change 9 on the first
error (`docs/ARCHITECTURE.md`, "The initialization chain", "Terminal output").

**Gap.** None.

### `lifecycle` — Control of the render life cycle (Must)

Thesis refs: `ns:2_render_lebenszyklus` (until 2026-10-03 `ns:1_CB_rendering`).

**Requirement.** Students control the rendering flow explicitly: they define the frame life cycle through simplified
calls in a fixed order (begin the frame, update the scene, begin rendering, draw the scene, end rendering, end the
frame). Recording the individual draw commands into the command buffer stays inside the engine. (Amended by the owner
on 2026-09-21: in the original entry the students bundled the draw commands in a command buffer themselves. The
thesis label was `ns:1_CB_rendering` until 2026-10-03.)

**Status since 2026-09-22 (planned change 8).** Met; the owner's run confirmed it on 2026-09-22. `main()` in `sandbox.cpp`
renders with `while (sandbox.IsRunning()) { BeginFrame(); UpdateScene(); BeginRendering(); DrawScene();
EndRendering(); EndFrame(); }`: the six calls are the six parts of the tutorial's `Renderer::Render()` (every line
kept, `docs/ARCHITECTURE.md`, "One frame"), `UpdateScene()` preceded by the tutorial's `Engine::Update()`. The
sandbox layer checks the order: a call out of order prints `Frame sequence error: <call>() was called, <expected>()
was expected. Rendering stopped.` and the loop ends. Acquire, the recording of the draw commands (one `DrawScene()`
for the whole scene, as amended), submit and synchronisation stay inside `Renderer`. The paragraphs below describe
the state before.

**Code before planned change 8.** `Engine::Run()` owned the loop; `Engine::Render()` was private and handed an
entity snapshot to `Renderer::Render()` (`src/renderer_rendering.cpp`, about 870 lines), which acquired the image,
uploaded lights, culled, recorded every pass and ImGui, submitted and presented. `sandbox.cpp` called the eight
initialization calls (planned change 7), `SetupScene` and `Run`, and nothing else. `GetCurrentCommandBuffer()`
returns the raw `vk::raii::CommandBuffer`, the native object, not a simplified call (unchanged).

**Gap (before planned change 8).** Not met. Needed: the frame API of the student-facing interface in `docs/ROADMAP.md`: six calls with an order
check (`BeginFrame`, `UpdateScene`, `BeginRendering`, `DrawScene`, `EndRendering`, `EndFrame`) that the student calls
from an own loop, while acquire, the recording of the draw commands, submit and synchronisation stay inside
`Renderer`. The tutorial has no such API (Rule 1).

### `einfache_pipeline` — Explicit but simplified pipeline configuration (Must)

Thesis refs: `ns:3_pipeline_konfiguration` (until 2026-10-03 `ns:2_pipeline_konfiguration`).

**Requirement.** Students assign shaders and define pipeline state through simplified abstractions, without writing
native Vulkan structures.

**Code today.** Pipelines are chosen internally, and the user has no choice left: the basic / PBR toggle of the
ImGui "Renderer" panel was removed on 2026-09-21 (planned change 2; `RenderMode`, Forward+ and planar reflections
went on 2026-09-20 / 2026-09-21). Every opaque object uses the PBR pipeline; transparent and glass materials pick
their pipeline from the glTF material. The shader name is a literal (`"shaders/pbr.spv"`) in
`src/renderer_pipelines.cpp`. Using an own shader means editing `createPBRPipeline()` and its
`vk::GraphicsPipelineCreateInfo`.

**Status since 2026-09-21 (planned change 4).** Met at the renderer level: `CreatePipeline(name, shaderFile,
settings)` takes a shader file and three settings without any Vulkan structure, and `AddToPipeline(name, entity)`
attaches entities (none: `"pbr"`; several: one draw per pipeline in creation order; blending: transparent pass).
Students reach both through `Renderer` today; the wrapper without Vulkan headers follows with planned change 5. The
owner ran a test pipeline successfully on 2026-09-21. The paragraph below describes the state before.

**Gap (before planned change 4).** Not met. Needed: a pipeline description (vertex and fragment shader, topology,
cull mode, depth test, blending) that the engine turns into a pipeline, and a way to attach it to an entity or
material. Rule 1.

### `tr:szenenmanagement` — Scene management and data handling (Must)

Thesis refs: `fa:1_szenenmanagement`, `ns:4_geometrie_assets`, `ns:6_kamera_licht` (until 2026-10-03 `ns:3_…`, `ns:5_…`).

**Requirement.** Load and transform 3D models, cameras and lights without allocating vertex or index buffers by hand;
the engine translates the scene data into UBOs / push constants and provides them to the shaders.

**Code today.** `LoadGLTFModel()` (`src/scene_loading.cpp`) creates one entity per material mesh with
`TransformComponent` and `MeshComponent` (CPU vertices, indices, instances); the renderer allocates vertex, index and
instance buffers at the frame safe point, fills the per-entity UBO (`UniformBufferObject`: model / view / proj /
camPos ...) and the `MaterialProperties` push constants every frame. Camera: `CameraComponent` on an entity,
optionally overwritten by the first glTF camera. Lights: `KHR_lights_punctual` lights and emissive materials become
`ExtractedLight` entries in a storage buffer; `Renderer::SetStaticLights()` also accepts a hand-built vector.

**Status since 2026-09-21 (planned change 5).** Met; the owner's run confirmed it on 2026-09-22. Students load and transform models,
cameras and lights through `sandbox.h` without touching a buffer: `LoadModel()` returns a `SceneObject` whose
`SetPosition` / `SetRotation` / `SetScale` / `Move` / `Rotate` / `Scale` reach every part, `CreateSphere()` builds a
mesh without a file and queues its GPU upload, `CreateCamera()` / `CreateLight()` return handles with their own
setters. Since planned change 6 (2026-09-22) the transforms can also be changed while the engine renders, from the
terminal: `Room.Move(1, 0, 0)`, `Sun.Rotate(0, 30, 0)`, `Sphere.Scale(2, 2, 2)` (`docs/ARCHITECTURE.md`, "Terminal
commands"). The three gaps below are closed; they are kept as the record of the state before.

**Gap (before planned changes 3 and 5).** Partly met.

- Closed on 2026-09-21 by planned change 3: `LightComponent` (`src/light_component.h/.cpp`) makes an entity a light;
  position and direction come from its `TransformComponent`, and `Renderer::Render()` appends the lights of all
  entities to the glTF lights every frame. The scene set-up (`sandbox.cpp` since planned change 5) creates the light "Sun" this way. Before that, lights were
  not components: there was only the renderer-level vector.
- Closed on 2026-09-21 by planned change 3 (shader part): `pbr.slang` is now the tutorial's full PBR shader, trimmed,
  and its light loop reads the light buffer. The rest of this item describes the state before, when `pbr.slang`
  ignored the light buffer and used a fixed direction (1, 1, 1) plus ambient (the second shader,
  `texturedMesh.slang`, did the same and was removed on 2026-09-21). The engine side is in place: the scene lights are uploaded every frame to a storage buffer bound at
  PBR set 0, binding 6, with the count in `ubo.lightCount`. But no shader reads it. `ray_query.slang` was the one
  shader that shaded with the lights (removed 2026-09-20) and Forward+ only culled them (removed 2026-09-21), so
  "light data provided to the shaders" holds for no shading pass. Closing this needs a raster shader that declares
  binding 6 and loops over the lights (Rule 1); it is also an obvious first exercise for students.
- Closed on 2026-09-21 by planned change 5: `Sandbox::CreateSphere()` queues the upload itself. Before, meshes built
  in code (`MeshComponent::CreateSphere()`, `SetVertices()`) got GPU buffers only if the caller also invoked
  `Renderer::EnqueueEntityPreallocationBatch()`; entities without resources are skipped by `Render()`.

### `ascii_pipeline` — ASCII pipeline visualisation (Could)

Thesis refs: `vp:pipeline`.

**Requirement.** At run time the Vulkan render pipeline is drawn as ASCII art in the terminal to support learning.

**Status since 2026-09-22 (planned change 9).** Met; the owner's run confirmed it on 2026-09-22. The first `IsRunning()` prints, once,
the initialization chain (one line per call, `[ok]`, with what the call does), every pipeline as a block of its seven
stages from the input assembly to the attachments (the four settings of an own pipeline marked `yours`, the rest
`fixed`; the objects that are drawn with it) and the six frame calls; an initialization error or a frame sequence
error prints the same list on stderr with the failed call marked (`[!!] PickDevice            <- missing`). It is
text with fixed columns rather than a drawing of the GPU pipeline, in the form the owner chose in `docs/ROADMAP.md`,
"Terminal output". The paragraphs below describe the state before.

**Code before planned change 9.** Nothing; no pipeline introspection or printing existed, neither in the port nor
upstream.

**Gap (before planned change 9).** Not met (optional). Rule 1.

### `validation_layer` — Error handling with validation layers (Must)

Thesis refs: `vl:validation`.

**Requirement.** The Vulkan validation layers are wired in so that wrong commands are caught and reported as
understandable messages.

**Code today.** `VK_LAYER_KHRONOS_validation` is requested when `ENABLE_VALIDATION_LAYERS` is set (`sandbox_impl.cpp`: true
in Debug, false in Release), availability is checked (`checkValidationLayerSupport()`), and a `VK_EXT_debug_utils`
messenger prints `Validation layer: <message>` (warnings and errors to stderr, the rest to stdout) in
`src/renderer_core.cpp`.

**Gap.** Met. Notes: the message is the layer's own text with a prefix, there is no further translation. One
message appears at every Debug start (`robustBufferAccess2` without `robustBufferAccess`, see `docs/BUILD.md`); the
second one, the `Int64` capability in `ray_query.spv`, went away with that shader on 2026-09-20 (confirmed by the
owner's Debug run of 2026-09-21). A clean start-up would help students tell their own errors apart. Validation is a compile-time
switch, not a runtime option.

### `keine_audio_physik` — No components outside the teaching scope (Won't)

Thesis refs: `sec:systemgrenzen_abgrenzung`, `ak:4_fokus_lokal`.

**Requirement.** The engine has no audio components, no physics components, no animation, none of the template's
advanced rendering techniques (ray tracing via ray query, planar reflections, the Forward+ light-culling
optimisation) and no platform other than Windows. (Amended by the owner on 2026-10-03: the original entry named audio and physics only; the thesis
now lists every removed component under its section 6.1 and this requirement, with the justification that they lie
outside the seminar learning goals and the focus on local lighting models. The key is unchanged.)

**Code today.** Physics (`physics_system.*`, ball demo), audio (`audio_system.*`, HRTF compute), glTF animation, the
ray-query render mode with the acceleration structures and the raster shadow option, planar reflections, Forward+
with its depth pre-pass, and the Android / direct-to-display / Linux / macOS paths and every reference to them are
deleted (`docs/ARCHITECTURE.md`, intro; `docs/DELETIONS.md`).

**Gap.** None.

## Summary of the gaps

Every "hide it" requirement (`ki:*`) is covered by the tutorial engine as ported. Every "expose it to the student"
requirement (`ns:*`, `fa:*`) is open: the tutorial is a demo application whose renderer does the whole frame itself,
so there was never a student-facing frame or pipeline API to keep. Those two (`lifecycle`, `einfache_pipeline`) need
new code and therefore owner approval under Rule 1, and they are the ones a student would touch first; their planned
shape is the student-facing interface in `docs/ROADMAP.md`. Smaller items: raster shaders that use the scene lights,
a `LightComponent`, PNG textures, the remaining start-up validation message, the optional ASCII view.
