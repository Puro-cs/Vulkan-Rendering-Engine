# Architecture

The port is the Vulkan-Tutorial "simple engine" (Holochip / Khronos, Apache-2.0) reduced to a Windows-only rendering
study system. Removed: physics, audio, Android / direct-to-display / Linux / macOS paths, glTF animation, the
opacity-micromap course, unused shaders, everything ray query (2026-09-20 the render mode, 2026-09-21 the acceleration
structures and the raster shadow option), Forward+ with its depth pre-pass, planar reflections, the dead
standalone `Pipeline` class (`pipeline.h/.cpp`), and the basic lighting path with `texturedMesh.slang`. What is left
is one forward rasterization path with one mesh shader, `pbr.slang` (since planned change 3 the tutorial's full PBR
shader, trimmed). Details in `docs/DELETIONS.md`; the file-by-file
comparison with the tutorial is `src/source-file-difference.md`.

Intended use (owner, 2026-09-21): students who have never worked with Vulkan write their own simple shaders and
learn the basics of Vulkan and computer graphics. That is why the renderer was cut down to the parts a first shader
touches.

Vulkan 1.3 through vulkan.hpp (RAII handles, dynamic dispatch), dynamic rendering and synchronization2 throughout,
no render passes or framebuffers. C++20, Slang shaders, Dear ImGui 1.92 vendored in `src/imgui/`.

## Ownership

```
main.cpp
  CrashReporter (singleton, minidumps)
  Engine
    Platform (DesktopPlatform: GLFW window, input callbacks, surface)
    Renderer (everything Vulkan; ~7.2k lines across renderer_*.cpp)
      MemoryPool        sub-allocates device memory for buffers/images
      ThreadPool        texture decode / upload workers
      SwapChain         swap chain + image views, recreated on resize
      VulkanDevice      physical device pick, logical device, queues
    ModelLoader (tinygltf parsing, materials, light and camera extraction; needs Renderer)
    ImGuiSystem (own Vulkan pipeline, font texture, input translation)
    ResourceManager (generic handle store; created but no subsystem uses it)
    entities: vector<unique_ptr<Entity>>, each Entity = list of Component
      TransformComponent   position / rotation / scale -> model matrix
      CameraComponent      view / projection, aspect ratio set by Engine on resize
      MeshComponent        CPU-side vertices, indices, instances, material id
      LightComponent       type, colour, intensity, range, cone angles; position and direction from the
                           entity's transform (shines along its -Z axis). Own code, added 2026-09-21
```

Scene loading is a free function pair in `scene_loading.h/.cpp`: `LoadGLTFModel(engine, path, pos, rotDeg, scale)`
runs on a **background thread** started by `SetupScene()` in `main.cpp`. It parses with ModelLoader, creates one
Entity per glTF mesh node (instanced when a mesh is reused), and hands GPU work to the Renderer through queues that
are drained on the render thread (`ProcessPendingMeshUploads`, `ProcessPendingEntityPreallocations`,
`ProcessDirtyDescriptorsForFrame`). All Vulkan submits happen on the main thread.

## Files

| File(s) | Responsibility | Used at run time |
|---|---|---|
| `main.cpp` | entry point, window size, validation toggle, `SetupScene()` | yes |
| `engine.h/.cpp` | main loop, delta time, FPS title, entity list with removal queue, camera fly controls, input routing to ImGui | yes |
| `platform.h/.cpp` | `Platform` interface + `DesktopPlatform` (GLFW) | yes |
| `renderer.h` | the whole `Renderer` class declaration, UBO / push-constant structs, `LoadingPhase` | yes |
| `renderer_core.cpp` | instance, debug messenger, device and feature selection, swap chain, sync objects, command pools | yes |
| `renderer_pipelines.cpp` | descriptor set layouts and graphics pipelines: PBR (opaque, blended, glass; a premultiplied-alpha variant is declared in `renderer.h` but never created, as in the tutorial) and composite. Since planned change 4 also the named pipelines: `createNamedPipeline()`, `CreatePipeline()`, `AddToPipeline()` (own code) | yes |
| `pipeline_settings.h` | `CullMode`, `PipelineSettings` (cull mode, depth test, blending): the description of a named pipeline, without Vulkan types (own code, planned change 4) | yes |
| `renderer_resources.cpp` | buffers, images, textures (KTX2 via libktx), mipmaps, per-entity resources, streaming queues | yes |
| `renderer_rendering.cpp` | `Renderer::Render()` frame function (see below), light extraction, culling, the "Renderer" ImGui panel | yes |
| `renderer_utils.cpp` | shader module loading, memory type lookup, layout transitions, copy helpers | yes |
| `vulkan_device.h/.cpp` | `VulkanDevice` helper | yes |
| `swap_chain.h` | `SwapChain` helper | yes |
| `memory_pool.h/.cpp` | `MemoryPool` allocator | yes |
| `thread_pool.h` | `ThreadPool` | yes |
| `model_loader.h/.cpp` | glTF parsing, `Material`, `Model`, `ExtractedLight`, `CameraData`, mikktspace tangents | yes |
| `mikktspace.h/.c` | Morten Mikkelsen tangent generation (C) | yes |
| `scene_loading.h/.cpp` | `LoadGLTFModel()` | yes |
| `entity.*`, `component.*`, `transform_component.*`, `camera_component.*`, `mesh_component.*` | entity-component model | yes |
| `light_component.h/.cpp` | `LightComponent` (not in the tutorial; planned change 3). `GetLight()` returns an `ExtractedLight` in world space; `Renderer::Render()` appends the lights of all entities to the glTF lights every frame | yes |
| `imgui_system.h/.cpp` | Dear ImGui integration, loading overlay, texture-streaming status window | yes |
| `crash_reporter.h` | minidump writer (Dbghelp) | yes |
| `debug_system.h` | `DebugSystem` base, `LogLevel`, `LOGI` macros | yes (macros) |
| `resource_manager.h/.cpp` | `ResourceManager`, `ResourceHandle` | compiled, owned by Engine, never called |
| `descriptor_manager.h/.cpp` | standalone `DescriptorManager` | compiled, not referenced |
| `renderdoc_debug_system.h/.cpp` | RenderDoc capture hooks | compiled, not referenced |
| `vulkan_compatibility.h`, `vulkan_dispatch.cpp` | SDK version shims, `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` | yes |

The three "compiled, not referenced" rows are dead code kept from the tutorial. Removing them is a Rule 2 deletion.
A fourth one, the standalone `Pipeline` class (`pipeline.h/.cpp`), was deleted on 2026-09-21 (planned change 1).

## Shaders (`src/shaders/`)

| Shader | Stage | Used by |
|---|---|---|
| `pbr.slang` | vert + frag | every mesh. Since 2026-09-21 (planned change 3) it is the tutorial's `pbr_full.slang`, trimmed by deletion: material sampling (metallic-roughness or spec-gloss, normal map, occlusion, emissive, alpha-mask `discard`), a loop over all scene lights in the storage buffer (directional, point, spot, emissive; distance falloff, spot cone, GGX specular plus diffuse), ambient = 10 % of the surface colour (`0.1 * ubo.scaleIBLAmbient`, which C++ sets to 1.0), emissive. Output is linear; `composite` tone-maps. `GlassPSMain` shows the off-screen scene colour through glass (tint, rim, emissive surface term) and tone-maps itself, because glass is drawn after the composite pass. Without a light the scene shows only ambient and emissive |
| `composite.slang` | vert + frag | fullscreen pass that draws the off-screen opaque colour to the swap chain (exposure, filmic tone map; gamma only on a non-sRGB swap chain) before the transparent pass |
| `template.slang` | vert + frag | nothing yet: the blank starting point for student shaders (transform plus UV in `VSMain`, the unlit base colour texture sample in `PSMain`). Cut out of the tutorial's `texturedMesh.slang`; compiled to `template.spv`, first usable with the named pipelines of planned change 4 |
| `imgui.slang` | vert + frag | ImGuiSystem |
| `common_types`, `pbr_utils`, `lighting_utils`, `tonemapping_utils` | modules | imported by the above, not compiled standalone |

The second mesh shader, `texturedMesh.slang` (the "basic" path behind the panel option "Use Basic Lighting (Phong)"),
was deleted on 2026-09-21 with its pipeline, descriptor set layout and per-entity descriptor sets (planned change 2).

## Named pipelines (planned change 4, own code)

`Renderer::CreatePipeline(name, shaderFile, settings)` builds a pipeline for a student shader. The student chooses
four things: the shader file (`"shaders/x.slang"`, loaded as `"shaders/x.spv"`, entry points `VSMain` and `PSMain`),
the cull mode (default none), the depth test (default on; compare `LessOrEqual`) and blending (default off; on means
the blend factors of the engine's blended pipeline, no depth writes, transparent pass). Everything else is copied
from the opaque PBR pipeline, including the PBR pipeline layout, so every named pipeline receives exactly the inputs
of the next section. The descriptions are kept in creation order and `recreateSwapChain()` rebuilds the pipelines
from them. Call `CreatePipeline()` after `Initialize()` and before rendering; the name `"pbr"` is reserved.

`Renderer::AddToPipeline(name, entity)` records, in the renderer, which pipelines an entity was added to (an unknown
name is an error that lists the known names). An entity that was added to no pipeline is drawn as before, with the
engine's PBR pipelines (`"pbr"`: opaque, blended or glass, chosen from the material). An entity in several pipelines
gets one render job per pipeline, in creation order with `"pbr"` first. It may be called from the loading thread.

## What a mesh shader gets from the engine

Since 2026-09-21 these are the complete inputs, which is what a student has to know to write a shader. There is one
mesh layout, the PBR one; the pipelines of planned change 4 will use it too:

| Input | Content |
|---|---|
| Vertex attributes | locations 0 to 3: position, normal, UV, tangent; 4 to 7: per-instance model matrix; 8 to 10: per-instance normal matrix |
| Set 0, binding 0 | `UniformBufferObject` (model, view, proj, camPos, exposure, lightCount, screenDimensions, ...) |
| Set 0, binding 1 | base colour texture |
| Set 0, bindings 2 to 5 | metallic-roughness, normal, occlusion, emissive textures |
| Set 0, binding 6 | storage buffer of `LightData` (scene lights, count in `ubo.lightCount`), fragment stage. Read by the light loop of `pbr.slang`. Convention: for a directional light (`lightType == 1`) `position.xyz` holds the direction in which the light travels, so the shader uses `L = normalize(-light.position.xyz)`; `color.rgb` is colour times intensity |
| Set 1, binding 0 | off-screen opaque scene colour (used by `GlassPSMain`) |
| Push constants | `PushConstants` (material factors), fragment stage |

`UniformBufferObject` still carries fields of removed features (`padding1`, `padding2`, `slicesZ`, reflection and
ray-query fields). They are never written, and since planned change 3 no shader reads them. They were kept on
2026-09-21 so that the compiled shaders stayed byte-identical to the tutorial's; that still holds for `composite.spv`
and `imgui.spv`, no longer for `pbr.spv`; see `docs/DELETIONS.md`. The fields `pbr.slang` reads are `model`, `view`,
`proj`, `camPos`, `lightCount`, `scaleIBLAmbient`, `screenDimensions`, `exposure`, `gamma` and `padding0` (1 on an
sRGB swap chain).

## One frame (`Renderer::Render`, `renderer_rendering.cpp`)

1. `Engine::Run`: `platform->ProcessEvents()`, delta time, `Update()` (camera controls; entity updates skipped while loading), `Render()` with a snapshot of entity pointers.
2. Build the frame light list from `staticLights` (the glTF lights) plus the lights of all active entities with a `LightComponent`, upload to the light storage buffer; fill the UBO template from the camera.
3. Wait on this frame slot's fence, reset it. Safe point: drain pending mesh uploads and entity preallocations.
4. Apply dirty descriptor writes for this frame index.
5. Preparation pass: collect active entities with GPU resources, per-frame descriptor cold-init, frustum culling, distance LOD; sort into opaque and transparent jobs. An entity that was added to named pipelines gets one job per pipeline (a pipeline with blending: transparent list).
6. `acquireNextImage`; out-of-date / suboptimal recreates the swap chain and returns.
7. Grow the light storage buffer if needed, begin the command buffer, process pending texture uploads, draw the "Renderer" panel.
8. Pass 1: clear colour and depth, draw the opaque jobs into the off-screen colour image, each with the pipeline of its job (`pbr` unless the entity was added to a named pipeline).
9. Pass 1b: `composite` draws that image to the swap chain (exposure, tone map).
10. Pass 2: draw the transparent jobs, sorted back to front (stable, so the jobs of one entity keep their pipeline order), onto the swap chain with the blended PBR pipeline, the glass pipeline or the job's named pipeline; glass samples the off-screen colour.
11. `imguiSystem->Render()` in its own dynamic-rendering pass on top.
12. `submit2` with the frame fence, `presentKHR`; recreate the swap chain on out-of-date. Advance `currentFrame` (MAX_FRAMES_IN_FLIGHT slots).

There is no depth pre-pass and no compute work any more; every opaque PBR draw uses `pbrGraphicsPipeline` (depth test
`Less`, depth writes on).

A watchdog thread checks `Render()` progress labels and aborts with the last label if no frame finishes for 10 s.
`watchdogSuppressed` still exists, but only acceleration-structure builds ever set it, so it is always false now.

## Build files

One `CMakeLists.txt`: dependencies through vcpkg CMake configs, `PLATFORM_DESKTOP` define, Slang compile rules, MSVC flags
(`/W3 /MP /bigobj /permissive-`), tinygltf configured without stb image (images go through libktx). CPack section is
unused. `CMakePresets.json` has one configure preset and Debug / Release build presets.
