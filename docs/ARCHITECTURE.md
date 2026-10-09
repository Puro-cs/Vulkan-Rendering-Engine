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
sandbox.cpp (the students' file: SetupScene() and main(); includes only sandbox.h)
  Sandbox (sandbox.h / sandbox_impl.cpp, own code: the engine as students see it, no Vulkan types)
    SceneObject, Camera, Light   handles over entities, owned by the Sandbox
    TerminalCommands (terminal_commands.h/.cpp, own code: reader thread on std::cin, queue, parser)
  CrashReporter (singleton, minidumps; started by the Sandbox constructor)
  Engine (owned by the Sandbox)
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
      CameraComponent      view / projection, aspect ratio set by Sandbox::SetActiveCamera() and by Engine on resize
      MeshComponent        CPU-side vertices, indices, instances, material id
      LightComponent       type, colour, intensity, range, cone angles; position and direction from the
                           entity's transform (shines along its -Z axis). Own code, added 2026-09-21
      TerminalCommandComponent  on the entity "TerminalCommands" only: its Update() applies the typed
                           terminal commands once per frame (sandbox_impl.cpp, own code, added 2026-09-22)
```

Scene loading is a free function pair in `scene_loading.h/.cpp`: `LoadGLTFModel(engine, path, pos, rotDeg, scale)`.
Since planned change 5 `Sandbox::LoadModel()` calls it **directly on the main thread**, before the render loop
starts (the tutorial's `main.cpp` ran it on a background thread); the call returns when the entities exist. It parses
with ModelLoader, creates one Entity per glTF mesh node (instanced when a mesh is reused), and hands GPU work to the
Renderer through queues that are drained on the render thread during the first frames, behind the loading overlay
(`ProcessPendingMeshUploads`, `ProcessPendingEntityPreallocations`, `ProcessDirtyDescriptorsForFrame`). Texture
decoding still runs on the thread pool. All Vulkan submits happen on the main thread. Known limit: the engine's
watchdog aborts when no frame finishes for 10 s, so a model that takes longer than that to parse trips it; planned
change 8 can render progress frames while a model loads.

## The sandbox layer (planned change 5, own code)

`sandbox.h` is the only engine header that `sandbox.cpp` includes. It contains no Vulkan types and no engine classes
(only the forward declaration `class Entity;` and a hidden `Impl`), which was checked with the compiler's include
listing: `sandbox.cpp` pulls in `sandbox.h`, `pipeline_settings.h`, glm and the standard library.

| Class | What students do with it |
|---|---|
| `Sandbox` | the initialization chain `InitializeWindow(title, width, height)`, `CreateInstance`, `PickDevice`, `CreateSwapChain`, `InitializeRendering`, `CreatePipelines`, `CreateCommandBuffers`, `CreateSyncObjects` (see "The initialization chain"); the render loop `IsRunning()` and the six frame calls `BeginFrame`, `UpdateScene`, `BeginRendering`, `DrawScene`, `EndRendering`, `EndFrame` (see "The render loop"); `CreateCamera`, `SetActiveCamera`, `CreateLight(name, LightType)`, `LoadModel(name, file)`, `CreateSphere(name, radius)`; `CreatePipeline(name, shaderFile, settings)`, `AddToPipeline(name, object)`. Owns the tutorial's `Engine` and every object it hands out |
| `SceneObject` | a name plus the entities of a loaded model (one per material) or of a simple mesh. `SetPosition` / `SetRotation` / `SetScale`, `Move` / `Rotate` / `Scale` are forwarded to all parts, angles in degrees. `Part(materialName)` addresses one part; the glTF loader names its entities `<model>_Material_<index>_<materialName>` |
| `Camera` | `SetPosition`, `SetRotation` (degrees; without a rotation it looks along -Z), `SetFieldOfView` |
| `Light` | `SetPosition`, `SetRotation` or `SetDirection` (a light shines along the -Z axis of its transform; `SetDirection` computes the rotation), `SetColor`, `SetIntensity`, `SetRange`, `SetConeAngles` (degrees) |

A failed `LoadModel()` and an unknown `Part()` print an error and return an empty object, so that calls on it do
nothing. A second `LoadModel()` appends its glTF lights to those of the first (the tutorial's loader replaces the
list). `CreateSphere()` queues the GPU upload itself and, since 2026-10-04, turns the triangle order of the tutorial's
sphere around: the engine's pipelines take counter-clockwise triangles as the front, the tutorial's sphere lists them
clockwise and was drawn inside out. `SetActiveCamera()` gives the camera the aspect ratio of the window (since
2026-10-04; the engine itself only sets it when the window is resized).
`Run()` ends the engine's initial load cycle when no model was
loaded; without that the loading overlay would stay forever. Since planned change 8 the render loop is in `main()`
(see "The render loop"); `Run()` no longer exists.

### The render loop (planned change 8, own code)

`main()` renders with `while (sandbox.IsRunning()) { BeginFrame(); UpdateScene(); BeginRendering(); DrawScene();
EndRendering(); EndFrame(); }`. `IsRunning()` is the head of the tutorial's `Engine::Run()` loop (window events,
delta time, FPS title) and is false once the window was closed; the six calls are the six parts of the tutorial's
`Renderer::Render()` (the table in "One frame"), `UpdateScene()` preceded by the tutorial's `Engine::Update()`
(camera controls, ImGui frame, entity updates, which include the terminal commands). The sandbox layer keeps the
call that is expected next: a frame call out of order prints `Frame sequence error: DrawScene() cannot run yet.
Rendering stopped.` (since 2026-10-04 without the name of the call that is missing, see "Terminal output"; a call
that was already made in the frame prints `... was called twice in this frame`), every later frame call does
nothing, and `IsRunning()` returns false, so the loop ends and the program exits normally. `IsRunning()` itself requires the previous frame to be
complete (a forgotten `EndFrame()` is reported the same way) and, on its first call, the complete initialization
chain and an active camera; the first call also does what the old `Run()` did before its loop: it ends the load
cycle when no model was loaded and starts the terminal reader. An exception inside a frame call is printed and stops
rendering, as the old `Run()` caught it. What was `Engine::Run()`, `Engine::Render()`, `Renderer::Render()` and
`Sandbox::Run()` is gone; every line of the first three lives on in the calls.

### The initialization chain (planned change 7, own code)

`main()` initializes the engine with eight calls in a fixed order; each one is a group of the Vulkan set-up steps
of the tutorial's `Engine::Initialize()` and `Renderer::Initialize()`, which were split along this table on
2026-09-22 (inside a group the engine's internal order is unchanged):

| Call | What the engine does |
|---|---|
| `InitializeWindow(title, width, height)` | `Engine::InitializeWindow()`: the GLFW platform and window, the four input callbacks |
| `CreateInstance()` | `Engine::CreateInstance()` creates the `Renderer`; `Renderer::CreateInstance()`: the vulkan.hpp dispatcher, the instance, the debug messenger, the surface |
| `PickDevice()` | `Renderer::PickDevice()`: the physical device, the logical device with its queues, the memory pool |
| `CreateSwapChain()` | `Renderer::CreateSwapChain()`: the swap chain and its image views |
| `InitializeRendering()` | `Renderer::InitializeRendering()`: the dynamic rendering attachment info, the depth image, the off-screen opaque colour images. The two images used to come after the command pool; they can come first because `transitionImageLayout()` records with a temporary pool |
| `CreatePipelines()` | `Renderer::CreatePipelines()`: the PBR descriptor set layouts and pipelines (opaque, blended, glass), the composite pipeline, the light storage buffers |
| `CreateCommandBuffers()` | `Renderer::CreateCommandBuffers()`: the command pool, the descriptor pool, the transparent descriptor sets, the default textures, the fallback sets, the shared PBR textures, the command buffers |
| `CreateSyncObjects()` | `Renderer::CreateSyncObjects()`: semaphores and fences, the thread pool, the uploads worker, the watchdog; then `Engine::CreateSyncObjects()` creates the model loader and the ImGui system. The engine is initialized after it |

The order check lives in the sandbox layer (`sandbox_impl.cpp`): a `done` flag per call. A call of the chain checks
that it was not made before and that every call in front of it is done; `Run()`, `LoadModel()`, `CreateSphere()`
and `AddToPipeline()` need all eight, `CreatePipeline()` needs `CreatePipelines()` (it copies the PBR pipeline
layout created there). Since 2026-10-04 the error names the call that was made, not the one that is missing:
`Initialization error: CreateSwapChain() cannot run yet. Initialization stopped.`, followed by the chain view with
that call marked and the reason (see "Terminal output"). The first error stops the initialization: the call and
every call after it do nothing and print nothing (`LoadModel()` and `CreateSphere()` return an empty object), and
`IsRunning()` returns false without rendering.
`CreateCamera()`, `CreateLight()`
and `SetActiveCamera()` are not guarded: they only create entities and their handles stay usable. Engine and
Renderer do no checking of their own, like the tutorial's private helpers; the `Sandbox` is their only caller.
`Engine::Initialize()`, `Renderer::Initialize()` and `Sandbox::Initialize()` no longer exist. The name of the first
call is `InitializeWindow` rather than the roadmap's working name `CreateWindow`, which is a `windows.h` macro.

### Terminal commands (planned change 6, own code)

While the engine renders, objects and lights can be moved from the terminal the engine was started from:
`Room.Move(1, 0, 0)`, `Sun.Rotate(0, 30, 0)` (degrees), `Sphere.Scale(2, 2, 2)`. The name is the one given to
`LoadModel()`, `CreateSphere()` or `CreateLight()`; a command on an object reaches all its parts. Blanks are allowed
anywhere and a `;` may end the line. Everything else (a wrong spelling, a missing number, an unknown name, `Scale` on
a light) answers with one line on stderr that lists the three forms and the known names. A successful command prints
nothing. Cameras are not addressable, because the fly controls overwrite the camera rotation every frame.

How it works: `Run()` starts a detached reader thread that blocks in `std::getline(std::cin)` and pushes each line
into a mutex-guarded queue (`TerminalCommands`, `terminal_commands.h/.cpp`). `Run()` also creates an entity
"TerminalCommands" with a `TerminalCommandComponent`; the tutorial's `Engine::Update()` updates all components once
per frame on the main thread and skips that while a model is loading, so the component is the place where the queue
is drained and the commands are applied to the `TransformComponent`s (`Sandbox::Impl::ApplyTerminalCommands()` in
`sandbox_impl.cpp`). No tutorial file changed for this. Lines typed while the loading overlay shows are applied when
loading ends. At exit the reader thread is still blocked in `getline`; being detached, it does not keep the process
alive (checked with a pipe and with a console). Since planned change 8 the entity updates, and with them the
draining, happen inside the frame call `UpdateScene()`.

### Terminal output (planned change 9, own code)

Three views on the terminal, in the form of `docs/ROADMAP.md`, "Terminal output". Printed once to stdout by the first
`IsRunning()`, after its one-time work and the start-up log (by then the chain is complete, so every call of it is
in the sandbox file): the initialization chain as one line per call (`[ok]`,
the name, what the call does); every pipeline as a block (`Pipeline "toon"    shaders/toon.slang -> shaders/toon.spv`,
then the seven stages input assembly, vertex shader `VSMain`, rasterization with the cull mode, fragment shader
`PSMain`, depth test, color blending and attachments, each marked `yours` when it is one of the three settings of an
own pipeline and `fixed` otherwise, so every line of the engine's `"pbr"` block is fixed; then `Objects` with the
names of the objects the pipeline draws). The frame sequence, one line per call with `[ok]`, is printed once after
the first complete frame.

Since 2026-10-04 a view only shows calls that the sandbox file has made. The students are meant to find the calls
and their order in the wiki; the earlier form (the missing call named in the error line and marked `<- missing`, the
calls after it listed with `[  ]`, the whole frame sequence at start-up) gave the solution away one run at a time.
After an error, on stderr: the calls that are done as `[ok]` lines, then the call that was made and could not run,
marked `[!!]` with the reason, and nothing below it:

```
Initialization error: CreateSwapChain() cannot run yet. Initialization stopped.

  [ok] InitializeWindow  the window and its input callbacks
  [ok] CreateInstance    Vulkan instance, debug messenger, window surface
  [!!] CreateSwapChain   <- too early: a swap chain is created by a logical device, with an image
                            format the GPU supports. No device exists yet.
```

The reason says in Vulkan terms what the call needs and never names another call. The other reasons are
`<- called twice: ...` (a chain call that is done, a frame call that was already made in this frame) and
`<- failed` (the engine's work failed or threw). `IsRunning()` is reported the same way when the chain is not
complete or the previous frame was not finished, the scene calls when the chain is not complete. The first
initialization error stops the initialization and the first frame error stops rendering, so there is one view per run.

How it works (`sandbox_impl.cpp`): description arrays and requirement arrays (the reason texts) next to the name
arrays of the two call enums. A fixed text per call is enough: the calls that are done are always a prefix of the
order, so a call that is too early always lacks what the call directly in front of it creates.
`PrintCallList()` formats one list (the done calls, then the marked call; the name column is as wide as the longest
printed name plus two, and the lines of a reason start in the same column), `PrintPipeline()` one
block (columns of 19 and 43 characters); the `Sandbox` keeps a `PipelineView` (name, shader file, settings, engine
flag, object names) per pipeline, `"pbr"` first when `CreatePipelines()` succeeds, then one per successful
`CreatePipeline()`, and `AddToPipeline()` appends the object's name to its view; an object that was added to no
pipeline is listed under `"pbr"`, which is how the renderer draws it. A part is named after its object and material
(`Room.wood`, `SceneObject::Part()`) so that the objects line is unambiguous. The `.spv` name in the heading is
derived as the renderer derives it. The printed text uses American spelling like the rest of the code. No tutorial
file changed.

## Files

| File(s) | Responsibility | Used at run time |
|---|---|---|
| `sandbox.cpp` | the students' file: window size, `SetupScene(Sandbox &)` with the example scene (camera, "Sun", Viking room, a sphere), `main()` with the eight initialization calls (planned change 7) and the render loop of six calls per frame (planned change 8). Own code; takes the place of the tutorial's `main.cpp` since planned change 5 | yes |
| `sandbox.h`, `sandbox_impl.cpp` | the student-facing layer (see "The sandbox layer"): `Sandbox`, `SceneObject`, `Camera`, `Light`; crash reporter start, validation toggle, the initialization chain and the render loop, each with its order check (planned changes 7 and 8); since planned change 6 also `TerminalCommandComponent` and the code that applies the terminal commands. Own code | yes |
| `terminal_commands.h/.cpp` | `TerminalCommands`: the reader thread on `std::cin`, the queue of typed lines, the parser of `Name.Move(x, y, z)` / `Rotate` / `Scale` (see "Terminal commands"). No engine or Vulkan include. Own code, planned change 6 | yes |
| `engine.h/.cpp` | the eight calls of the initialization chain (since planned change 7; the tutorial's `Initialize()` split), `IsRunning()` and the six frame calls (since planned change 8; the tutorial's `Run()` and `Render()` split: window events, delta time, FPS title, the entity snapshot), entity list with removal queue, camera fly controls, input routing to ImGui | yes |
| `platform.h/.cpp` | `Platform` interface + `DesktopPlatform` (GLFW) | yes |
| `renderer.h` | the whole `Renderer` class declaration, UBO / push-constant structs, `LoadingPhase` | yes |
| `renderer_core.cpp` | the seven renderer calls of the initialization chain (since planned change 7; the tutorial's `Initialize()` split), instance, debug messenger, device and feature selection, cleanup, watchdog | yes |
| `renderer_pipelines.cpp` | descriptor set layouts and graphics pipelines: PBR (opaque, blended, glass; a premultiplied-alpha variant is declared in `renderer.h` but never created, as in the tutorial) and composite. Since planned change 4 also the named pipelines: `createNamedPipeline()`, `CreatePipeline()`, `AddToPipeline()` (own code) | yes |
| `pipeline_settings.h` | `CullMode`, `PipelineSettings` (cull mode, depth test; blending removed 2026-10-09): the description of a named pipeline, without Vulkan types (own code, planned change 4) | yes |
| `renderer_resources.cpp` | buffers, images, textures (KTX2 via libktx), mipmaps, per-entity resources, streaming queues | yes |
| `renderer_rendering.cpp` | the six frame calls `BeginFrame` to `EndFrame` (since planned change 8; the tutorial's `Render()` split, see below), light extraction, culling, the "Renderer" ImGui panel | yes |
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
| `template.slang` | vert + frag | nothing yet: the blank starting point for student shaders (transform plus UV in `VSMain`, the unlit base colour texture sample in `PSMain`). Cut out of the tutorial's `texturedMesh.slang`; compiled to `template.spv`, first usable with the named pipelines of planned change 4. Since 2026-10-07 on `learning` and since 2026-10-09 on every branch the template also declares the light buffer and the push constants, hands the world position and the normal to `PSMain`, and carries the three slot comments `ambient` / `diffuse` / `specular` for the worksheet's Phong task |
| `phong.slang` (branch `phong-solution` only) | vert + frag | the sphere of the example scene, through the pipeline `"phong"` that `sandbox.cpp` creates: the template with the lecture's Phong formula in `PSMain` (ambient, diffuse and specular term over all scene lights, gold constants, see `docs/DELETIONS.md`, "Added code") |
| `imgui.slang` | vert + frag | ImGuiSystem |
| `common_types`, `pbr_utils`, `lighting_utils`, `tonemapping_utils` | modules | imported by the above, not compiled standalone |

The second mesh shader, `texturedMesh.slang` (the "basic" path behind the panel option "Use Basic Lighting (Phong)"),
was deleted on 2026-09-21 with its pipeline, descriptor set layout and per-entity descriptor sets (planned change 2).

## Named pipelines (planned change 4, own code)

`Renderer::CreatePipeline(name, shaderFile, settings)` builds a pipeline for a student shader. The student chooses
three things: the shader file (`"shaders/x.slang"`, loaded as `"shaders/x.spv"`, entry points `VSMain` and `PSMain`),
the cull mode (default none) and the depth test (default on; compare `LessOrEqual`). A named pipeline never blends
and is always drawn in the opaque pass (the blending setting was removed on 2026-10-09, see `DELETIONS.md`: no
object of the teaching unit is transparent). Everything else is copied
from the opaque PBR pipeline, including the PBR pipeline layout, so every named pipeline receives exactly the inputs
of the next section. The descriptions are kept in creation order and `recreateSwapChain()` rebuilds the pipelines
from them. Call `CreatePipeline()` after `CreatePipelines()` (planned change 7; it creates the layout that is copied)
and before rendering; the name `"pbr"` is reserved.

`Renderer::AddToPipeline(name, entity)` records, in the renderer, which pipelines an entity was added to (an unknown
name is an error that lists the known names). An entity that was added to no pipeline is drawn as before, with the
engine's PBR pipelines (`"pbr"`: opaque, blended or glass, chosen from the material). An entity in several pipelines
gets one render job per pipeline, in creation order with `"pbr"` first. It may be called from the loading thread.

## What a mesh shader gets from the engine

Since 2026-09-21 these are the complete inputs, which is what a student has to know to write a shader. There is one
mesh layout, the PBR one; the pipelines of planned change 4 will use it too:

| Input | Content |
|---|---|
| Vertex attributes | locations 0 to 3: position, normal, UV, tangent; 4 to 7: per-instance model matrix; 8 to 10: per-instance normal matrix, one column per location. A shader that assembles it with `float3x3(a, b, c)` has to transpose the result, because that Slang constructor takes rows (`pbr.slang` does so since 2026-10-04) |
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

Since planned change 8 (2026-09-22) the tutorial's `Render()` is six calls in a fixed order, each a member function
of `Renderer`, called by the `Engine`'s six frame calls, which the students' loop makes (see "The render loop").
What one call leaves for the next (the acquired image index, the semaphore index, the job lists, the pass-1
viewport) lives in the member `frame` (`FrameInProgress`). Every line inside the six is the tutorial's, moved; the
cut and its reasons are in `docs/IMPLEMENTATION_PLAN.md`, step 8.

1. `IsRunning()` (`Engine::IsRunning`, the head of the tutorial's `Run()` loop): `platform->ProcessEvents()`, delta time, the FPS title.
2. `BeginFrame()`: wait on this frame slot's fence, reset it. Safe point: drain pending mesh uploads and entity preallocations; apply dirty descriptor writes for this frame index. `acquireNextImage`; out-of-date / suboptimal recreates the swap chain at once and marks the frame as skipped: the remaining calls of this frame do nothing (the empty submit that signals the fence stays, as in the tutorial's early returns).
3. `UpdateScene()`: first `Engine::Update()` (camera controls; ImGui `NewFrame`; entity updates, skipped while loading; among the entity updates the `TerminalCommandComponent` applies the terminal commands typed since the last frame), then a snapshot of entity pointers and the renderer's part: build the frame light list from `staticLights` (the glTF lights) plus the lights of all active entities with a `LightComponent`, upload to the light storage buffer; fill the UBO template from the camera. Preparation pass: collect active entities with GPU resources, per-frame descriptor cold-init, frustum culling, distance LOD; sort into opaque and transparent jobs. An entity that was added to named pipelines gets one job per pipeline (a pipeline with blending: transparent list). Loading-complete check, deferred descriptor writes, transparent jobs sorted back to front (stable, so the jobs of one entity keep their pipeline order).
4. `BeginRendering(imguiSystem)`: grow the light storage buffer if needed, begin the command buffer, process pending texture uploads, draw the "Renderer" panel. Pass 1 begins: clear colour and depth of the off-screen colour image.
5. `DrawScene()`: draw the opaque jobs into the off-screen colour image, each with the pipeline of its job (`pbr` unless the entity was added to a named pipeline). Pass 1b: `composite` draws that image to the swap chain (exposure, tone map). Pass 2 begins: draw the transparent jobs onto the swap chain with the blended PBR pipeline, the glass pipeline or the job's named pipeline; glass samples the off-screen colour.
6. `EndRendering(imguiSystem)`: end pass 2, transition to present, `imguiSystem->Render()` in its own dynamic-rendering pass on top, end the command buffer.
7. `EndFrame(imguiSystem)`: a skipped frame only ends ImGui's frame. Otherwise `submit2` with the frame fence, `presentKHR`; recreate the swap chain on out-of-date. Advance `currentFrame` (MAX_FRAMES_IN_FLIGHT slots).

Three things moved with the cut, compared with the tutorial's order: `Engine::Update()` runs after the acquire of
`BeginFrame()` instead of before the whole frame; the light upload and the UBO template come after the fence wait
and after that update, so that the camera and the transforms of the frame are current; and the image is acquired
before the preparation pass rather than after it. Everything that writes descriptors or may resize the light buffer
still runs before the command buffer begins recording, which is what `isRecordingCmd` requires.

There is no depth pre-pass and no compute work any more; every opaque PBR draw uses `pbrGraphicsPipeline` (depth test
`Less`, depth writes on).

A watchdog thread checks `Render()` progress labels and aborts with the last label if no frame finishes for 10 s.
`watchdogSuppressed` still exists, but only acceleration-structure builds ever set it, so it is always false now.

## Build files

One `CMakeLists.txt`: dependencies through vcpkg CMake configs, `PLATFORM_DESKTOP` define, Slang compile rules, MSVC flags
(`/W3 /MP /bigobj /permissive-`), tinygltf configured without stb image (images go through libktx). CPack section is
unused. `CMakePresets.json` has one configure preset and Debug / Release build presets.
