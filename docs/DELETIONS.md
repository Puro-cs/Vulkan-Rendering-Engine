# Deletion log

What was removed from the tutorial engine, and every edit that a deletion forced. Rule 2 in
`docs/CODE_CHANGE_RULES.md` requires each new removal to be appended here. The mechanical comparison
(byte-identical / differing / only-in-one-tree per file) is `src/source-file-difference.md`.

## Removed subsystems (state as of 2026-09-21)

| Subsystem | Deleted files | Code removed from remaining files |
|---|---|---|
| Physics | `physics_system.h/.cpp`, `shaders/physics.slang` | `engine.*`: PhysicsSystem member, getter, update, physics scaling, ball-throwing demo (`ThrowBall`, `ProcessPendingBalls`, `BallMaterial`, `PendingBall`, right-mouse handling, camera ball tracking); `scene_loading.cpp`: rigid-body creation and the "Physics" loading phase; `imgui_system.*`: ball debug controls; `renderer.h`: `LoadingPhase::Physics`, `SHARED_BRIGHT_RED_ID`; `renderer_rendering.cpp`, `renderer_ray_query.cpp`, `renderer_resources.cpp`: `Ball_` entity special cases and the red ball texture. |
| Audio | `audio_system.h/.cpp`, `shaders/hrtf.slang` | `engine.*`: AudioSystem member, getter, update; `imgui_system.*`: `SetAudioSystem`, audio members, "HRTF Audio Controls" window; `renderer.h`, `renderer_compute.cpp`: `DispatchCompute()` and the HRTF compute pipeline (compute descriptor and command pools stay, Forward+ uses them). |
| Android platform | `android/` (Gradle project), `shaders/pbr_android.slang` | `platform.*`: `AndroidPlatform`, accelerometer and display-rotation API, Android `LOGI` macros; `engine.*`: `InitializeAndroid`, `RunAndroid`, touch/tilt camera controls, mobile state; `main.cpp`: `android_main`; all `PLATFORM_ANDROID` branches in `renderer_core.cpp`, `renderer_utils.cpp`, `renderer_resources.cpp`, `renderer_pipelines.cpp` (`pbr_android.spv` fallback), `imgui_system.cpp`, `vulkan_compatibility.h`, `shaders/ray_query.slang`. |
| Direct-to-display platform | none | `platform.*`: `DirectDisplayPlatform` (`VK_KHR_display`); `renderer_core.cpp`: its instance-extension branch. |
| Linux / macOS | `install_dependencies_linux.sh`, `fetch_bistro_assets.sh` | `crash_reporter.h`, `renderdoc_debug_system.cpp`: `__linux__` and `__APPLE__` branches. |
| Animation | `animation_component.h/.cpp` | `model_loader.*`: `Animation*` structs, `ProcessAnimations()`, animated-node transform/mesh maps, `sourceMeshIndex`; `scene_loading.cpp`: animation controller set-up; `renderer.h`, `renderer_resources.cpp`: `EnqueueInstanceBufferRecreation()` / `recreateInstanceBuffer()` (only animation used them); `renderer_rendering.cpp`: `_AnimNode_` entity filters. |
| Course module (opacity micromaps) | `src/Courses/` | every `ENABLE_COURSE_OPACITY_MICROMAPS` block in `engine.*`, `renderer.h`, `renderer_core.cpp`, `renderer_resources.cpp`, `renderer_ray_query.cpp`, `scene_loading.cpp`; micromap provider callback, `GetThreadPool()` / `KickWatchdog()` accessors, `VK_KHR_opacity_micromap` feature detection in `renderer_core.cpp`; OMM fallback define in `vulkan_compatibility.h`; `ENABLE_COURSES` option in `CMakeLists.txt`. |
| Unused shaders | `shaders/lighting.slang`, `shaders/pbr_full.slang` | `renderer.h`, `renderer_core.cpp`, `renderer_pipelines.cpp`, `renderer_rendering.cpp`: `createLightingPipeline()` and `lightingPipeline*` members (created every start-up, never bound). `pbr_full.spv` was compiled but never loaded. |
| Build glue | `src/CMake/` (all `Find*.cmake`), `src/install_dependencies_windows.bat`, `src/fetch_bistro_assets.bat`, `src/CMakeLists.txt`, `src/CMakePresets.json` | Root `CMakeLists.txt` finds every dependency through the vcpkg CMake configs. The tutorial's `FindVulkanHpp.cmake` never detected the SDK version with CMake 3.21+ and always tried to clone Vulkan-Hpp from GitHub ("Build step for vulkanhppmain failed"). |
| Ray-query render mode (2026-09-20) | `shaders/ray_query.slang` (and its build output `shaders/ray_query.spv`) | `renderer.h`: `RayQueryUniformBufferObject` with its `static_assert`s, `RenderMode::RayQuery`, `ToggleRenderMode()`, `updateRayQueryDescriptorSets()`, `createRayQueryDescriptorSetLayout()` / `createRayQueryPipeline()` / `createRayQueryResources()`, the ray-query pipeline, pipeline layout, descriptor set layout, descriptor sets, written / dirty-mask tracking, per-frame UBO and output image members, `rqCompositeDescriptorSets`, `rqCompositeSampler`, and the option members behind the panel (`enableRayQueryShadows`, `rayQueryShadowSampleCount`, `rayQueryShadowSoftness`, `enableRayQueryReflections`, `enableRayQueryTransparency`, `rayQueryMaxBounces`, `enableThickGlass`, `thickGlassThicknessClamp`, `thickGlassAbsorptionScale`); `renderer_pipelines.cpp`: the three `createRayQuery*()` functions; `renderer_ray_query.cpp`: `updateRayQueryDescriptorSets()` and the dirty-mask write at the end of `buildAccelerationStructures()`; `renderer_core.cpp`: the three create calls in `Initialize()` and the matching resets in `Cleanup()`; `renderer_rendering.cpp`: in `Render()` the compute dispatch, the composite blit of the ray-query image and the magenta "TLAS not built" frame, the per-frame ray-query descriptor update, the static-only one-time AS request, `rayQueryRenderedThisFrame`, the ray-query branch of the texture streaming budget, the "Ray Query" combo entry with its mode-switch handler, the panel section "Ray Query Status / Ray Query Features / Thick Glass"; in `recreateSwapChain()` the ray-query descriptor / output-image handling; in `prepareFrameUboTemplate()` the two writes of the deleted option members; `renderer_resources.cpp`: the dirty-mask write in `OnTextureUploaded()`. |
| Dead panel controls (2026-09-21) | none | The "Rendering Mode" combo with the text "Rasterization only (ray query not supported)", `enum class RenderMode`, `SetRenderMode()`, `GetRenderMode()`, `currentRenderMode` (`renderer.h`, `renderer_rendering.cpp`); the "Reflection intensity" slider, `reflectionIntensity`, `SetReflectionIntensity()`, `GetReflectionIntensity()` and its UBO write (no shader reads the value); the "Gamma" slider only (`gamma`, `SetGamma()` and the non-sRGB branch of `composite.slang` stay, see below). The separator that followed the mode combo went with it. |
| Planar reflections (2026-09-21) | none | Never active: the render call and the checkbox were commented out upstream and `enablePlanarReflections` was forced to `false` every frame. `renderer_rendering.cpp`: `createReflectionResources()`, `destroyReflectionResources()`, `renderReflectionPass()`, `SetPlanarReflectionsEnabled()`, `TogglePlanarReflections()`, the calls in `cleanupSwapChain()` / `recreateSwapChain()`, the reflection fields in `prepareFrameUboTemplate()`, the reflection-pass override in `updateUniformBufferInternal()`, the safe-point block in `Render()`, the "Reflection resolution scale" slider with the commented-out checkbox, the commented-out pass; `renderer.h`: `ReflectionRT`, `reflections`, `enablePlanarReflections`, `reflectionResolutionScale`, `currentReflectionVP`, `currentReflectionPlane`, `reflectionVPs`, `sampleReflectionVP`, `reflectionResourcesDirty`, `pbrReflectionGraphicsPipeline`, `IsPlanarReflectionsEnabled()`; `renderer_pipelines.cpp`: PBR layout binding 10 and the reflection pipeline; `renderer_resources.cpp`: the binding 10 writes; `shaders/pbr.slang`: the unused `reflectionMap` declaration at binding 10. |
| Acceleration structures, the rest of ray query (2026-09-21) | `renderer_ray_query.cpp` (whole file: `buildAccelerationStructures()`, `refitTopLevelAS()`), its `CMakeLists.txt` entry | The option "RayQuery shadows (raster)" and `enableRasterRayQueryShadows` (never had a visible effect, `pbr.slang` declares no TLAS); `renderer.h`: `AccelerationStructure`, `blasStructures`, `tlasStructure`, `PendingASDelete`, `GeometryInfo`, `MaterialData`, the geometry / material buffers, the texture table (`rayQueryTex*`, `RQ_MAX_TEX`, `RQ_SLOT_*`, `RQMaterialTexPaths`), all `asBuild*` / `lastASBuilt*` / `asFrozen` / TLAS refit state, `RequestAccelerationStructureBuild()` (both overloads), the AS progress getters, `LoadingPhase::AccelerationStructures`, `GetRayQueryEnabled()`, `GetAccelerationStructureEnabled()`, `rayQueryEnabled`, `accelerationStructureEnabled`, `rayQueryStaticOnly` with its accessors, `MeshResources::materialIndex` (unused), the optional extensions `VK_KHR_ray_query`, `VK_KHR_acceleration_structure`, `VK_KHR_deferred_host_operations`; `renderer_core.cpp`: both feature structs in the query and enable chains, the AS teardown; `renderer_pipelines.cpp`: PBR layout bindings 11 to 13; `renderer_rendering.cpp`: the deferred AS deletion queue, the readiness scan and the build handling in `Render()` (about 290 lines), the TLAS refit, `noASPending`, three UBO-template writes; `renderer_resources.cpp`: the binding 11 to 13 writes (two sites), the AS usage flag on vertex / index buffers, the AS, storage-image and ray-query texture reservations in `createDescriptorPool()`, both "uploads completed" rebuild requests with `anyCompleted`; `imgui_system.cpp`: the AS phase of the loading overlay and the "Building acceleration structures..." window; `scene_loading.cpp`: the AS request (the tutorial's existing no-ray-query branch, jump to `Finalizing`, is what remains); `shaders/common_types.slang`: the `GeometryInfo` and `MaterialData` mirror structs. |
| Forward+ (2026-09-21) | `renderer_compute.cpp` (whole file: compute descriptor / command pools, Forward+ pipeline, tile buffers, params, dispatch), `shaders/forward_plus_cull.slang` (and its build output), the `CMakeLists.txt` entry | The option "Forward+ (tiled light culling)" and `useForwardPlus` (on by default). Its only real effect was the depth pre-pass; no shader read the tile lists. `renderer.h`: tile size / slice settings, `MAX_LIGHTS_PER_TILE`, `TileHeader`, `ForwardPlusPerFrame`, `forwardPlusPerFrame`, the Forward+ pipeline / layout / set layout, `depthPrepassPipeline`, `pbrPrepassGraphicsPipeline`, `computeDescriptorPool`, `computeCommandPool`, `pbrFixedBindingsWritten`, eight declarations; `renderer_core.cpp`: compute pool, Forward+ and depth pre-pass creation in `Initialize()`, their teardown in `Cleanup()`; `renderer_pipelines.cpp`: PBR layout bindings 7 to 9, the "after pre-pass" PBR pipeline (depth `Equal`, no writes), `createDepthPrepassPipeline()`; `renderer_rendering.cpp`: the depth pre-pass and the compute dispatch in the raster pass (about 95 lines), the per-frame tile buffer resize, the Forward+ rebuild in `recreateSwapChain()`, the checkbox with its lazy creation, two UBO-template writes; `renderer_resources.cpp`: the dummy tile buffers and binding 7 / 8 writes in `createDescriptorSets()`, the compute refresh in `createOrResizeLightStorageBuffers()`, the never-called `refreshPBRForwardPlusBindingsForFrame()`, and the whole "fixed bindings" mechanism of `updateDescriptorSetsForFrame()` (`appendPbrFixedWrites`, `needFixedWrites`), which only served bindings 7 to 13; `shaders/common_types.slang`: `TileHeader`. |

### Ray-query render mode: what stayed on 2026-09-20, and where it went

The removal of 2026-09-20 deleted the render mode, not the acceleration-structure subsystem, because the owner wanted
the Rasterization panel untouched. On 2026-09-21 the owner named the use case (students without Vulkan experience
write simple shaders) and asked for every control without effect to go, machinery included. Third column: state now.

| Kept on 2026-09-20 | Reason then | 2026-09-21 |
|---|---|---|
| `buildAccelerationStructures()`, `refitTopLevelAS()`, deferred AS deletion, AS build progress UI, `LoadingPhase::AccelerationStructures`, the AS request in `scene_loading.cpp`, `rayQueryEnabled` / `accelerationStructureEnabled` and the three device extensions | The Rasterization option "RayQuery shadows (raster)" is only shown when both features are on, and it drives the per-frame TLAS refit. The raster PBR descriptor set layout carries binding 11 (TLAS), 12 (geometry info) and 13 (material data) whenever AS is enabled, and they are rewritten after every AS build. `src/renderer_ray_query.cpp` therefore stays in `CMakeLists.txt`. | removed |
| `geometryInfoBuffer`, `materialBuffer`, `GeometryInfo`, `MaterialData`, the texture table bookkeeping (`rayQueryTex*`, `RQ_MAX_TEX`, `RQ_SLOT_*`, `rqMaterialTexPaths`) | Filled inside `buildAccelerationStructures()`, which was left untouched apart from the one forced cut listed below. The two buffers are bound to the raster PBR sets (bindings 12 / 13). | removed |
| `shaders/composite.slang`, `createCompositePipeline()` | Not ray-query only: the raster path uses it to draw the off-screen opaque colour to the swap chain before the transparent pass. | stays, in use |
| `UniformBufferObject` fields `enableRayQueryReflections`, `enableRayQueryTransparency`, `geometryInfoCount`, `materialCount`, `_rqReservedWorldPos` (C++ and `common_types.slang`) | Part of the UBO layout every raster shader is compiled against. Deleting them would change `pbr.spv` and `texturedMesh.spv`. The first two now keep their default of 1; no raster shader reads them. | stays (layout only, see next section) |
| `createDescriptorPool()` unchanged | Shared pool. It still reserves `MAX_FRAMES_IN_FLIGHT * RQ_MAX_TEX` combined image samplers and a storage-image pool size for the deleted descriptor sets: unused capacity, no effect on rasterization. | the three ray-query reservations are removed |
| `rayQueryStaticOnly`, `SetRayQueryStaticOnly()`, `IsRayQueryStaticOnly()` | Read by the shared TLAS refit gate and the AS build log in `Render()`. Always false. | removed |
| `RenderMode` (only `Rasterization` left), `SetRenderMode()`, `GetRenderMode()`, `currentRenderMode`, the "Mode" combo with one entry | Owner request: every control of the Rasterization panel stays visible. | removed |

Stale comments left on 2026-09-20: those in `renderer_ray_query.cpp`, `common_types.slang` line 108 and
`createDescriptorPool()` went with their code on 2026-09-21. Still there, in files that are otherwise tutorial
verbatim: the header line "Shared between rasterization and ray query shaders" of `lighting_utils`, `pbr_utils`,
`tonemapping_utils` (and the similar first comment of `common_types.slang`), and "Bindings 6, 7, 8 (SSBOs) REMOVED"
in `pbr.slang` (an upstream comment).

Behaviour that follows from the removal: the engine now starts in Rasterization (the tutorial default was Ray Query
whenever the GPU supported it), so after loading, non-critical textures stream with the raster budget (one job every
third frame) instead of the ray-query budget (32 jobs per frame). The raster code itself is unchanged.

Verification on 2026-09-20: Debug build with 0 errors and 0 warnings; the five remaining `.spv` files are
byte-identical (SHA-256) to the binaries built before the change, so the GPU code of the raster path did not change.
Not verified by the assistant (see `docs/WORKFLOW.md`, the owner runs the exe): the panel at run time and the
validation output. The owner ran it on 2026-09-21 and sent a screenshot: the panel was as expected.

### Removals of 2026-09-21: what stayed, and why

| Kept | Reason |
|---|---|
| Every field of `UniformBufferObject`, in `renderer.h` and `common_types.slang` (`padding1`, `padding2`, `slicesZ`, the reflection and ray-query fields, ...) | The struct is the layout all shaders are compiled against. Left alone, the four `.spv` files stay byte-identical to the tutorial's. Nobody writes the orphaned fields any more (they are 0, or their default of 1). Slimming the UBO is a separate candidate because it changes `pbr.spv` and `texturedMesh.spv`. |
| The light storage buffer, `updateLightStorageBuffer()`, `lightCount`, PBR layout binding 6 | Independent of Forward+. No shader reads it yet; it is the hook for a student shader that loops over the scene lights. The PBR set layout is now bindings 0 to 6 (UBO, five textures, lights). |
| `eShaderDeviceAddress` on vertex / index buffers and the required `bufferDeviceAddress` feature | Only ray tracing needed the addresses, but the flag selects the dedicated-memory-block branch of `MemoryPool::createBuffer()`. Dropping it would move every mesh buffer to the pooled branch, a behaviour change that cannot be tested broadly here. Only the AS usage flag was removed (its extension is gone). |
| `storageBufferDescriptors = ... * 6u` in `createDescriptorPool()` | Sized for six storage buffers per PBR set; one is left. Unused pool capacity, number not rewritten. |
| `gamma`, `SetGamma()`, its UBO write and push constant, the `outputIsSRGB == 0` branch of `composite.slang` | Only the slider went. The branch is live code on a swap chain that is not sRGB; on sRGB formats (every desktop GPU here) the hardware encodes and the value is ignored. |
| `RenderJob::isAlphaMasked` | Still filled in the preparation pass; its two readers (pre-pass skip, pipeline choice) are gone. |
| `computeQueue` with its accessors | Part of queue-family setup, not of Forward+. |
| `updateUniformBuffer(..., const glm::mat4& customTransform)` | Has no caller, but had none in the tutorial either. |

Behaviour that follows from these removals:

- No depth pre-pass: opaque PBR objects are always drawn with `pbrGraphicsPipeline` (depth `Less`, depth writes on).
  That is the tutorial's own path for "Forward+ unticked". A student who moves vertices in `texturedMesh.slang` or
  `pbr.slang` is no longer depth-tested against a pre-pass that was drawn with the unmodified `pbr.slang` vertex shader.
- Loading no longer has an "Acceleration Structures" phase; the loader goes from the scene phase to `Finalizing`
  through the branch the tutorial uses on GPUs without ray query. The three ray-tracing extensions are not requested.
- The panel shows only controls with a visible effect: basic / PBR lighting, frustum culling, distance LOD with two
  thresholds, sampler anisotropy, culling statistics, exposure.

Verification on 2026-09-21: Release build after each of the four stages, Debug build at the end, 0 errors and 0
warnings; the four remaining `.spv` files were force-recompiled (both `pbr.slang` and `common_types.slang` changed) and
are byte-identical to the binaries from before 2026-09-20 and to the tutorial's. Not verified by the assistant: run
time (panel, loading overlay, validation output). The owner ran it on 2026-09-21 and sent a screenshot: the panel
shows exactly the three groups described above, the scene renders in PBR mode, and the loading overlay was dismissed
(the panel is only drawn once loading has finished, so the path without the acceleration-structure phase works).
A Debug run of the owner on 2026-09-21 (31 s, four lighting toggles, one window resize with swap-chain recreation,
stdout and stderr captured to `build/run_stdout.txt` / `build/run_stderr.txt`) closed the last open point: the only
validation message is the inherited `robustBufferAccess2` one, the `Int64` message of `ray_query.spv` is gone, no new
message appeared. The log shows no acceleration-structure, Forward+, compute or ray-query line, the device is created
without the three ray-tracing extensions although the GPU (RTX 2070) supports them, and shutdown is clean (no crash
dump, watchdog silent).

## Forced edits (code that had to change because of a deletion)

| Date | File | Edit | Why |
|---|---|---|---|
| before 2026-09-13 | six comments across `engine.*`, `scene_loading.cpp`, `renderer_*.cpp` | words "physics", "audio", "Android" removed from comments | comment referred to deleted code |
| before 2026-09-13 | two blocks | re-indented | enclosing `#if` disappeared |
| before 2026-09-13 | `renderer_resources.cpp` | the scope draining the instance-recreation queue lost that queue from one declaration line and one condition | queue removed with Animation |
| before 2026-09-13 | `CMakeLists.txt` | vcpkg config lookups; `VS_STARTUP_PROJECT`; `VS_DEBUGGER_WORKING_DIRECTORY = src/` | find modules deleted |
| 2026-09-13 | `main.cpp` | `SetupScene()` loads `../assets/viking_room/viking_room.gltf` with a -90° X rotation instead of the Bistro scene | different demo scene |
| 2026-09-20 | `renderer.h` | default of `currentRenderMode`: `RenderMode::RayQuery` became `RenderMode::Rasterization` | the old default value no longer exists. **Replaced token**, not a deletion |
| 2026-09-20 | `renderer_rendering.cpp`, mode combo | `{"Rasterization", "Ray Query"}` lost its second entry; `int currentMode = (currentRenderMode == RenderMode::RayQuery) ? 1 : 0;` became `int currentMode = 0;`; item count `2` became `1`; the body of `if (ImGui::Combo(...))` (mode switch, AS request, descriptor refresh) was deleted and is now empty | one mode left. The count is a **replaced token**: `2` would read past the one-element array |
| 2026-09-20 | `renderer_rendering.cpp`, TLAS refit | `needTLAS = (currentRenderMode == RenderMode::RayQuery \|\| enableRasterRayQueryShadows) && ...` lost its first operand | enum value deleted; in Rasterization the expression evaluated to the same value before |
| 2026-09-20 | `renderer_rendering.cpp`, three scopes | `if (!rayQueryRenderedThisFrame) {` became `{` and its closing `} // skip rasterization when ray query has rendered` became `}` (raster pass); `} else {` became `{` (raster texture-streaming throttle) | flag and condition deleted. Bare scopes keep every line of the bodies byte-identical; re-indenting would have touched about 390 raster lines |
| 2026-09-20 | 11 comments: `renderer.h` (4), `renderer_core.cpp` (1), `renderer_rendering.cpp` (6) | words about the ray-query mode trimmed, e.g. "Ray query rendering mode control" became "Rendering mode control", "SHARED OPTIONS (BOTH MODES)" became "SHARED OPTIONS" | comment referred to deleted code |
| 2026-09-21 | `renderer_rendering.cpp`, panel | `if (currentRenderMode == RenderMode::Rasterization) {` around the raster options became `{` | `RenderMode` deleted; the bare scope keeps the lines of the option block unchanged |
| 2026-09-21 | `renderer_rendering.cpp`, opaque pass | `depthAttachment.loadOp = (didOpaqueDepthPrepass) ? eLoad : eClear;` became `... = vk::AttachmentLoadOp::eClear;`; the `if (job.isAlphaMasked) { ... } else { ... }` that picked between `pbrGraphicsPipeline` and the after-pre-pass pipeline collapsed to its first branch, `selectedPipeline = &pbrGraphicsPipeline; // writes depth, compare Less`, **re-indented** by one level | depth pre-pass deleted; both branches would have selected the same pipeline |
| 2026-09-21 | `renderer_rendering.cpp`, loading check | `noASPending` dropped from the condition that ends the loading overlay | flag deleted with the AS build |
| 2026-09-21 | `renderer_resources.cpp`, `createDescriptorPool()` | pool size array `5` became `3` (**replaced token**); `textureDescriptors + rqTexDescriptors` lost its second operand | acceleration-structure and storage-image pool sizes deleted. The AS entry had to go: its descriptor type belongs to an extension that is no longer enabled |
| 2026-09-21 | `renderer_resources.cpp`, mesh buffers | `eShaderDeviceAddress \| eAccelerationStructureBuildInputReadOnlyKHR` lost its second flag (twice) | same reason: usage flag of a disabled extension |
| 2026-09-21 | `renderer_resources.cpp`, uploads | `IsLoading() \|\| asBuildRequested.load(...)` became `IsLoading()` in `flushUploadsNow` and `forceSynchronous` | flag deleted |
| 2026-09-21 | `renderer_resources.cpp`, two scopes | `} {` after the binding 11 to 13 block and `appendPbrFixedWrites(writes); {` became `{` | the code in front of the lock scope was deleted |
| 2026-09-21 | `renderer_core.cpp`, feature query | the template list of `getFeatures2<...>` now ends at `vk::PhysicalDeviceShaderTileImageFeaturesEXT>();` | the two ray-tracing feature structs behind it were deleted |
| 2026-09-21 | `imgui_system.cpp` | `yBase = 10.0f + (showASBuild ? (90.0f + 10.0f) : 0.0f)` became `yBase = 10.0f` | AS status window deleted |
| 2026-09-21 | `scene_loading.cpp` | `} else {` of the AS request became `{`; its comment lost "No acceleration structure build needed;" | the `if` branch (AS request) was deleted, the tutorial's else branch is what runs |
| 2026-09-21 | 13 comments: `renderer.h` (4), `renderer_core.cpp` (4), `renderer_resources.cpp` (3), `imgui_system.cpp` (1), `scene_loading.cpp` (1) | words about AS builds, Forward+, ray tracing or reflection render targets trimmed, e.g. "(AS build, descriptor cold-init, etc.)" became "(descriptor cold-init, etc.)" | comment referred to deleted code |

## Added code (the only exception to "deletions only")

| Date | File | What | Why | Approved |
|---|---|---|---|---|
| 2026-09-13 | `src/imgui_system.cpp` | `static ImGuiKey GlfwKeyToImGuiKey(int key)` (line ~402) and modifier-key syncing in `HandleKeyboard()` | tutorial passes raw GLFW key codes to `ImGuiIO::AddKeyEvent()`; Dear ImGui 1.92 asserts (`IsNamedKeyOrMod`), first key press aborted Debug builds. Mapping mirrors `ImGui_ImplGlfw_KeyToImGuiKey()` of the official GLFW backend. | yes (owner) |

## How to append

Removal: add a row to the first table (or extend an existing one), then a row per forced edit in the second table.
Addition after owner approval: a row in the third table with the approval noted. Then update `src/source-file-difference.md`.
