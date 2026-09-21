# Roadmap

Owner: Paulo. Purpose of the port and the planned changes, in order. Each entry names the rule that applies
(`docs/CODE_CHANGE_RULES.md`). Status: `todo`, `in progress`, `done`, `dropped`.

## Purpose

This engine serves as a learning material for Vulkan and graphics programming at the HTWK Leipzig. It is not intended to be a production-ready engine.
It got developed in the scope of the bachelore theosis of Paulo Hoheisel.

## Planned changes

The owner approved this order on 2026-09-21. Implementation started the same day: the owner names a step, and only
that step is implemented. The target is the student-facing interface at the end of this file. Logging, builds and
runs follow `docs/WORKFLOW.md`. Steps 1 and 2 only delete code (step 2 also adds the template shader); step 3 swaps a
shader and adds the light component; the student-facing interface starts with step 4. The sub-steps, the code each
step touches and its checks are in `docs/IMPLEMENTATION_PLAN.md`. Step 1 is done and listed under "Done"; the
remaining steps keep their numbers, because this file and the plan refer to them by number.

2. `in progress` since 2026-09-21: the deletion part is done and built (log in `docs/DELETIONS.md`); the template
   shader is proposed and waits for the owner's approval (Rule 1), and the owner's run is pending.
   Rule 2, and Rule 1 for the template: remove the "Use Basic Lighting (Phong)" option. That is the checkbox
   with its status lines, `pbrEnabled` / `IsPBREnabled()` / `SetPBREnabled()` in `ImGuiSystem`, the `useBasic`
   branches of the draw loop, `createGraphicsPipeline()` with `graphicsPipeline` and `pipelineLayout`, the basic
   descriptor set layout and the per-entity basic descriptor sets with their flags. `texturedMesh.slang` is deleted
   completely, together with its compiled `.spv` and everything that loads it, so that nothing is left that could
   fail at run time. Every object is drawn with `pbr.slang`. A template shader for students is added, as blank as
   possible (working name `shaders/template.slang`; the engine does not load it, and it is first usable with step 4).
3. `todo`, Rule 3 plus Rule 2, and Rule 1 for the component, the merged light list and the `scaleIBLAmbient` line:
   real lighting. `pbr.slang` is replaced by the tutorial's `pbr_full.slang`, trimmed by deletion. The owner's rule
   for the trimming: keep what the rasterization pipeline uses, drop what belongs to a removed rendering option or
   needs ray tracing; the result is the table below. It is the shader the tutorial itself loaded as `pbr.slang` until
   its commit `ccc8dd8` ("Add in Android build support") swapped in the stripped 109-line version. A scene without a
   light is then black, so the same step adds `LightComponent` (an entity with a transform, collected every frame),
   merges glTF lights and entity lights instead of replacing the list, and puts a light into the example scene.
   Closes the light part of `tr:szenenmanagement` and makes the name `"pbr"` true.
4. `todo`, Rule 1: named pipelines. One function that builds a pipeline from a description (shader file, cull mode,
   depth test, blending), patterned on the opaque pipeline of `createPBRPipeline()` and using the PBR layout; the
   descriptions are kept so that a swap-chain recreation can rebuild the pipelines; `AddToPipeline(name, entity)`;
   the draw loops take the pipeline from the object (none: `"pbr"`; several: one draw per pipeline; blending: the
   transparent pass). Closes `einfache_pipeline` and `pipeline_shader_automatisierung`.
5. `todo`, Rule 1: the sandbox layer. `sandbox.h` without Vulkan types, `SceneObject`, a `LoadModel()` that waits,
   `CreateSphere()` with automatic GPU upload, `CreateCamera()`, `CreateLight()`, `AddToPipeline()` for all parts of
   an object. `sandbox.cpp` takes the place of `main.cpp`, for now still with `Initialize()` and `Run()`. Closes the
   rest of `tr:szenenmanagement`.
6. `todo`, Rule 1: terminal commands (`Name.Move(x, y, z)`, `Rotate`, `Scale`): a reader thread, a queue, applied on
   the main thread once per frame, ignored while a model is loading. From here on they help to test the later steps.
7. `todo`, Rule 1: the initialisation chain. `Renderer::Initialize()` is split into the eight calls, each with its
   prerequisite check; `CreatePipeline()` requires `InitializeRendering()`.
8. `todo`, Rule 1: the frame calls. `Renderer::Render()` (about 870 lines) is split into `BeginFrame`, `UpdateScene`,
   `BeginRendering`, `DrawScene`, `EndRendering` and `EndFrame` with the order check; `IsRunning()` and the loop move
   into `sandbox.cpp`. The riskiest step, therefore late: everything it moves is stable by then. Closes `lifecycle`.
9. `todo`, Rule 1: terminal output. The start-up print of the initialisation chain, of the pipelines with their
   objects and of the frame sequence; the same views on an error. Closes `ascii_pipeline` (Could).
10. `todo`, documentation: a README for students: the shape of `sandbox.cpp`, what a shader receives and the template,
    a new shader file needs a CMake re-configure, PNG to KTX2 with `toktx`, the terminal commands.

### Planned change 3 in detail: what is kept from `pbr_full.slang`

Checked on 2026-09-21 against the tutorial file (634 lines). What is kept needs set 0 bindings 0 to 6, set 1
binding 0 and the material push constants, which is exactly the PBR layout the engine has today.

| Part of `pbr_full.slang` | Verdict | Reason |
|---|---|---|
| `VSMain`; material sampling: base colour, metallic-roughness or spec-gloss, normal map, occlusion, emissive with emissive strength, alpha-mask discard | keep | Material data of the raster pipeline; no panel option involved. |
| Light loop over all scene lights (binding 6): directional, point, spot and emissive lights, distance falloff, spot cone, GGX specular plus diffuse | keep | The lighting of the raster pipeline. Its enclosing `if` names Forward+ variables and is reduced to a bare scope, so the loop body stays byte-identical. |
| Ambient term `albedo * ao * 0.1 * ubo.scaleIBLAmbient` | keep | Raster code. It stays 0 until C++ writes `scaleIBLAmbient` into the raster UBO; the tutorial only ever set it for its ray-query UBO. The owner approved the one new line on 2026-09-21 (Rule 1): the one with `1.0f`, as close to the tutorial as possible. It is the tutorial's own line from `renderer_rendering.cpp:1884-1886`, the first comment line plus `scaleIBLAmbient = 1.0f;`, written to `frameUboTemplate` in `prepareFrameUboTemplate()`. The port does not write the value anywhere today. The tutorial's second comment line is left out because it talks about Ray Query. With 1.0 the ambient light is 10 % of the surface colour, the value `texturedMesh.slang` hard-codes today. |
| Emissive term | keep | Raster code. |
| Glass (`GlassPSMain`): transmission of the opaque scene colour (set 1), tint, rim, emissive surface term | keep | The raster glass pipeline exists and uses it. |
| Glass post-processing: exposure, filmic tone map, gamma on a swap chain that is not sRGB | keep | Glass is drawn after the composite pass and has to tone-map itself. The exposure slider exists; the gamma path was kept on purpose. |
| Forward+ tile lists: bindings 7 and 8, `ubo.padding1`, `nearZ` / `farZ` / `slicesZ`, the tile loop in `PSMain`, all light highlights on glass (they only exist as a tile loop) | drop | The option "Forward+ (tiled light culling)" and its compute pass were removed on 2026-09-21. |
| Ray-traced shadows: `traceShadowOccluded()`, the TLAS at binding 11, `ubo.padding2` | drop | It was a raster option ("RayQuery shadows (raster)"), but it is ray-tracing specific: a `RayQuery` against the TLAS needs the acceleration structures and the two ray-tracing extensions, all removed on 2026-09-21 together with the option. |
| Ray-query shared buffers at bindings 12 and 13 | drop | Declared, never read in the file; the buffers were removed with the acceleration structures. |
| Planar reflections: `reflectionMap` at binding 10, the reflection sample and the Fresnel reflection mix in glass (scaled by `ubo.reflectionIntensity`), the clip-plane discard in `PSMain` | drop | Planar reflections (never active upstream) and the "Reflection intensity" slider were removed on 2026-09-21; C++ no longer writes these UBO fields. |
| `#if !defined(PLATFORM_ANDROID)` guards | drop the guard lines, keep their content | Android was removed from the port. |

## Done

- 2026-09-20, `done`, Rule 2: removed the ray-query render mode (the "Ray Query" entry of the mode combo, its panel
  options, `ray_query.slang`, its compute pipeline, descriptor sets, UBO and output image). Rasterization is the only
  render mode and its panel is unchanged. The acceleration-structure subsystem stayed because rasterization uses it;
  what was kept and the forced edits are listed in `docs/DELETIONS.md`.
- 2026-09-21, `done`, Rule 2: removed every panel control without a visible effect, with its machinery. Owner's use
  case: students who never worked with Vulkan write simple shaders and learn the basics. Four stages, each built:
  (1) "Rendering Mode" combo with `RenderMode`, "Reflection intensity", the "Gamma" slider; (2) planar reflections
  with "Reflection resolution scale"; (3) the rest of ray query: "RayQuery shadows (raster)", `renderer_ray_query.cpp`,
  the BLAS / TLAS build and its loading phase, PBR bindings 11 to 13, the three ray-tracing device extensions;
  (4) Forward+: the checkbox, `renderer_compute.cpp`, `forward_plus_cull.slang`, the depth pre-pass, PBR bindings 7 to 9.
  The PBR set layout is now bindings 0 to 6; the compiled shaders are byte-identical to the tutorial's. Kept items and
  forced edits: `docs/DELETIONS.md`.
- 2026-09-21, `done`, Rule 2, planned change 1: deleted the dead `Pipeline` class (`pipeline.h/.cpp` and its entry in
  `CMakeLists.txt`). Its name collided with the pipelines of step 4, and it was the only other code that names
  `texturedMesh.spv`. No forced edit; Debug build with 0 errors. The other three dead files of the candidates list
  were not part of the step and are still undecided.

## Candidates already visible in the code (not decided)

- Remove the three compiled-but-unreferenced files that are left: `descriptor_manager.*`, `renderdoc_debug_system.*`,
  `resource_manager.*` (the last is owned by Engine but never called). Rule 2. Still undecided. The fourth one,
  `pipeline.*`, went on 2026-09-21 as planned change 1.
- Remove the CPack / install section at the end of `CMakeLists.txt`. Rule 2.
- Slim `UniformBufferObject` (C++ and `common_types.slang`): `padding1`, `padding2`, `slicesZ`, the reflection fields
  and the ray-query fields are never written since 2026-09-21. Rule 2 on both sides; it changes `pbr.spv` and
  `texturedMesh.spv`, which are byte-identical to the tutorial today. Worth it for the use case: the UBO is the first
  thing a student reads when writing a shader.
- Drop `eShaderDeviceAddress` from the vertex / index buffers (only ray tracing needed the addresses) and the required
  `bufferDeviceAddress` feature. Rule 2, but it moves every mesh buffer from the dedicated-block branch of
  `MemoryPool::createBuffer()` to the pooled branch, so it needs a test with a large scene first.
- Lower `storageBufferDescriptors` in `createDescriptorPool()` from six storage buffers per PBR set to the one that is
  left. Not a deletion (a number changes), so Rule 1.
- The "Use Basic Lighting (Phong)" toggle switches between two shaders that are neither Phong nor PBR (fixed light
  direction, diffuse only, `scaleIBLAmbient` never set). Decided on 2026-09-21: the toggle and the basic path go
  (planned change 2), and `pbr.slang` becomes what its name says (planned change 3).
- Let a raster shader read the scene lights (PBR binding 6 is uploaded every frame and unread). Decided on
  2026-09-21: planned change 3, with the tutorial's own shader (Rule 3 plus Rule 2) instead of new code.
- "Sampler max anisotropy" recreates the samplers without rewriting the descriptor sets that reference the old ones
  (tutorial behaviour). A fix is new code: Rule 1. Removing the slider is Rule 2.
- Remove the loading watchdog thread. Rule 2. `watchdogSuppressed` is always false since the AS builds are gone.

## Student-facing interface (target description, nothing of it exists yet)

What the students work with. Everything in this section is new code (Rule 1), except the removal of the Phong
option (Rule 2). It belongs to the requirements `lifecycle`, `einfache_pipeline`, `ascii_pipeline` and the open
parts of `tr:szenenmanagement` in `docs/REQUIREMENTS.md`. The owner made the decisions below on 2026-09-21; nothing
is open. The names of calls and files (`IsRunning`, `AddToPipeline`, `CreateSphere`,
`sandbox.h`, `template.slang`, ...) are working names from the design discussion, not decisions.

### Principles

- The interface is small and abstract. It is a sandbox in which students build scenes and write shaders while still
  seeing the basic structure of a Vulkan application. The focus is on the scene and the shaders, not on the API.
- Students work in one file (`sandbox.cpp`) that always has the same shape: the initialisation chain, `SetupScene()`,
  the render loop.
- Each group of Vulkan steps becomes one call. Example: instead of setting up dynamic rendering step by step, a
  student calls `InitializeRendering()` and the engine does the rest.
- Hidden from the sandbox file, and not reachable through the header it includes: the Vulkan API itself, ImGui,
  descriptor sets, memory management, the inside of the swap chain and of the rendering setup. The debug system is
  hidden too; students only see its errors in the terminal.
- The restriction applies to the sandbox file only. Students may read and change the engine itself.

### Initialisation chain

- Eight calls in a fixed order: `CreateWindow`, `CreateInstance`, `PickDevice`, `CreateSwapChain`,
  `InitializeRendering`, `CreatePipelines`, `CreateCommandBuffers`, `CreateSyncObjects`. They group the roughly twenty
  create calls of `Renderer::Initialize()`; the engine-internal ones (debug messenger, surface, memory pool,
  descriptor pool, default textures, light buffers, ImGui) are folded into them.
- If a student removes a call or changes the order, the engine reports it. Every call checks its own prerequisites
  ("`CreateSwapChain()` needs `PickDevice()`"), and the render loop checks that all eight were made. The error names
  the missing call; once the optional `ascii_pipeline` exists, its view marks the call as well.

### Pipelines and shaders

- One pipeline per shader, any number of objects per pipeline. A pipeline is one shader file (vertex and fragment
  shader) plus fixed-function state; it is created once and never changes. Objects do not own pipelines, and a
  pipeline cannot hold several shaders of the same stage.
- Students set four things: the shader file, the cull mode, the depth test and blending. Everything else (vertex
  layout, topology, multisampling, pipeline layout, attachment formats) is fixed by the engine. Blending switched on
  also means: no depth writes, drawn in the transparent pass.
- Pipelines are created in the initialisation chain only, after `InitializeRendering()`:
  `CreatePipeline(name, shaderFile, settings)`. `CreatePipelines()` creates the engine's own; the one students see
  is `"pbr"`, the default of every object.
- The "Use Basic Lighting (Phong)" option is removed entirely (Rule 2): it is only a shader underneath, and Phong
  shading is something the students implement themselves. The deletion covers the checkbox with its status lines,
  `pbrEnabled` in `ImGuiSystem`, the `useBasic` branches of the draw loop, `createGraphicsPipeline()` with its
  pipeline and layout, the basic descriptor set layout with the per-entity basic descriptor sets, and
  `texturedMesh.slang`, which is deleted completely (nothing else loads it; the dead `Pipeline` class, which also
  named it, went with planned change 1). By default every object is drawn with `pbr.slang`.
- A template shader is the starting point for own shaders: the most neutral shader for both stages, as blank as
  possible. The vertex shader leaves the geometry as it is and only does the conversion no vertex shader can skip,
  from the model's own coordinates to clip space (instance, model, view and projection matrix), and hands on the UV.
  Handing on the input position unchanged is not an option: the object would stick to the screen and ignore its
  transform and the camera. The fragment shader passes the object's colour through unchanged, with no lighting. The
  vertex data of this engine has no colour attribute (position, normal, UV, tangent), so that colour is the sample
  of the base colour texture. Besides that the file holds only the vertex input the engine fixes, the UBO and the
  texture binding. The engine does not load it unless a student creates a pipeline from it. It is new
  shader code (Rule 1), so its exact content is proposed as a diff when step 2 is implemented; to stay close to the
  tutorial it can be cut out of the tutorial's `texturedMesh.slang` by deletion, with `return baseColor;` as the only
  new line.
- Objects are added to a pipeline in `SetupScene()`: `AddToPipeline(name, object)`. An unknown name is an error that
  lists the known names. An object that was added to no pipeline is drawn with `"pbr"`. An object that was added to
  several pipelines is drawn once per pipeline, in the order in which the pipelines were created (for example shading
  plus an outline).
- Shader files are `src/shaders/*.slang`; every file has the entry points `VSMain` and `PSMain`. CMake compiles them
  at build time, and a new file needs a CMake re-configure (to be described in the README). What a shader receives
  from the engine is the table "What a mesh shader gets from the engine" in `docs/ARCHITECTURE.md`; student pipelines
  use the PBR layout.
- There is no interface for own C++ parameter structs (requirement `shader_verwaltung`, removed by the owner on
  2026-09-21). Shader parameters are constants in the shader file.

### Scene

- Students create all scene objects in `SetupScene()`, before the render loop: cameras, lights, models, simple
  meshes. They set properties and textures and add the objects to pipelines, in whatever order they like. No object
  is created or deleted while rendering.
- `LoadModel()` returns a `SceneObject`: a name plus the entities of that model (the loader creates one entity per
  material, all with the same transform). `Move`, `Rotate` and `Scale` are forwarded to all parts, and
  `AddToPipeline` adds all parts; `Part(materialName)` addresses one. `LoadModel()` waits until the entities exist;
  mesh and texture uploads still happen during the first frames, behind the engine's loading overlay.
- A simple mesh needs no file: `CreateSphere(name, radius)` returns a `SceneObject` as well. It uses the existing
  `MeshComponent::CreateSphere()`, and the engine queues the GPU upload itself (today the caller has to invoke
  `Renderer::EnqueueEntityPreallocationBatch()`, otherwise the mesh is never drawn).
- Textures stay KTX2 only; the engine gets no PNG decoding. The README will document the conversion of own images,
  for example `toktx --t2 --target_type RGBA --lower_left_maps_to_s0t0 out.ktx2 in.png` (four channels, rows stored
  bottom-up). Not tried on this machine yet: `toktx` belongs to KTX-Software and is not part of the Vulkan SDK.
- Lights are scene objects as well: an entity with a transform and a `LightComponent`, collected every frame into the
  light list that the renderer already rebuilds and uploads per frame. Two prerequisites, both part of planned
  change 3: a mesh shader that reads the light buffer, and glTF lights and entity lights have to be merged
  (`SetStaticLights()` replaces the whole list today).
- Objects can be modified while rendering: move, rotate (degrees per axis), scale. For now only through terminal
  commands such as `Room.Move(1, 0, 0)`; there is no panel for it. A reader thread queues the lines, `UpdateScene()`
  applies them on the main thread, and they are ignored while a model is loading.

### Render loop

- Six mandatory calls in a fixed order: `BeginFrame`, `UpdateScene`, `BeginRendering`, `DrawScene`, `EndRendering`,
  `EndFrame`. A missing or misplaced call is reported by the next call, and rendering stops.
- `DrawScene()` records the draw commands of every object in one call; there is no `Draw(object)` per object. The
  owner accepted this for `lifecycle` and amended the requirement text on 2026-09-21.
- The engine's four passes (opaque to the off-screen image, composite, transparent, ImGui) stay inside these calls;
  students see one pass.
- The loop condition `IsRunning()` only means that the window is open. The loop also runs while a model is loading,
  exactly as the engine does today: it renders complete frames that show a progress bar instead of the scene.

### Terminal output (`ascii_pipeline`)

Printed once at start-up (initialisation chain, every pipeline, the frame sequence) and again on an error, never per
frame. One pipeline:

```
Pipeline "toon"    shaders/toon.spv

  Input assembly     triangle list, engine vertex layout        fixed
        |
  Vertex shader      VSMain                                     yours
        |
  Rasterisation      cull mode: none                            yours
        |
  Fragment shader    PSMain                                     yours
        |
  Depth test         on, writes depth                           yours
        |
  Colour blending    off                                        yours
        |
  Attachments        off-screen colour image + depth image      fixed

  Objects            Room
```

A wrong call order (the initialisation chain is reported in the same form):

```
Frame sequence error: DrawScene() was called, BeginRendering() was expected. Rendering stopped.

  [ok] BeginFrame      wait for the GPU, acquire image, begin command buffer
  [ok] UpdateScene     scene data -> uniform buffers
  [!!] BeginRendering  <- missing
  [  ] DrawScene
  [  ] EndRendering
  [  ] EndFrame        submit, present
```

### Example of a student file

```cpp
#include "sandbox.h" // the only engine header the sandbox file includes; it contains no Vulkan types

SceneObject *room = nullptr;

// Students create every scene object here, before rendering starts, in any order.
void SetupScene(Engine &engine)
{
    Camera *camera = engine.CreateCamera("Camera");
    camera->SetPosition({2.0f, 0.5f, -2.0f});
    engine.SetActiveCamera(camera);

    Light *sun = engine.CreateLight("Sun", LightType::Directional);
    sun->SetDirection({-1.0f, -1.0f, -1.0f});

    room = engine.LoadModel("Room", "../assets/viking_room/viking_room.gltf"); // waits until the model is loaded
    room->SetRotation({-90.0f, 0.0f, 0.0f});                                   // degrees
    engine.AddToPipeline("toon", room);                                        // unknown name: error
}

int main()
{
    Engine engine;
    engine.CreateWindow("Sandbox", 800, 600);
    engine.CreateInstance();
    engine.PickDevice();
    engine.CreateSwapChain();
    engine.InitializeRendering();
    engine.CreatePipelines();                            // the engine's own; "pbr" is every object's default
    engine.CreatePipeline("toon", "shaders/toon.slang"); // own pipeline; all four settings at their defaults
    engine.CreateCommandBuffers();
    engine.CreateSyncObjects();

    SetupScene(engine); // needs every call above

    // True until the window is closed. The loop also runs while a model is still loading: the engine then
    // renders complete frames that show a progress bar instead of the scene.
    while (engine.IsRunning())
    {
        engine.BeginFrame();     // wait for the GPU, acquire a swap chain image, begin the command buffer
        engine.UpdateScene();    // camera controls, terminal commands, transforms to the uniform buffers
        engine.BeginRendering(); // bind and clear the colour and depth attachments
        engine.DrawScene();      // per object: bind its pipeline, descriptor sets and buffers, then draw
        engine.EndRendering();   // the engine adds tone mapping and its own UI panel
        engine.EndFrame();       // submit the command buffer, present the image
    }
}
```

A pipeline with a setting that is not the default:
`engine.CreatePipeline("outline", "shaders/outline.slang", {.cullMode = CullMode::Front});`

### Open points

None as of 2026-09-21. The last two were confirmed by the owner word for word: the template shader is as blank as
possible and `texturedMesh.slang` is deleted completely; the `scaleIBLAmbient = 1.0f` line is added (the port does
not write that value anywhere today).

### Thesis changes that the owner makes (nothing to do in this repository)

- `architektur_techstack`: tinygltf (glTF 2.0) instead of tinyobjloader.
- `basis_infrastruktur`: dynamic rendering and attachments instead of render pass objects and frame buffers.
- `shader_verwaltung`: removed. That is the row in the requirements table, every reference to it, the count of
  requirements (one Must less) and the two items it was derived from, `fa:2_shader` and `ns:4_material_shader`.
- `lifecycle`: the frame steps are called in a fixed order and one `DrawScene()` call draws the scene; students no
  longer bundle the draw commands in a command buffer themselves.
