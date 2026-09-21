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
| Dead `Pipeline` class (2026-09-21, planned change 1) | `pipeline.h` (230 lines), `pipeline.cpp` (712 lines); both were byte-identical to the tutorial | `CMakeLists.txt`: the source-list entry `src/pipeline.cpp`. Nothing else: `pipeline.cpp` was the only file that included `pipeline.h`, and no code ever created a `Pipeline`. The class was a standalone duplicate of the renderer's pipeline set-up (basic, PBR and lighting pipeline, loading `texturedMesh.spv`, `pbr.spv` and `lighting.spv`, whose shader left the port long ago); `Renderer` builds its own pipelines in `renderer_pipelines.cpp`. No forced edit. |
| Basic lighting path, the option "Use Basic Lighting (Phong)" (2026-09-21, planned change 2) | `shaders/texturedMesh.slang` (and its build output `texturedMesh.spv`, next to the source and in the build tree) | `renderer_rendering.cpp`: the panel group "Rasterization Options" (checkbox, console line, four status lines) with the separator below it; in the opaque draw loop `useBasic` with the basic pipeline choice, the basic descriptor set choice and the basic bind call; in the per-frame descriptor cold-init the creation of the basic sets and the basic UBO / image initialisation; in `cleanupSwapChain()` the resets of `graphicsPipeline` and `pipelineLayout`; in `recreateSwapChain()` the three basic clears, the `createGraphicsPipeline()` call and the re-creation of the basic sets. `renderer_pipelines.cpp`: `createDescriptorSetLayout()` (bindings 0 and 1) and `createGraphicsPipeline()` (it loaded `texturedMesh.spv`). `renderer_core.cpp`: both create calls in `Initialize()` with their log lines, four resets in `Cleanup()`. `renderer_resources.cpp`: the basic branch of `createDescriptorSets()` with `resolvedTexturePath` (only that branch read it), the basic branch of `updateDescriptorSetsForFrame()` with `newlyAllocated` (same), the basic flag initialisation at three sites and the two size checks, the basic set creation in `preAllocateEntityResources()` and `preAllocateEntityResourcesBatch()`, the basic refresh in `ProcessDirtyDescriptorsForFrame()`. `renderer.h`: `pipelineLayout`, `graphicsPipeline`, `descriptorSetLayout`, `EntityResources::basicDescriptorSets` / `basicUboBindingWritten` / `basicImagesWritten`, the declarations `createDescriptorSetLayout()` and `createGraphicsPipeline()`. `imgui_system.h`: `pbrEnabled`, `IsPBREnabled()`, `SetPBREnabled()`. |

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

### Dead `Pipeline` class (2026-09-21, planned change 1)

First step of `docs/IMPLEMENTATION_PLAN.md`. Reason for the removal: the class name collides with the named pipelines
of planned change 4, and the class was the only other code that names `texturedMesh.spv`, which goes with planned
change 2. Not part of the step, still undecided: `descriptor_manager.*`, `renderdoc_debug_system.*` and
`resource_manager.*` stay. No forced edit was needed.

Verification on 2026-09-21: Debug build with 0 errors and 0 compiler warnings; the regenerated project no longer lists
`pipeline.cpp`, and the exe links without it. No shader source changed and no shader was recompiled. The build prints
one MSBuild warning, `MSB8028`, which has nothing to do with the removal: the intermediate directory
`build/windows-msvc/VulkanRenderEngine.dir/Debug/` still holds a `.tlog` folder of the project under its old path
`C:\Dev\VulkanRenderEngine` (the repository folder was renamed). Not verified by the assistant: run time. The deleted
code was never called, so the run is expected to be unchanged; the owner's run is pending.

### Basic lighting path (2026-09-21, planned change 2): what stayed, and why

Second step of `docs/IMPLEMENTATION_PLAN.md`. Reason for the removal (owner): the option is only a second shader
underneath, and Phong shading is something the students implement themselves. `texturedMesh.slang` went completely,
together with everything that loads it, so that nothing is left that could fail at run time. Every object is drawn
with `pbr.slang`. The template shader for students that belongs to the same step is new code (Rule 1) and is listed
under "Added code" once the owner has approved it.

| Kept | Reason |
|---|---|
| The `usePBR` parameter of both `createDescriptorSets()` overloads (default `false`) and of both `updateDescriptorSetsForFrame()` overloads, `PendingDescOp::usePBR`, the `if (usePBR)` around the PBR branch in both functions | Every caller passes `true` now. Removing the parameter would change four signatures and ten call sites, which the removal does not need. Called with `false`, both functions would now do nothing and return `true`; no caller does that. |
| The `texturePath` argument of the same functions, and the code that computes it at the call sites (`texturePath`, `texPath`, `basicTexPath`, each with its base-colour fallback) | Only the basic branch used the path; the PBR branch takes its five texture paths from the `MeshComponent`. The argument is part of the signatures, so the callers still have to pass something. |
| `maxDescriptorSets = MAX_FRAMES_IN_FLIGHT * maxEntities * 2; // 2 pipeline types per entity` in `createDescriptorPool()`, with the three comment lines about the basic pipeline above it | Unused pool capacity, number not rewritten, as with the earlier removals. The comments explain the numbers that stayed, so they stayed too. |

Behaviour that follows from the removal:

- The panel has two groups, "Culling & LOD" and "Tone Mapping & Tuning". Every opaque object is drawn with
  `pbrGraphicsPipeline`, which was the default before.
- Each entity allocates `MAX_FRAMES_IN_FLIGHT` descriptor sets instead of twice that number.
- Start-up no longer reads `shaders/texturedMesh.spv`. The console loses the lines "Creating descriptor set
  layout...", "Descriptor set layout created successfully", "Creating graphics pipeline...", "Graphics pipeline
  created successfully" and "Creating main graphics pipeline with depth format: ...".
- In the descriptor cold-init of `Render()` the inner `if` of each of the three blocks now repeats the outer
  condition. Both are tutorial lines; the outer one lost its basic operand (forced edit below).

Verification on 2026-09-21: `cmake --preset windows-msvc` re-run (the shader list is a configure-time glob), then a
Debug build with 0 errors and 0 compiler warnings (the `MSB8028` warning described above is still printed). No shader
source changed: `composite.spv`, `imgui.spv` and `pbr.spv` are byte-identical (SHA-256) to the binaries from before
the step, and `texturedMesh.spv` was not created again. A search of `src/` finds none of the removed names any more.
Not verified by the assistant: run time (the panel, the picture, the validation output); the owner's run is pending.

### Real lighting (2026-09-21, planned change 3): `pbr.slang` from the tutorial's `pbr_full.slang`

Third step of `docs/IMPLEMENTATION_PLAN.md`. Rule 3: the tutorial's `shaders/pbr_full.slang` (635 lines, pinned commit
`6dd48b5`) was copied byte for byte over `src/shaders/pbr.slang`; the file keeps the name the engine loads, so no C++
changed for it. Until then the port carried the tutorial's stripped 109-line `pbr.slang` (fixed light direction, no
BRDF); `pbr_full.slang` is the shader the tutorial itself loaded as `pbr.slang` until its commit `ccc8dd8`. Rule 2:
the copy was then trimmed by deletion, following the owner's table in `docs/ROADMAP.md` (keep what the rasterization
pipeline uses, drop what belongs to a removed rendering option or needs ray tracing). Result: 351 lines, of which 3
are forced edits (below) and all others are tutorial lines. Line numbers are those of the tutorial file:

| Dropped from `pbr_full.slang` | Lines | Reason |
|---|---|---|
| Bindings 7 and 8 (Forward+ tile lists), 10 (`reflectionMap`), 11 (TLAS), 12 and 13 (ray-query shared buffers), with their comments and the two `#if !defined(PLATFORM_ANDROID)` / `#endif` pairs around them | 58, 60-74 | Those bindings left the PBR set layout on 2026-09-21. Binding 6 (`lightBuffer`), which sat inside the first guard, stays. |
| `RASTER_SHADOW_EPS` and `traceShadowOccluded()` | 78-116 | Ray-traced shadows need the acceleration structures and the ray-tracing extensions, removed on 2026-09-21. |
| `PSMain`: the Forward+ tile and depth-slice computation, `useForwardPlus`, `base` / `count`, `forceGlobal`, and the Forward+ light loop | 210-246, 248-320 | The option "Forward+ (tiled light culling)" and its compute pass were removed on 2026-09-21. `ubo.padding1`, `nearZ`, `farZ`, `slicesZ` are no longer read. |
| `PSMain`: four comment lines about the fall-back condition of the global loop, and the first comment line inside it | 322-325, 327 | They only describe the deleted condition. |
| `PSMain`: the shadow test inside the global light loop (`ubo.padding2`, `traceShadowOccluded()`) | 368-373 | Same reason as `traceShadowOccluded()`. |
| `PSMain`: the `#endif` of the guard opened at 210 | 389 | Android was removed from the port; the guarded content stays. |
| `PSMain`: the clip-plane discard of the reflection pass, and the note that planar reflections are only applied in the glass path | 398-406 | Planar reflections were removed on 2026-09-21; C++ no longer writes `reflectionPass` / `clipPlaneWS`. |
| `GlassPSMain`: the planar reflection sample (`refl`, `ubo.reflectionEnabled`, `ubo.reflectionVP`, `reflectionMap`) | 477-487 | Same reason. |
| `GlassPSMain`: the Fresnel reflection mix (`F_view2`, `F_avg2`, `reflStrength`, `ubo.reflectionIntensity`) and the comment line that refers to it | 515-522, 524 | It mixed in the reflection sample; the "Reflection intensity" slider was removed on 2026-09-21. |
| `GlassPSMain`: the whole Forward+ lighting block for glass with its guard | 540-617 | Light highlights on glass only exist as a tile loop. Known consequence: glass gets no light highlights. |

Kept on purpose, although nothing reads them any more (tutorial lines of the kept light loop, left so that its body
stays as close to the tutorial as possible): `float distToLight = 10000.0;` with `distToLight = d;` (only the shadow
test read it) and `float visibility = 1.0;` (now a constant factor of 1). Unused in the tutorial file already and left
alone: `import lighting_utils;` and `float3 G` in `PSMain`.

What the shader needs is exactly what the engine provides: the compiled `pbr.spv` declares set 0 bindings 0 to 6,
set 1 binding 0, the material push constants (last member at offset 112, as in `MaterialProperties`), the entry
points `VSMain`, `PSMain` and `GlassPSMain`, and only the `Shader` capability. The UBO fields it reads are `model`,
`view`, `proj`, `camPos`, `lightCount`, `scaleIBLAmbient`, `screenDimensions`, `exposure`, `gamma` and `padding0`.

Behaviour that follows:

- The scene lights are read by a shader again (binding 6, `ubo.lightCount`): directional, point, spot and emissive
  lights with GGX specular plus diffuse. A scene without lights shows only the ambient term, 10 % of the surface
  colour, plus emissive surfaces. The Viking room glTF has no lights, so until the light entity of this step exists
  the room is dark.
- `pbr.spv` is no longer byte-identical to the tutorial's `pbr.spv` (it cannot be: it is a different shader), and it
  is not byte-identical to the tutorial's `pbr_full.spv` either, because of the trim. `composite.spv` and `imgui.spv`
  are unchanged. The argument of the table "Removals of 2026-09-21: what stayed" for leaving `UniformBufferObject`
  untouched now only holds for those two.
- The light storage buffer and binding 6, kept on 2026-09-21 as "the hook for a student shader", are in use.

Verification on 2026-09-21: the trim was built up in four cumulative stages in a scratch folder (uses in `PSMain`,
uses in `GlassPSMain`, the shadow helper, the bindings), each compiled with `slangc` and the engine's flags; the file
in the repository is identical to the last stage. A normalised diff against the tutorial file shows exactly three
lines that are not tutorial lines, the forced edits. `spirv-val` passes. Debug build with 0 errors and 0 compiler
warnings (the `MSB8028` warning is still printed); `composite.spv` and `imgui.spv` are byte-identical to before. Not
verified by the assistant: the picture and the validation output; the owner's run is pending.

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
| 2026-09-21, planned change 2 | `renderer_rendering.cpp`, opaque draw loop | `} else {` became `{` twice (after the deleted basic pipeline choice and after the deleted basic bind call); `auto* descSetsPtr = useBasic ? &job.entityRes->basicDescriptorSets : &job.entityRes->pbrDescriptorSets;` became `auto* descSetsPtr = &job.entityRes->pbrDescriptorSets;` | `useBasic` and the basic sets deleted. The bare scopes keep the PBR lines byte-identical |
| 2026-09-21, planned change 2 | `renderer_rendering.cpp`, descriptor cold-init in `Render()` | three conditions lost their basic operand: `entityRes.basicDescriptorSets.empty() \|\| `, ` \|\| !entityRes.basicUboBindingWritten[currentFrame]`, ` \|\| !entityRes.basicImagesWritten[currentFrame]` | members deleted |
| 2026-09-21, planned change 2 | `renderer_resources.cpp`, `createDescriptorSets()` and `updateDescriptorSetsForFrame()` | `usePBR ? *pbrDescriptorSetLayout : *descriptorSetLayout` became `*pbrDescriptorSetLayout`, and `usePBR ? res.pbrDescriptorSets : res.basicDescriptorSets` became `res.pbrDescriptorSets` (each once per function) | the second operand named a deleted member |
| 2026-09-21, planned change 2 | 4 comments: `renderer.h` (1), `renderer_rendering.cpp` (1), `renderer_resources.cpp` (2) | words about the basic pipeline trimmed: ", Basic: b1", "for basic pipeline", "BOTH basic and", "for the basic pipeline" | comment referred to deleted code |
| 2026-09-21, planned change 3 | `shaders/pbr.slang`, `PSMain` (tutorial `pbr_full.slang` line 326) | `if (forceGlobal \|\| !useForwardPlus \|\| (count == 0 && ubo.lightCount > 0)) {` became `{` | every name in the condition belongs to Forward+. The bare scope keeps the body of the global light loop unchanged |
| 2026-09-21, planned change 3 | 2 comments in `shaders/pbr.slang` (tutorial lines 321 and 489) | "// Global light loop (fallback or forced debug)" lost "(fallback or forced debug)"; "// glass body + rim highlight, then add planar reflection contribution." became "// glass body + rim highlight." | comment referred to deleted code |

## Added code (the only exception to "deletions only")

| Date | File | What | Why | Approved |
|---|---|---|---|---|
| 2026-09-13 | `src/imgui_system.cpp` | `static ImGuiKey GlfwKeyToImGuiKey(int key)` (line ~402) and modifier-key syncing in `HandleKeyboard()` | tutorial passes raw GLFW key codes to `ImGuiIO::AddKeyEvent()`; Dear ImGui 1.92 asserts (`IsNamedKeyOrMod`), first key press aborted Debug builds. Mapping mirrors `ImGui_ImplGlfw_KeyToImGuiKey()` of the official GLFW backend. | yes (owner) |
| 2026-09-21, planned change 3 | `src/renderer_rendering.cpp`, `prepareFrameUboTemplate()` | two lines after the `gamma` write: the comment `// Match raster convention: ambient scale factor for simple IBL/ambient term.` and `frameUboTemplate.scaleIBLAmbient = 1.0f;` | The ambient term of `pbr.slang` is `albedo * ao * 0.1 * ubo.scaleIBLAmbient`. The tutorial only ever wrote that field in its ray-query UBO update (`renderer_rendering.cpp:1884-1886`), which the port removed, so the raster ambient term was 0. The two lines are the tutorial's own, with `ubo.` replaced by `frameUboTemplate.`; the tutorial's second comment line was left out because it talks about Ray Query. With 1.0 the ambient light is 10 % of the surface colour. | yes (owner, 2026-09-21: "the one with 1.0f", as close to the tutorial as possible) |

## How to append

Removal: add a row to the first table (or extend an existing one), then a row per forced edit in the second table.
Addition after owner approval: a row in the third table with the approval noted. Then update `src/source-file-difference.md`.
