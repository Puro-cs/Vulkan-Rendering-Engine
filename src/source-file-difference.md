# Source file differences: VulkanRenderEngine/src vs Vulkan-Tutorial simple_engine

Compared on 2026-09-21 (previous comparisons: 2026-09-14, and 2026-09-20 after the ray-query render mode was removed).

- **Port:** `C:\Dev\VulkanRenderEngine\src`
- **Tutorial:** `C:\Dev\Vulkan-Tutorial\attachments\simple_engine`

Method: every file under both trees was listed, byte-compared, and a unified diff was produced for each shared file that differs. The tutorial's `build/`, `build-release/` and `Assets/` folders were excluded because they hold build output and binary assets, not source. Everything else (including `CMake/`, `Courses/`, `android/`, `imgui/` and `shaders/`) was compared.

## 1. Summary

| Category | Count |
|---|---|
| Only in the port | 1 file (plus this document) |
| Only in the tutorial | 28 files at top level and in `shaders/`, plus 3 whole folders (`CMake/`, `Courses/`, `android/`) |
| Shared and byte-identical | 49 files |
| Shared but different | 21 files |

History of the counts: the 2026-09-14 edition said 49 identical files, a miscount (its own list had 54 entries). 2026-09-20: 53, after `ray_query.spv` left. 2026-09-21: 49, because `forward_plus_cull.slang` / `.spv` were deleted and `pbr.slang` and `common_types.slang` moved to "different"; "different" stays at 21 because `renderer_ray_query.cpp` and `renderer_compute.cpp` left the port at the same time.

The port is the tutorial engine with eight feature areas deleted (Android/Linux/macOS platform code, physics + ball throwing, audio/HRTF, glTF animation, everything ray query: the render mode, the acceleration structures and the raster shadow option, Forward+ with its depth pre-pass, planar reflections, three dead panel controls), the opacity-micromap course hooks removed, one dead pipeline removed, a different demo scene, and one functional bug fix in ImGui keyboard handling. Almost every difference is a pure deletion; the only new code is the ImGui key translation and the scene setup lines in `main.cpp`.

## 2. Files that exist only in the port

| File | What it is |
|---|---|
| `imgui.ini` | Runtime-generated Dear ImGui window layout (positions/sizes of the `Debug##Default` and `Renderer` windows). Written by ImGui when the engine runs; not source code. |

## 3. Files that exist only in the tutorial

### Build system and helper scripts

| File | Purpose |
|---|---|
| `CMakeLists.txt` | Tutorial's CMake project (the port keeps its own build files outside `src/`). |
| `CMakePresets.json` | Ninja + vcpkg presets for `build/` and `build-release/`. |
| `CMake/Find*.cmake` (10 files) | Find modules for KTX, OpenAL, Vulkan, VulkanHpp, glfw3, glm, nlohmann_json, stb, tinygltf, tinyobjloader. |
| `fetch_bistro_assets.bat` / `.sh` | Downloads the Bistro scene. |
| `install_dependencies_linux.sh` / `install_dependencies_windows.bat` | Dependency bootstrap scripts. |

### Physics (whole subsystem removed from the port)

| File | Purpose |
|---|---|
| `physics_system.cpp` / `physics_system.h` | `PhysicsSystem`, `RigidBody`, `CollisionShape`, GPU physics. |
| `shaders/physics.slang` / `physics.spv` | Physics compute shader. |

### Audio (whole subsystem removed from the port)

| File | Purpose |
|---|---|
| `audio_system.cpp` / `audio_system.h` | `AudioSystem`, `AudioSource`, OpenAL + GPU HRTF. |
| `shaders/hrtf.slang` / `hrtf.spv` | HRTF compute shader. |

### Animation (whole subsystem removed from the port)

| File | Purpose |
|---|---|
| `animation_component.cpp` / `animation_component.h` | `AnimationComponent` that plays glTF animation clips. |

### Ray query and Forward+ (removed from the port on 2026-09-20 and 2026-09-21)

| File | Purpose |
|---|---|
| `shaders/ray_query.slang` / `ray_query.spv` | Compute shader of the "Ray Query" render mode: primary rays, shadows, reflections, refraction and thick glass through `VK_KHR_ray_query`, written to a storage image. |
| `renderer_ray_query.cpp` (1796 lines) | BLAS / TLAS build and refit, geometry-info and material buffers, ray-query descriptor sets. |
| `renderer_compute.cpp` (564 lines) | Compute descriptor / command pools, the HRTF dispatch, and Forward+: light-culling pipeline, tile buffers, params, dispatch. |
| `shaders/forward_plus_cull.slang` / `forward_plus_cull.spv` | Compute shader that sorts the lights into 16x16-pixel screen tiles. |

### Shaders the port never loads

| File | Purpose |
|---|---|
| `shaders/lighting.slang` / `lighting.spv` | Legacy forward lighting shader used by the removed `createLightingPipeline()`. |
| `shaders/pbr_android.slang` / `pbr_android.spv` | Reduced PBR shader selected on Android or when ray query is unavailable. |
| `shaders/pbr_full.slang` / `pbr_full.spv` | Alternate full PBR variant; not referenced by either code base's C++. |

### Course module and Android project

| Folder | Purpose |
|---|---|
| `Courses/` | Opacity Micromap course: `omm_integration.*`, `omm_imgui_panel.*`, `opacity_micromap_builder.*`, its own `CMakeLists.txt` and `README.adoc`. Enabled in the tutorial via `ENABLE_COURSE_OPACITY_MICROMAPS`. |
| `android/` | Gradle project, `AndroidManifest.xml`, `VulkanActivity.java`, `game_activity_bridge.cpp`, resources. |

## 4. Shared files that are byte-identical (49)

`camera_component.cpp`, `camera_component.h`, `component.cpp`, `component.h`, `debug_system.h`, `descriptor_manager.cpp`, `descriptor_manager.h`, `entity.cpp`, `entity.h`, `memory_pool.cpp`, `memory_pool.h`, `mesh_component.cpp`, `mesh_component.h`, `mikktspace.c`, `mikktspace.h`, `pipeline.cpp`, `pipeline.h`, `renderdoc_debug_system.h`, `resource_manager.cpp`, `resource_manager.h`, `scene_loading.h`, `swap_chain.h`, `thread_pool.h`, `transform_component.cpp`, `transform_component.h`, `vulkan_device.cpp`, `vulkan_device.h`, `vulkan_dispatch.cpp`

All 11 files in `imgui/` (`LICENSE.txt`, `imconfig.h`, `imgui.cpp`, `imgui.h`, `imgui_draw.cpp`, `imgui_internal.h`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h`).

Shaders (10): `composite.slang`, `composite.spv`, `imgui.slang`, `imgui.spv`, `lighting_utils.slang`, `pbr.spv`, `pbr_utils.slang`, `texturedMesh.slang`, `texturedMesh.spv`, `tonemapping_utils.slang`.

Note: all four `.spv` files were force-recompiled on 2026-09-21 and are byte-identical to the tutorial binaries, so the GPU code is the tutorial's. That includes `pbr.spv`, although `pbr.slang` and the imported `common_types.slang` now differ: the deleted declarations were unused and never reached the SPIR-V (`pbr.spv` contains only set 0 bindings 0 and 1 and set 1 binding 0).

## 5. Shared files that differ (21)

Line counts are tutorial -> port. "+" and "-" are added/removed lines relative to the tutorial.

| File | Lines | Change | What changed |
|---|---|---|---|
| `crash_reporter.h` | 444 -> 363 | -75 | Removed Linux/macOS/Android branches: `execinfo`/`signal` includes, `mkdir -p` fallback, Unix backtrace crash-file writer, Unix `signal()` handlers and their reset. Windows minidump path untouched. |
| `main.cpp` | 145 -> 112 | +3 / -33 | Removed `android_main()` entry point and the `PLATFORM_ANDROID` bistro path switch. Camera now starts at (2, 2, -2) with a (-35 deg, 135 deg, 0) rotation instead of (0, 0, 3). Loads `../assets/viking_room/viking_room.gltf` with a -90 deg X rotation instead of `../Assets/bistro/bistro.gltf`. |
| `engine.h` | 439 -> 292 | -127 | Removed `audio_system.h`, `physics_system.h`, `Courses/omm_integration.h` includes; `GetAudioSystem()`, `GetPhysicsSystem()`, `GetOmmIntegration()`; `InitializeAndroid()`/`RunAndroid()`; `audioSystem`, `physicsSystem`, `ommIntegration` members; mobile touch/tilt fields in `cameraControl`; `BallMaterial`, `PhysicsScaling`, `PendingBall` structs and members; `GenerateBallMaterial()`, `InitializePhysicsScaling()`, `Scale*ForPhysics()`, `ThrowBall()`, `ProcessPendingBalls()`. |
| `engine.cpp` | 1251 -> 655 | +3 / -500 | Removed the Android early-return in `Initialize()`, creation/teardown of audio, physics and OMM subsystems, `SetAudioSystem()` wiring, ball material/physics init calls, right-click ball throwing, touch tracking in `handleMouseInput()`, Android `handleKeyInput()` branch, physics/audio `Update()` calls, camera-tracks-ball logic, all Android accelerometer/swipe/hold camera controls, the ball/physics helper implementations, and `InitializeAndroid()`/`RunAndroid()`. Three comments reworded to drop "physics". |
| `imgui_system.h` | 272 -> 235 | -32 | Removed `AudioSystem`/`AudioSource` forward declarations, `SetAudioSystem()`, `IsBallOnlyRenderingEnabled()`, `IsCameraTrackingEnabled()`, audio pointers and source position fields, `ballOnlyRenderingEnabled`, `cameraTrackingEnabled`. |
| `imgui_system.cpp` | 1149 -> 1034 | +151 / -266 | 2026-09-21: removed the acceleration-structure phase of the loading overlay and the "Building acceleration structures..." status window; the streaming window no longer shifts below it. Earlier: removed `audio_system.h` include, Android 2x UI scaling, `SetAudioSystem()`, the whole "HRTF Audio Controls" window (audio source selection, directional buttons, play/stop, ball debugging checkboxes, texture progress), and the Android touch-source event. **Added** a `GlfwKeyToImGuiKey()` translation table (mirrors ImGui's official GLFW backend) and rewrote `HandleKeyboard()` to translate GLFW codes to named `ImGuiKey` values and keep Ctrl/Shift/Alt/Super modifier flags in sync. This is the bug fix for the Debug-build `IsNamedKeyOrMod` assert. One comment updated to drop the Physics loading phase. |
| `model_loader.h` | 478 -> 369 | -96 | Removed `AnimationInterpolation`, `AnimationPath`, `AnimationSampler`, `AnimationChannel`, `Animation` types; `MaterialMesh::sourceMeshIndex`; `Model` animation accessors and members (`animations`, `animatedNodeTransforms`, `animatedNodeMeshes`); `ModelLoader::GetAnimations()`, `ProcessAnimations()`; the `animatedNodeIndices` parameter of `ProcessMeshes()`. |
| `model_loader.cpp` | 1977 -> 1818 | +1 / -134 | Removed `ProcessAnimations()` implementation, the animation call and animated-node collection in `ParseGLTF()`, capture of animated node transforms/meshes during scene traversal, `sourceMeshIndex` assignment, and `GetAnimations()`. One comment drops "(supports Android assets)". |
| `platform.h` | 615 -> 298 | -283 | Removed the Android include/log-macro block, `GetAccelerometerData()`/`GetDisplayRotation()` virtuals, the entire `AndroidPlatform` class, the entire `DirectDisplayPlatform` class (VK_KHR_display kiosk mode), and the Android/direct-display branches in `CreatePlatform()`. Only `DesktopPlatform` remains. |
| `platform.cpp` | 720 -> 161 | -462 | Removed all `AndroidPlatform` implementation (sensors, GameActivity input, JNI device/battery queries) and all `DirectDisplayPlatform` implementation (signal handling, display/plane/mode selection). Desktop GLFW code untouched. |
| `renderdoc_debug_system.cpp` | 164 -> 149 | -15 | Removed `dlfcn.h` include and the `dlopen("librenderdoc.so")` Linux/macOS branch. |
| `renderer.h` | 2135 -> 1405 | +6 / -736 | 2026-09-21: removed `RenderMode` with its accessors, the reflection-intensity member and accessors, all planar-reflection members (`ReflectionRT`, `reflections`, ...), all acceleration-structure types, members, getters and the two `RequestAccelerationStructureBuild()` overloads, `LoadingPhase::AccelerationStructures`, the three ray-tracing device extensions, all Forward+ types and members, `depthPrepassPipeline`, `pbrPrepassGraphicsPipeline`, the compute pools, `pbrFixedBindingsWritten`, and the matching declarations. `UniformBufferObject` is unchanged. Ray-query render mode (2026-09-20): removed `RayQueryUniformBufferObject` and its `static_assert`s, `RenderMode::RayQuery`, `ToggleRenderMode()`, `updateRayQueryDescriptorSets()`, the three `createRayQuery*()` declarations, all ray-query pipeline / descriptor / UBO / output-image members, `rqCompositeDescriptorSets`, `rqCompositeSampler`, and the nine option members of the "Ray Query" panel; `currentRenderMode` now defaults to `RenderMode::Rasterization`; four comments trimmed. Earlier: removed `DispatchCompute()` (HRTF), `WaitForRawPixelCacheToSettle()`, `LoadingPhase::Physics`, `GetOpacityMicromapEnabled()`/`GetOpacityEnabled()`, raw pixel cache API (`StoreRawTexturePixels`, `GetRawTexturePixels`, `ClearRawPixelCache`), micromap provider callback, `GetThreadPool()`/`KickWatchdog()`, `EnqueueInstanceBufferRecreation()`, `recreateInstanceBuffer()`, `SHARED_BRIGHT_RED_ID`, lighting pipeline/layout/rendering-info members, compute pipeline/layout/set-layout/descriptor-set members, `pendingInstanceBufferRecreations`, `opacityMicromapEnabled`, `RawPixelEntry` cache, `createLightingPipeline()`. Three comments reworded to drop "physics". `computeDescriptorPool` and `computeCommandPool` are kept. |
| `renderer_core.cpp` | 1214 -> 993 | +8 / -229 | 2026-09-21: `Initialize()` no longer creates the compute pools, the Forward+ resources or the depth pre-pass pipeline; `Cleanup()` lost their teardown and the AS teardown; `createLogicalDevice()` no longer queries or enables the acceleration-structure and ray-query features. Ray-query render mode (2026-09-20): removed the `createRayQueryDescriptorSetLayout()`, `createRayQueryPipeline()` and `createRayQueryResources()` calls in `Initialize()` and the ray-query pipeline / layout / descriptor set / composite sampler / output image resets in `Cleanup()`; one comment trimmed. Acceleration-structure teardown and the ray-query / AS device features are untouched. Earlier: removed `vulkan_android.h` include, Android render-mode override in the constructor, Android `DynamicLoader` dispatcher init, `createLightingPipeline()` call, `computeDescriptorSets`/`lightingPipeline`/`lightingPipelineLayout`/`computeDescriptorSetLayout` teardown, direct-display and Android instance-extension branches (GLFW extensions are now unconditional), opacity micromap device extensions and feature chaining, and the `!PLATFORM_ANDROID` guards around local-read/tile-image features. |
| `renderer_pipelines.cpp` | 1408 -> 731 | -678 | 2026-09-21: the PBR descriptor set layout shrank from bindings 0 to 13 to bindings 0 to 6 (7 to 9 Forward+, 10 reflection map, 11 to 13 TLAS / geometry / material removed); removed the after-pre-pass PBR pipeline, the reflection PBR pipeline and `createDepthPrepassPipeline()`. Ray-query render mode (2026-09-20): removed `createRayQueryDescriptorSetLayout()`, `createRayQueryPipeline()` (loaded `ray_query.spv`) and `createRayQueryResources()` (output image, descriptor sets, composite sets and sampler, per-frame UBOs). Earlier: removed the `pbr_android.spv` fallback when ray query is unavailable, and the whole `createLightingPipeline()` implementation. Every remaining line is a tutorial line. (678, not 677: the tutorial file ends without a final newline.) |
| `renderer_rendering.cpp` | 2781 -> 1554 | +12 / -1240 | 2026-09-21: removed the three planar-reflection functions (about 300 lines) and every call into them, the acceleration-structure housekeeping in `Render()` (deferred deletion, readiness scan, build handling, TLAS refit, about 300 lines), the Forward+ depth pre-pass and light-culling dispatch in the raster pass, the per-frame tile buffer resize, and from the panel the mode combo, "Forward+", "RayQuery shadows (raster)", "Reflection resolution scale", "Reflection intensity" and "Gamma". The opaque pass now always clears depth and draws PBR objects with `pbrGraphicsPipeline`. Ray-query render mode (2026-09-20): removed from `Render()` the compute dispatch with its UBO upload, the composite blit of the ray-query image, the magenta "TLAS not built" frame, the per-frame ray-query descriptor update, the static-only one-time AS request, `rayQueryRenderedThisFrame`, the ray-query texture-streaming budget, the "Ray Query" combo entry with its mode-switch handler and the panel section "Ray Query Status / Ray Query Features / Thick Glass"; from `recreateSwapChain()` the ray-query descriptor and output-image handling; from `prepareFrameUboTemplate()` two writes of deleted option members. The 12 lines that differ from the tutorial today are forced edits listed in `docs/DELETIONS.md` (bare scopes, the `eClear` load op, the loading condition, the collapsed pipeline choice, trimmed comments). Earlier: removed lighting pipeline/layout teardown and recreation on swapchain rebuild, and two `IsRayQueryStaticOnly()` filters that skipped `_AnimNode_`/`Ball_` entities during readiness counting. |
| `renderer_resources.cpp` | 4215 -> 3518 | +14 / -711 | 2026-09-21: PBR descriptor writes now cover bindings 0 to 6 only (the writes of 7 / 8, 10 and 11 to 13 are gone at both sites, with the dummy tile buffers and the "fixed bindings" mechanism); removed the never-called `refreshPBRForwardPlusBindingsForFrame()`, the Forward+ compute refresh after a light-buffer resize, both "uploads completed" AS rebuild requests, the AS usage flag on mesh buffers, and the acceleration-structure, storage-image and ray-query texture reservations of `createDescriptorPool()`. Ray-query render mode (2026-09-20): removed the ray-query dirty-mask write in `OnTextureUploaded()`. Earlier: removed `SHARED_BRIGHT_RED_ID` definition, creation and sRGB format rule; Android in-memory KTX2 loading paths (named-file load is now unconditional); all `ENABLE_COURSE_OPACITY_MICROMAPS` raw-pixel caching in texture load paths; `EnqueueInstanceBufferRecreation()`, the instance-buffer recreation batch in `ProcessPendingEntityPreallocations()`, and `recreateInstanceBuffer()`. |
| `renderer_utils.cpp` | 443 -> 361 | -69 | Removed Android branches in `readFile()` (external dir + APK assets), `fileExists()`, and `chooseSwapExtent()`. Desktop code unchanged. |
| `scene_loading.cpp` | 600 -> 315 | +2 / -287 | 2026-09-21: removed the acceleration-structure build request at the end of `LoadGLTFModel()`; the loader always takes the tutorial's other branch and sets the `Finalizing` phase. Earlier: removed `animation_component.h` include, physics progress counters and logging, the Physics loading phase, per-entity rigid-body creation for near-ground geometry, the physics summary log, all animation controller/entity setup after load, and the OMM `buildMicromaps()` call before the AS build. |
| `shaders/pbr.slang` | 109 -> 108 | -1 | 2026-09-21: removed the unused declaration `[[vk::binding(10, 0)]] Sampler2D reflectionMap;` because binding 10 no longer exists in the PBR set layout. Compiled `pbr.spv` unchanged. |
| `shaders/common_types.slang` | 204 -> 139 | -65 | 2026-09-21: removed `TileHeader` (Forward+) and the `GeometryInfo` / `MaterialData` mirror structs (acceleration structures). No remaining shader used them; `UniformBufferObject`, `LightData` and `PushConstants` are unchanged. Compiled shaders unchanged. |
| `vulkan_compatibility.h` | 172 -> 30 | -113 | Removed the `VK_KHR_OPACITY_MICROMAP_EXTENSION_NAME` fallback and the entire Android KHR-to-EXT opacity micromap alias block (typedefs, enum values, `vk::` using-aliases). Only the tile-image/local-read extension-name fallbacks remain. |

## 6. Overview of the differences by theme

### 6.1 Platform: Windows desktop only
The tutorial targets Windows, Linux, macOS, Android, and a windowless VK_KHR_display "direct display" mode. The port keeps only the GLFW `DesktopPlatform`. Affects `platform.h/.cpp`, `engine.h/.cpp`, `main.cpp`, `renderer_core.cpp`, `renderer_utils.cpp`, `renderer_resources.cpp`, `imgui_system.cpp`, `crash_reporter.h`, `renderdoc_debug_system.cpp`, `vulkan_compatibility.h`, and the `android/` folder. (`shaders/ray_query.slang` also lost its Android stub, before the whole file was deleted with the ray-query render mode, see 6.7.)

### 6.2 Physics and ball throwing removed
`PhysicsSystem`, `physics.slang`, the `Physics` loading phase, rigid-body creation for scene geometry, right-click ball throwing, ball material generation, camera-track-ball, the shared bright-red texture, the virtual `BallMaterial` (index 9999) in the ray-query material table, and the ImGui ball debug controls are all gone.

### 6.3 Audio and HRTF removed
`AudioSystem`, `hrtf.slang`, `Renderer::DispatchCompute()`, the HRTF compute pipeline and descriptor sets, and the "HRTF Audio Controls" ImGui window are gone. `createComputePipeline()` survives but only creates a descriptor pool.

### 6.4 glTF animation removed
`AnimationComponent`, the animation data model in `model_loader.h`, `ProcessAnimations()`, animated node capture, `sourceMeshIndex`, instance-buffer recreation, and the animation controller setup in `scene_loading.cpp` are gone. The `_AnimNode_` readiness filters in `renderer_rendering.cpp` went with them.

### 6.5 Opacity Micromap course hooks removed
Every `ENABLE_COURSE_OPACITY_MICROMAPS` block is deleted: raw pixel cache, micromap provider callback, OMM device extensions/features, BLAS `pNext` attachment, `buildMicromaps()` call, and the `Courses/` folder itself.

### 6.6 Unused lighting pipeline removed
`createLightingPipeline()`, its pipeline/layout members, and `lighting.slang` are gone. Neither code base ever bound this pipeline for drawing.

### 6.7 Ray-query render mode removed (2026-09-20)
The tutorial has two render modes and starts in "Ray Query" when the GPU supports `VK_KHR_ray_query`: a compute shader (`ray_query.slang`) traces the scene into a storage image that `composite.slang` blits to the swap chain. The port keeps only "Rasterization". Gone: the shader, its pipeline, descriptor set layout, descriptor sets, per-frame UBO (`RayQueryUniformBufferObject`), output image, composite descriptor sets and sampler, the dispatch in `Render()`, `RenderMode::RayQuery`, `ToggleRenderMode()`, and the panel section with "Enable Hard Shadows", "Shadow samples", "Shadow softness", "Enable Reflections", "Enable Transparency/Refraction", "Max secondary bounces", "Enable Thick Glass", "Thickness Clamp", "Absorption Scale" and the acceleration-structure status line.

On 2026-09-20 the acceleration-structure subsystem stayed, because the raster option "RayQuery shadows (raster)" and PBR bindings 11 to 13 used it. It followed on 2026-09-21, see 6.8. `composite.slang` stays for good: the raster path draws the off-screen opaque colour to the swap chain with it.

### 6.8 Dead options and their machinery removed (2026-09-21)
Use case named by the owner: students without Vulkan experience write simple shaders. Every panel control without a visible effect went, with the code behind it:

- **Acceleration structures**: `renderer_ray_query.cpp`, the BLAS / TLAS build after loading with its loading phase and progress UI, the per-frame TLAS refit, PBR bindings 11 to 13, the three ray-tracing device extensions, the option "RayQuery shadows (raster)" (`pbr.slang` never declared a TLAS).
- **Forward+**: `renderer_compute.cpp`, `forward_plus_cull.slang`, the depth pre-pass, the after-pre-pass PBR pipeline, PBR bindings 7 to 9, the option "Forward+ (tiled light culling)" (on by default; no shader read the tile lists). The pre-pass drew with the vertex shader of `pbr.slang`, so a student's modified vertex shader would have been depth-tested against the unmodified shape.
- **Planar reflections**: three functions, the reflection render targets and pipeline, PBR binding 10, the "Reflection resolution scale" slider. The feature was switched off in code upstream.
- **Small controls**: the "Rendering Mode" combo with `RenderMode`, "Reflection intensity" (no shader read it), the "Gamma" slider (ignored on sRGB swap chains; the code path stays).

Result: the PBR descriptor set layout is bindings 0 to 6 (UBO, five textures, light buffer), `renderer_*.cpp` went from about 10.6k to about 7.2k lines, and the panel holds only working controls. `UniformBufferObject` was left untouched on both sides, which keeps all four `.spv` files byte-identical to the tutorial's. Affects `renderer.h`, `renderer_core.cpp`, `renderer_pipelines.cpp`, `renderer_rendering.cpp`, `renderer_resources.cpp`, `imgui_system.cpp`, `scene_loading.cpp`, `shaders/pbr.slang`, `shaders/common_types.slang`, `CMakeLists.txt`; details, kept items and forced edits in `docs/DELETIONS.md`.

### 6.9 Demo scene
The port loads the Viking Room glTF from `../assets/` with a -90 deg X rotation and places the camera at (2, 2, -2) looking back at the origin. The tutorial loads Bistro from `../Assets/` with the camera at (0, 0, 3).

### 6.10 ImGui keyboard bug fix (only functional addition)
`imgui_system.cpp` gains `GlfwKeyToImGuiKey()` and a rewritten `HandleKeyboard()`. The tutorial passes raw GLFW key codes to `io.AddKeyEvent()`, which trips `IM_ASSERT(ImGui::IsNamedKeyOrMod(key))` in Debug builds of ImGui 1.87+. The port translates to named `ImGuiKey_*` values and also emits `ImGuiMod_*` events for modifier keys.

### 6.11 Cosmetic
A handful of comments were edited to drop references to physics or Android assets (`renderer.h`, `engine.cpp`, `imgui_system.cpp`, `model_loader.cpp`). With the ray-query render mode (2026-09-20), 11 comments lost the words that referred to it (`renderer.h` 4, `renderer_core.cpp` 1, `renderer_rendering.cpp` 6), and three conditions in `renderer_rendering.cpp` shrank to bare `{ }` scopes so that the code inside them did not have to be re-indented. The removals of 2026-09-21 trimmed 13 more comments (`renderer.h` 4, `renderer_core.cpp` 4, `renderer_resources.cpp` 3, `imgui_system.cpp` 1, `scene_loading.cpp` 1), produced four more bare scopes (`renderer_rendering.cpp` 1, `renderer_resources.cpp` 2, `scene_loading.cpp` 1) and re-indented one line (the PBR pipeline choice in the opaque pass). Some of the 2026-09-20 edits disappeared again together with the code they sat in.
