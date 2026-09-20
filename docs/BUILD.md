# Build, run, verify

## Toolchain (verified 2026-09-14 on this machine)

| Component | Version / location |
|---|---|
| Visual Studio 2026, MSVC | 19.51.36256 (generator `Visual Studio 18 2026`, x64) |
| CMake | 4.3.2 (minimum 3.29) |
| Vulkan SDK | 1.4.350.0 at `C:\VulkanSDK\1.4.350.0` (`VULKAN_SDK`), provides `slangc`, `spirv-opt`, validation layers |
| vcpkg | `C:\dev\vcpkg` (`VCPKG_ROOT`), manifest mode, baseline in `vcpkg.json` |
| GPU | NVIDIA GeForce RTX 4070 Laptop GPU (ray-tracing support is no longer requested or used since 2026-09-21) |

vcpkg packages installed into `build/windows-msvc/vcpkg_installed`: glfw3 3.5.1, glm 1.0.3, ktx 4.4.2, nlohmann-json 3.12.0, tinygltf 3.0.0, stb, zstd.
Vulkan headers, loader and vulkan.hpp come from the SDK, not from vcpkg.

## Commands (run from the repository root)

Configure once; the first run installs the vcpkg packages and takes minutes:

```bash
cmake --preset windows-msvc
```

Build:

```bash
cmake --build --preset windows-msvc-debug
```

```bash
cmake --build --preset windows-msvc-release
```

Output: `build/windows-msvc/Debug/VulkanRenderEngine.exe` (or `Release/`).
Shaders: every `src/shaders/*.slang` except the imported utility modules (`common_types`, `pbr_utils`, `lighting_utils`,
`tonemapping_utils`) is compiled with `slangc -target spirv -profile spirv_1_3 -O3`, optimised with `spirv-opt`,
written to `build/windows-msvc/shaders/` and copied to `src/shaders/*.spv`. The `.spv` files next to the sources are
build output, not source.

## Run

The executable resolves `shaders/*.spv` and `../assets/viking_room/viking_room.gltf` relative to the **working directory**,
so run it from `src/`. The Visual Studio debugger is configured for that (`VS_DEBUGGER_WORKING_DIRECTORY`).

```bash
cd src && ../build/windows-msvc/Debug/VulkanRenderEngine.exe
```

Window 800x600, title shows frame count, FPS and ms. Controls: `W A S D` move, `Q E` down/up, left mouse drag looks.
`Esc` is not bound; close the window to quit. The engine renders with one forward rasterization path (ray query,
Forward+ and planar reflections were removed on 2026-09-20 / 2026-09-21). The "Renderer" ImGui panel shows, top to
bottom: "Rasterization Options" ("Use Basic Lighting (Phong)" with two status lines), "Culling & LOD" (frustum
culling, distance LOD with two thresholds, sampler anisotropy, culling statistics) and "Tone Mapping & Tuning"
(exposure). Every control has a visible effect; the controls that had none were removed on 2026-09-21. The panel is
the same on every GPU: nothing depends on ray-tracing support any more.

Scene: `assets/viking_room/viking_room.gltf` references `viking_room.ktx2`. The renderer decodes **KTX2 only**; a glTF
that references PNG or JPEG loads geometry with the default albedo and prints a warning. `viking_room.png` in the
assets folder is unused by the engine. Without a loadable scene the window shows only the ImGui panel on black.

## Expected console output in Debug

Validation layers are on in Debug (`ENABLE_VALIDATION_LAYERS` in `main.cpp`). One message is inherited from the
tutorial and is not a bug:

- `robustBufferAccess2` enabled without `robustBufferAccess`

Until 2026-09-20 there was a second one, the `Int64` capability in `ray_query.spv`; it disappeared with that shader.
Confirmed by the owner's Debug run of 2026-09-21 (after all removals, including lighting toggles and a window
resize): exactly this one validation message, nothing new. Anything else from the validation layer is new and must
be reported.

The callback prints warnings and errors to stderr and the (very chatty) info / verbose loader messages to stdout, so
capture them separately to read them, from `src/`:
`cmd /c "..\build\windows-msvc\Debug\VulkanRenderEngine.exe > ..\build\run_stdout.txt 2> ..\build\run_stderr.txt"`.
Besides the validation message, stderr holds two harmless lines of the model loader for the Viking room:
`Warning: No decoded bytes for baseColor texture index 0` and `Warning: Failed to extract punctual lights from ...`
(the glTF has no `KHR_lights_punctual` lights). In stdout, `Renderer: Ending load cycle without completion mark.
Forcing completion to avoid deadlock.` is the normal end of loading: the `LoadingGuard` in `scene_loading.cpp` calls
`SetLoading(false)`, which completes the load itself, so `Transitioning from Loading to Active scene` never prints.
Six blank lines after `Features queried successfully` come from the two-statement `LOGW` macro used under an
unbraced `if` (tutorial code).

After a shader file is added or deleted, run `cmake --preset windows-msvc` again: the `*.slang` list is a configure-time
glob, and a build tree that still lists a deleted shader fails in the `shaders` target.

## Known issues

- `ImGuiSystem::HandleKeyboard()` in the tutorial passes raw GLFW key codes to `ImGuiIO::AddKeyEvent()`. Dear ImGui 1.92
  asserts on that, so the first key press aborted a Debug build. Fixed 2026-09-13 with `GlfwKeyToImGuiKey()` in
  `src/imgui_system.cpp` (mirrors the official GLFW backend mapping). This is the **only** code in the port that is
  not in the tutorial. See `docs/DELETIONS.md`.
- Both mesh shaders light the scene with one fixed direction (1, 1, 1). Neither is Phong or PBR in the textbook sense
  (no specular term, no BRDF), whatever the panel label says. `texturedMesh.slang` adds 10 % ambient; in `pbr.slang`
  the ambient term is multiplied by `scaleIBLAmbient`, which C++ never sets, so unlit sides are black.
- Scene lights are uploaded to a storage buffer (PBR set 0, binding 6) every frame, but no shader reads them. The
  ray-query shader, which did, was removed on 2026-09-20.
- "Sampler max anisotropy" recreates every sampler but does not rewrite the descriptor sets that reference the old
  ones (tutorial behaviour, found by reading the code, not run): expect validation messages when dragging it in Debug.
- The "Exposure" slider goes down to 0.1, the code clamps at 0.2. Exposure affects the opaque scene only; transparent
  objects are drawn after the composite pass.
- Textures must be KTX2 (see Run).
- Crash minidumps are written to `src/crashes/` by `CrashReporter` (working directory at run time).
