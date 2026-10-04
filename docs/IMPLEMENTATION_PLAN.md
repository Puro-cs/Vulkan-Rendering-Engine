# Implementation plan

The sub-steps behind the ten planned changes of `docs/ROADMAP.md`. The roadmap says what was decided and why; this
file says how each step is carried out, which code it touches and how it is checked. Written on 2026-09-21 from the
code as it is today; line numbers are hints and will drift. No step starts before the owner says so; a finished step
carries a "Status" line. Sizes (S, M, L) are estimates.

## Routine of every step

1. The owner gives the go for the step.
2. The rule is named first. Rule 3: the tutorial file and lines are cited. Rule 1: the change is shown as a diff in
   the chat and nothing is written before the owner approves it. Large steps are proposed in several diffs, the
   public interface (header) first.
3. The edit.
4. Debug build with 0 errors. The owner allowed it on 2026-09-21 for every change of steps 2 to 9 without asking
   (`docs/WORKFLOW.md`); the exe is still only run by the owner.
5. After a shader file was added or deleted: `cmake --preset windows-msvc` again (the shader list is a configure-time
   glob), and the stale `.spv` next to the source is removed.
6. The owner runs the exe and reports against the checklist of the step. The count of validation messages stays at
   one (`robustBufferAccess2`).
7. Logs and docs: `docs/DELETIONS.md` (removals, forced edits, added code), `src/source-file-difference.md`,
   `docs/ARCHITECTURE.md`, `docs/BUILD.md`, the status in `docs/REQUIREMENTS.md`, and in `docs/ROADMAP.md` the step
   moves from "Planned changes" to "Done".
8. The owner commits.

## Step 1: delete the dead `Pipeline` class (Rule 2, S)

1. Delete `src/pipeline.h` and `src/pipeline.cpp`. Nothing includes `pipeline.h` except `pipeline.cpp` itself.
2. Delete the line `src/pipeline.cpp` from the source list in `CMakeLists.txt`.
3. Not part of the step unless the owner decides it: `descriptor_manager.*`, `renderdoc_debug_system.*`,
   `resource_manager.*`.

Check: the build passes; the run is unchanged.

Status: done on 2026-09-21. Both files and the CMake line are deleted, no forced edit. Debug build with 0 errors; the
regenerated project no longer lists `pipeline.cpp`. The owner's run is pending. Sub-step 3 was not decided, the three
files stay. Log: `docs/DELETIONS.md`.

## Step 2: remove the Phong option, delete `texturedMesh.slang`, add the template (Rule 2, Rule 1 for the template, M)

Order chosen so that every intermediate state still compiles.

1. Panel (`renderer_rendering.cpp`, about 1079 to 1097): the group "Rasterization Options" with the checkbox, the
   console line and the four status lines. The group is empty afterwards and goes with its separator.
2. Draw loop (`renderer_rendering.cpp`, about 1208 to 1255): the `useBasic` variable and its three branches (pipeline
   choice, descriptor set choice, bind call). The PBR branches stay; a deleted condition is reduced to a bare scope so
   that the remaining lines stay unchanged.
3. `ImGuiSystem`: `pbrEnabled`, `IsPBREnabled()`, `SetPBREnabled()` (`imgui_system.h`). No other reader exists.
4. Basic pipeline: `createGraphicsPipeline()` (`renderer_pipelines.cpp`, about 201 to 361), its declaration, the
   members `graphicsPipeline` and `pipelineLayout`, the call in `Renderer::Initialize()`, the call in
   `recreateSwapChain()`, the resets in `Cleanup()` and `cleanupSwapChain()`.
5. Basic descriptor sets, the largest part: `createDescriptorSetLayout()` with `descriptorSetLayout`, the members
   `basicDescriptorSets`, `basicUboBindingWritten`, `basicImagesWritten` (28 references in `renderer_core.cpp`,
   `renderer_rendering.cpp`, `renderer_resources.cpp`), and the four calls `createDescriptorSets(..., false)`
   (`renderer_rendering.cpp` 483 and 796, `renderer_resources.cpp` 1392 and 1513). Inside `createDescriptorSets()` and
   `updateDescriptorSetsForFrame()` the selections `usePBR ? pbr : basic` lose their second operand; these are forced
   edits and are listed as such.
6. Delete `src/shaders/texturedMesh.slang` and `src/shaders/texturedMesh.spv`; re-run the configure step.
7. Kept on purpose: the pool size "2 pipeline types per entity" in `createDescriptorPool()` (unused capacity, as with
   the earlier removals).
8. Template shader (Rule 1): the diff of `src/shaders/template.slang` is proposed, approved, then written. Content as
   decided: vertex input, UBO, base colour texture, a `VSMain` that only does the transform and hands on the UV, a
   `PSMain` that returns the texture sample. Cut out of the tutorial's `texturedMesh.slang` by deletion, with
   `return baseColor;` as the only new line. CMake compiles it; nothing loads it before step 4.

Check: the panel shows "Culling & LOD" and "Tone Mapping & Tuning" only; the room renders as before; `pbr.spv` is
still byte-identical to before (no shader source changed). Docs: the shader table and the table "What a mesh shader
gets from the engine" in `ARCHITECTURE.md` lose the basic column; `BUILD.md` loses the panel group and the known issue
about two mesh shaders; `REQUIREMENTS.md` mentions the toggle in `einfache_pipeline` and `tr:szenenmanagement`.

Status: sub-steps 1 to 7 done on 2026-09-21. Six code files changed by deletion (395 lines deleted, 10 code lines and
4 comments changed as forced edits, all listed in `docs/DELETIONS.md`), `texturedMesh.slang` and both `.spv` copies
deleted, configure re-run, Debug build with 0 errors, `pbr.spv` / `composite.spv` / `imgui.spv` byte-identical to
before. Beyond the plan, because only the basic branches read them: `resolvedTexturePath` in `createDescriptorSets()`
and `newlyAllocated` in `updateDescriptorSetsForFrame()` went too, as did the three
`updateDescriptorSetsForFrame(..., false, ...)` calls (the plan only named the four `createDescriptorSets(..., false)`
calls). Kept: the `usePBR` and `texturePath` parameters. Sub-step 8 (template): the owner approved the diff on
2026-09-21; `src/shaders/template.slang` is written (69 lines, one line that is not a tutorial line) and compiles to
`template.spv` in the project build. The step is done; the owner's run is pending. Docs are updated as listed above.

## Step 3: real lighting (Rule 3 plus Rule 2, Rule 1 for three additions, M to L)

1. Rule 3: copy the tutorial's `shaders/pbr_full.slang` (634 lines) byte for byte over `src/shaders/pbr.slang`. The
   file keeps the name `pbr.slang` because that is what the engine loads, so no C++ changes; the mapping is noted in
   `src/source-file-difference.md`.
2. Rule 2: trim by deletion, following the table in the roadmap. In file order: the bindings 7, 8, 10, 11, 12, 13;
   `RASTER_SHADOW_EPS` and `traceShadowOccluded()`; in `PSMain` the tile computation and the Forward+ loop, the shadow
   test inside the global loop, the clip-plane discard; in `GlassPSMain` the reflection sample, the Fresnel reflection
   mix and the whole Forward+ lighting block; the `#if !defined(PLATFORM_ANDROID)` guard lines. The `if` around the
   global light loop becomes a bare scope. Compile with `slangc` after each group of deletions.
3. Rule 1, approved: in `prepareFrameUboTemplate()` the tutorial's line `scaleIBLAmbient = 1.0f;` with its first
   comment line.
4. Rule 1: `LightComponent` (`src/light_component.h/.cpp`, CMake entry), written in the style of `CameraComponent`:
   type (directional, point, spot), colour, intensity, range, cone angles. Position and direction come from the
   entity's `TransformComponent`.
5. Rule 1: where `Render()` builds the light list of the frame (about 698 to 712), the lights of all entities with a
   `LightComponent` are appended to the static glTF lights as `ExtractedLight`. `updateLightStorageBuffer()` stays
   unchanged; it already stores a directional light's direction in the position field and multiplies colour by
   intensity.
6. Rule 1: `SetupScene()` in `main.cpp` gets an entity "Sun" with a transform and a directional `LightComponent`. The
   intensity is tuned with the owner's run.

Check: the room is lit by the sun, has highlights and 10 % ambient on the sides facing away; without the "Sun" entity
the scene is dark except for the ambient part. Known consequences: `pbr.spv` is no longer byte-identical to the
tutorial's (the "kept on purpose" table in `DELETIONS.md` says so today), glass has no light highlights. Closes the
light part of `tr:szenenmanagement`.

Status: sub-steps 1 to 3 done on 2026-09-21. `pbr_full.slang` copied byte for byte over `pbr.slang`, then trimmed
from 635 to 351 lines in four cumulative stages that were each compiled with `slangc`; 3 forced edits (one bare
scope, two comments), every other line is a tutorial line; the compiled shader declares exactly set 0 bindings 0 to
6, set 1 binding 0 and the material push constants. The two `scaleIBLAmbient` lines are in
`prepareFrameUboTemplate()`. Debug build with 0 errors. Sub-steps 4 to 6 (`LightComponent`, the appended entity
lights, the "Sun") were proposed in the chat as diffs, test-compiled in a scratch folder with the project's compiler
settings, approved by the owner on 2026-09-21 and then written as proposed: `light_component.h/.cpp` with the CMake
entry, 11 lines plus an include in `Render()`, 16 lines plus an include in `SetupScene()` ("Sun": rotation -45°, 45°,
0°, intensity 3.0, both to be tuned with the owner's run). Configure and Debug build: 0 errors. The step is done;
the owner's run is pending. Log: `docs/DELETIONS.md`. Decided with this step: new files of the owner carry the
Apache-2.0 header with the owner's name, and the assistant builds after every change of steps 2 to 9
(`docs/WORKFLOW.md`).

## Step 4: named pipelines (Rule 1, M)

1. Description: a small struct (shader file, cull mode, depth test, blending) and enums, without Vulkan types in the
   header that students will see later.
2. `Renderer::CreatePipeline(name, description)`: patterned on the opaque pipeline of `createPBRPipeline()`, same
   vertex input, PBR pipeline layout, attachment formats. `"shaders/x.slang"` is mapped to `"shaders/x.spv"`. Depth
   compare is `LessOrEqual`, otherwise an object that is in two pipelines never shows the second one. Blending on
   means blending state as in the engine's blended pipeline, no depth writes, transparent pass.
3. Storage in creation order: name, description, pipeline. `recreateSwapChain()` rebuilds them from the descriptions
   next to `createPBRPipeline()`; `Cleanup()` destroys them before the device.
4. `AddToPipeline(name, entity)`: the entity remembers the pipelines it was added to. An unknown name is an error
   that lists the known names.
5. Job building in `Render()`: no pipeline means today's path (`"pbr"`); otherwise one job per pipeline; a pipeline
   with blending puts its jobs into the transparent list. Both draw loops take the pipeline from the job; layout,
   descriptor sets and push constants stay the PBR ones.
6. Temporary test in `main.cpp`: a pipeline from `template.slang`, the room added to it.

Check: the room appears flat and textured; resizing the window keeps it; removing the test lines gives the lit room
again. Closes `einfache_pipeline` and `pipeline_shader_automatisierung`.

Status: sub-steps 1 to 6 implemented on 2026-09-21 after the owner approved the complete diff (507 lines, shown in
the chat, test-compiled before in a scratch copy of the source tree). New `src/pipeline_settings.h`; `renderer.h`
+43 lines, `renderer_pipelines.cpp` +248, `renderer_rendering.cpp` +37 and one changed tutorial line (`sort` became
`stable_sort`, so that two blended jobs of one entity keep their pipeline order), `renderer_core.cpp` +3, `main.cpp`
+12 (the temporary test). Decisions of the owner: default cull mode none; the list of pipelines per entity lives in
the renderer (`entityPipelines`), not in `Entity`, so `entity.h` stays byte-identical to the tutorial. Differences to
the sub-steps above: `"pbr"` is not an entry of the pipeline list but a reserved name with the index -1, always first
in an entity's list; `AddToPipeline()` is guarded by a mutex because the temporary test (and any scene loading)
calls it from the loading thread. Open: the owner's run against the check above, then the test lines in `main.cpp`
are removed and the step moves to "Done".

Incident: the owner's first run of this step crashed at start-up ("A LIST_ENTRY has been corrupted"). The cause was
the build, not the code of the step: six object files that include the changed `renderer.h` had not been recompiled
(details and recovery in `docs/BUILD.md`, "Stale object files"). Debug and Release were rebuilt from scratch on
2026-09-21; an exact check of every object file against MSBuild's dependency log finds nothing stale. No source
file was changed for the fix. The owner ran the rebuilt exe on 2026-09-21: it works. The test lines in `main.cpp`
were removed again (the file is back to its state after step 3), Debug build with 0 errors. The step is done.

## Step 5: the sandbox layer (Rule 1, M to L)

1. To settle first: the class the students use cannot be a second global `Engine`, the tutorial's `Engine` has that
   name. Either a namespace or another name (for example `Sandbox`).
2. `src/sandbox.h`: no Vulkan include, no engine header; forward declarations and a hidden implementation. Classes:
   the engine wrapper, `SceneObject`, `Camera`, `Light`, the pipeline settings of step 4.
3. `LoadModel(name, file)`: calls `LoadGLTFModel()` directly instead of on a thread, with the loading flags set as
   `main.cpp` does today; the entities of the model are found by comparing the entity list before and after, so the
   tutorial's loader stays unchanged. A second model appends its glTF lights instead of replacing the list.
4. `CreateSphere(name, radius)`: entity, transform, `MeshComponent::CreateSphere()`, then
   `EnqueueEntityPreallocationBatch()` by the wrapper.
5. `CreateCamera()`, `SetActiveCamera()`, `CreateLight()` (step 3 component), `AddToPipeline(name, object)` for all
   parts, `Part(materialName)`, and on `SceneObject` the setters plus `Move`, `Rotate` (degrees), `Scale`.
6. `src/sandbox.cpp` with `SetupScene()` and `main()` takes the place of `src/main.cpp` (CMake line); the crash
   reporter start moves into the wrapper. For now `main()` still calls one `Initialize()` and `Run()`.

Check: the same picture as after step 3, produced by `sandbox.cpp`; a sphere next to the room; a second model keeps
the lights of the first. Closes the rest of `tr:szenenmanagement`.

Status: sub-steps 1 to 6 implemented on 2026-09-21 after the owner approved the complete diff (878 lines, shown in
the chat, test-compiled before in a scratch copy of the source tree; the compiler's include listing confirmed that
`sandbox.cpp` pulls in no Vulkan and no engine header). Decisions of the owner: the class is called `Sandbox` (a
second global `Engine` would clash with the tutorial's class), `src/main.cpp` is deleted, the sphere stays in the
example scene. New: `src/sandbox.h` (287 lines), `src/sandbox_impl.cpp` (468), `src/sandbox.cpp` (70); changed:
`renderer.h` +8 (`GetStaticLights()`, the one change to a tutorial file) and two lines of `CMakeLists.txt`. Beyond
the sub-steps: a failed `LoadModel()` and an unknown `Part()` return an empty object instead of `nullptr`, so that
the students' calls on it do nothing; `Run()` ends the engine's initial load cycle when no model was loaded (without
it a scene of only spheres would show the loading overlay forever); `Light::SetDirection()` turns a direction into
the rotation of the light's transform. Known limit, to be solved with step 8: `LoadModel()` blocks the main thread
before the loop, and the engine's watchdog aborts after 10 s without a frame, so a model that takes longer than that
to parse trips it. Configure, Debug and Release builds: 0 errors, 0 warnings; every object file checked against
MSBuild's dependency log, nothing stale. The position of the sphere (0.5, 0.3, -1.2) was
chosen without seeing the picture. The check "a second model keeps the lights of the first" cannot be seen with the
Viking room (it has no glTF lights); it needs a second model with a `KHR_lights_punctual` light. The owner ran the
step on 2026-09-22 and reported that everything works. The step is done.

## Step 6: terminal commands (Rule 1, S)

1. A reader thread on `std::cin`, a locked queue, a parser for `Name.Move(x, y, z)`, `Name.Rotate(x, y, z)` in
   degrees and `Name.Scale(x, y, z)`. Unknown names and malformed lines answer with one line of help.
2. The queue is drained once per frame on the main thread and skipped while `IsLoading()`. Until step 8 exists this
   needs one call inside `Engine::Update()`; step 8 moves it into `UpdateScene()`.
3. The reader thread is detached, a blocked `getline` must not keep the program alive at exit.

Check: `Room.Move(1, 0, 0)` moves the room in the next frame; `Sun.Rotate(...)` changes the lighting. Note: the
engine's info output in the same terminal is chatty; typing works but is not pretty.

Status: implemented on 2026-09-22 after the owner approved the complete diff (402 lines, shown in the chat,
test-compiled before with the project's compiler settings in a scratch copy of the source tree; a scratch program
checked 34 parser cases and that the process ends although the reader thread is blocked in `getline`, with a pipe
and with a console). New: `src/terminal_commands.h` (83 lines: `TerminalCommand`, `TerminalCommands` with `Start()`,
`TakeLines()`, `Parse()`) and `src/terminal_commands.cpp` (124); changed: `src/sandbox_impl.cpp` +142
(`TerminalCommandComponent`, `FindEntities()`, `PrintTerminalHelp()`, `ApplyTerminalCommands()`, three lines in
`Run()`) and one line of `CMakeLists.txt`. Decision of the owner (2026-09-22): the once-per-frame call of sub-step 2
is not a call inside `Engine::Update()` but a component on an entity of its own ("TerminalCommands"); the tutorial's
`Engine::Update()` already updates all components once per frame on the main thread and skips that while a model is
loading, so no tutorial file changed. Difference to the sub-step: lines typed while the loading overlay shows are
kept and applied when loading ends, not dropped. Beyond the sub-steps: lights are addressable too (`Move`, `Rotate`;
`Scale` on a light is refused because a zero scale would break its direction), cameras are not (the fly controls
overwrite the camera rotation every frame); a `;` may end a line; an empty line is ignored; the help line lists the
known names. Configure re-run (new source file), Debug and Release builds: 0 errors, 0 warnings; every object file
checked against MSBuild's dependency log, nothing stale. The owner ran it on 2026-09-22: everything works. The step
is done.

## Step 7: the initialisation chain (Rule 1, M)

1. `Engine::Initialize()` and `Renderer::Initialize()` are split along this mapping; inside a group the engine keeps
   today's internal order. Moving the depth image and the default textures in front of the command pool is safe: the
   helpers that record one-off commands (`transitionImageLayout()`, the copies) create their own temporary pool.

   | Call | Today's code |
   |---|---|
   | `CreateWindow` | platform, window, the four input callbacks |
   | `CreateInstance` | instance, debug messenger, surface |
   | `PickDevice` | physical device, logical device, memory pool |
   | `CreateSwapChain` | swap chain, image views |
   | `InitializeRendering` | dynamic rendering setup, depth image, off-screen colour image |
   | `CreatePipelines` | descriptor set layouts, PBR pipelines, composite pipeline, light buffers |
   | `CreateCommandBuffers` | command pool, descriptor pool, default textures, command buffers |
   | `CreateSyncObjects` | semaphores and fences, thread pool, uploads worker, watchdog, model loader, ImGui |

2. A set of "done" flags. Every call checks its prerequisites and reports the first missing one by name;
   `CreatePipeline()` requires `InitializeRendering`; `SetupScene` calls and the render loop require all eight.
3. `sandbox.cpp` switches from `Initialize()` to the eight calls.

Check: the normal run is unchanged; removing one call, or swapping two, gives the error that names the missing call.

Status: implemented on 2026-09-22 after the owner approved the complete diff (765 lines over seven files, shown in
the chat; test-compiled before with the project's compiler settings in a scratch copy of the source tree).
Sub-step 1: `Renderer::Initialize()` became the seven public calls `CreateInstance` to `CreateSyncObjects` in
`renderer_core.cpp` (+53 / -19 lines, `renderer.h` +45 / -4); a normalised comparison of the old body with the new
ones finds every tutorial line again (gone: the header and `LOGI("Renderer::Initialize start")`; new: seven headers,
six `return true;` with braces, comments). `Engine::Initialize()` became eight calls in `engine.cpp` (+42 / -3,
`engine.h` +52 / -3): `InitializeWindow` with the platform and callbacks, `CreateInstance` creating the renderer,
five forwarders, `CreateSyncObjects` with the model loader and ImGui; one changed tutorial line (ImGui gets the
window size from the platform instead of the former parameters). Sub-step 2: the flags and the check live in the
sandbox layer (`sandbox_impl.cpp` +149 / -34, `sandbox.h` +71 / -16): each call of the chain rejects a repeat and
reports the first call in front of it that is not done, the order is appended with the first error; `Run`,
`LoadModel`, `CreateSphere`, `AddToPipeline` need all eight, `CreatePipeline` needs `CreatePipelines`. Sub-step 3:
`main()` makes the eight calls (`sandbox.cpp` +10 / -5); `Sandbox::Initialize()`, `Engine::Initialize()` and
`Renderer::Initialize()` are deleted. Decisions of the owner (2026-09-22): the first call is `InitializeWindow`
(`CreateWindow` is a `windows.h` macro that `sandbox_impl.cpp` sees through `crash_reporter.h`; the definition and
the call in `sandbox.cpp` would be different symbols); `CreateCamera`, `CreateLight` and `SetActiveCamera` are not
guarded, because they need no Vulkan object and a guard would need null-safe handles; `Initialize()` is not kept as
a convenience. Deviation from sub-step 2: `CreatePipeline()` requires `CreatePipelines()` rather than
`InitializeRendering()`, because the PBR pipeline layout it copies is created there. Debug and Release builds:
0 errors, 0 warnings, the eleven files that include `engine.h` or `renderer.h` recompiled, every object file checked
against MSBuild's dependency log, nothing stale. The owner ran it on 2026-09-22: the normal run is unchanged, and
with `PickDevice()` and `CreateInstance()` swapped the log shows the first error naming `CreateInstance()` with the
order, `CreateInstance()` running normally afterwards, every later call naming `PickDevice()`, no watchdog, no window
loop, a clean exit with code 0. The step is done. Log: `docs/DELETIONS.md`.

## Step 8: the frame calls (Rule 1, L, the riskiest step)

1. 8a, refactor only: `Renderer::Render()` is cut into six member functions and still calls them in order itself.
   What was local to `Render()` (image index, job lists, the bound pipeline) becomes a frame context member. Today's
   early returns (swap chain out of date, resize) set a "frame skipped" flag that turns the remaining parts into
   no-ops. Mapping:

   | Call | Today's part of the frame (`ARCHITECTURE.md`, "One frame") |
   |---|---|
   | `BeginFrame` | light list and upload, UBO template, fence wait, pending uploads and preallocations, dirty descriptors, acquire, begin command buffer, texture jobs |
   | `UpdateScene` | `Engine::Update()` (camera controls, ImGui frame, terminal commands), entity collection, culling, LOD, job sorting, per-entity UBOs |
   | `BeginRendering` | transition of the off-screen image, begin of pass 1 with clear |
   | `DrawScene` | opaque draws, end of pass 1, composite pass, begin of pass 2, transparent draws |
   | `EndRendering` | end of pass 2, the "Renderer" panel, ImGui pass, transition to present |
   | `EndFrame` | submit, present, swap-chain recreation when out of date, next frame index |

   Check after 8a: the run is identical to before, including resize and the loading overlay.
2. 8b: the six calls become public on the wrapper with the order check; `IsRunning()` takes over event processing,
   delta time and the FPS title from `Engine::Run()`; the loop moves into `sandbox.cpp`.

Check: the normal run is unchanged; a removed or swapped call stops rendering with the error that names the expected
call. Closes `lifecycle`.

Status of 8a: implemented on 2026-09-22 after the owner approved the diff (515 lines over `renderer.h` and
`renderer_rendering.cpp`, shown in the chat, test-compiled before). `Render()` calls the six new private-by-use
public members in order; a `FrameInProgress frame` member holds what was local (`skipped`, `imageIndex`,
`acquireSemaphoreIndex`, the two job lists, the pass-1 viewport and scissor); every part but the first begins with
`if (frame.skipped) return;`. Every moved line is a tutorial line (checked with a whitespace-normalised,
order-insensitive comparison of the old body with the six new ones: only the duplicate `ImGui::EndFrame()` of the
two live skip paths, one `{` moved onto its own line and the two job-vector declarations changed). The cut differs
from the table above in four points, each for a reason: `begin()` of the command buffer, the texture jobs and the
"Renderer" panel are in `BeginRendering` (descriptor writes are deferred and the light-buffer rebind is skipped while
`isRecordingCmd` is set, so everything `UpdateScene` writes has to come before recording; the panel stays where it
was, before pass 1); the light list and the UBO template moved from the top of `Render()` into `UpdateScene`
(in 8b they have to follow `Engine::Update()`, or the camera position and the lights would lag one frame; the light
buffer of the slot is now written after the fence wait); the acquire moved in front of the preparation pass, the
only real reorder (it touches only the swap chain and its semaphore, and on out-of-date the frame is skipped before
any per-frame write); the transparent sort moved into `UpdateScene`. The two dead resize checks (after `begin()`,
before the submit) became skips as well, and the skipped frame always ends ImGui's frame in `EndFrame()`. Release
build: 0 errors, 0 warnings, ten files recompiled, nothing stale; the Debug link was blocked by the owner's running
exe (LNK1168) and succeeded on the retry. The owner ran 8a on 2026-09-22: "it looks fine".

Status of 8b: implemented on 2026-09-22 after the owner approved the diff (594 lines over seven files, shown in
the chat, test-compiled before). `Engine::Run()` became `Engine::IsRunning()` (the head of its loop: window events,
delta time, FPS title; false when the window closes) plus the six calls `BeginFrame()` to `EndFrame()`;
`Engine::UpdateScene()` is the tutorial's `Update()` followed by the prologue of the old `Engine::Render()` (its
guards, the entity-removal queue, the entity snapshot) and `renderer->UpdateScene()`, the other five forward to the
renderer; every line of `Run()` and `Render()` moved (checked as for 8a; only the `while` / `break` / `Render()`
scaffolding is gone). `Renderer::Render()` (both overloads), `Engine::Run()`, `Engine::Render()` and
`Sandbox::Run()` are deleted. The `Sandbox` has `IsRunning()` and the six frame calls; `sandbox_impl.cpp` keeps the
call that is expected next: a call out of order prints `Frame sequence error: <call>() was called, <expected>() was
expected. Rendering stopped.`, after which every frame call does nothing and `IsRunning()` is false, so the loop
ends and the program exits normally (`IsRunning()` itself requires the previous frame to be complete, which catches
a forgotten `EndFrame()`); an exception inside a frame call is printed (`Exception: ...`) and stops rendering, as
the old `Run()` did. The first `IsRunning()` does the one-time work of the old `Run()` (the chain check, the end of
the load cycle when no model was loaded, the terminal reader) and, beyond the plan and approved by the owner,
refuses to start without an active camera (the tutorial's `Render()` would return early every frame and ImGui would
assert in Debug). `main()` in `sandbox.cpp` is the eight initialization calls, `SetupScene()` and the loop
`while (IsRunning()) { the six calls }`. Consequence of the new order: `Engine::Update()` (camera controls, ImGui
`NewFrame`) runs after `BeginFrame()`'s acquire instead of before it, which is why 8a moved the light list and the
UBO template into `UpdateScene`. Debug and Release builds: 0 errors, 0 warnings, eleven files recompiled, nothing
stale. The owner checked 8b on 2026-09-22 and reported that everything works. Step 8 is done.

## Step 9: terminal output (Rule 1, S)

1. Printers for the initialisation chain, for one pipeline (stages, the four settings marked as the student's, its
   objects) and for the frame sequence, in the form shown in the roadmap.
2. Printed once at the first `IsRunning()`, so it is not buried under the start-up log, and again by the error paths
   of steps 7 and 8 with the failed call marked.

Check: start-up print and both error views. Closes `ascii_pipeline`.

Status: implemented on 2026-09-22 after the owner approved the diff (254 added / 16 removed lines over `sandbox.h`
and `sandbox_impl.cpp`, sent as a file, test-compiled before with the project's Debug flags in a scratch copy).
Sub-step 1: `PrintCallList()` (one list in the roadmap's form: `[ok]` for the calls that are done, `[!!]` for the
call the error is about with `<- missing` / `<- called twice` / `<- failed`, `[  ]` for the rest, every other line
with its description) and `PrintPipeline()` (the roadmap's block: the seven stages from the input assembly to the
attachments, the four settings marked `yours` on an own pipeline and everything `fixed` on the engine's `"pbr"`, the
objects line; the heading names the `.slang` file and the `.spv` it is compiled to), with description arrays next
to the existing name arrays. The `Sandbox` keeps a `PipelineView` per pipeline (`"pbr"` when `CreatePipelines()`
succeeds, one per `CreatePipeline()`, the objects from `AddToPipeline()`); an object that was added to no pipeline
is listed under `"pbr"`. Sub-step 2: `PrintOverview()` on stdout from the first `IsRunning()` after its one-time
work; `ReportInitializationChain()` on stderr with the first initialization error (a missing call, a call made
twice, a failed call), replacing the old one-line list of the order; `FrameSequenceError()` and the exception path
of `FrameCall()` print the frame view with the expected call marked missing or the failing call marked failed.
Decisions of the owner in the approval dialog: a part is named `Room.wood` (two changed lines in `Part()`, so
`GetName()` of a part changes too), and the box column is printed at start-up as well. The scratch exe of the
printers was blocked by Smart App Control (by reputation, in the build folder too), so the expected output was
derived from the fixed widths and shown to the owner as text. Debug and Release builds: 0 errors, 0 warnings, the
two files that include `sandbox.h` recompiled (`sandbox.cpp`, `sandbox_impl.cpp`), their objects newer than the
sources, nothing stale. The owner ran it on 2026-09-22: everything works. Step 9 is done.

Changed on 2026-10-04 (not a plan step; `docs/ROADMAP.md`, "Done"): the error texts of steps 7 and 8 and the views
of this step no longer name or list calls that the sandbox file has not made. `PrintCallList()` prints the done
calls and then the call that could not run with its reason (requirement arrays next to the description arrays);
the first initialization error stops the initialization; the frame sequence is printed after the first complete
frame. The checks of steps 7 and 8 ("the error that names the missing call") describe the state before.

## Step 10: README for students (documentation, S)

The shape of `sandbox.cpp`; the calls and their order; what a shader receives (the table of `ARCHITECTURE.md`) and the
template; a new shader file needs a CMake re-configure; PNG to KTX2 with `toktx` (the command is tried out first, no
KTX tools are installed on this machine today); the terminal commands.
