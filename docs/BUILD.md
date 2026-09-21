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
Forward+ and planar reflections were removed on 2026-09-20 / 2026-09-21) and one mesh shader, `pbr.slang` (the basic
lighting path went on 2026-09-21). The "Renderer" ImGui panel shows, top to bottom: "Culling & LOD" (frustum
culling, distance LOD with two thresholds, sampler anisotropy, culling statistics) and "Tone Mapping & Tuning"
(exposure). The group "Rasterization Options" with "Use Basic Lighting (Phong)" was removed on 2026-09-21 (planned
change 2). Every control has a visible effect; the controls that had none were removed on 2026-09-21. The panel is
the same on every GPU: nothing depends on ray-tracing support any more.

Scene: `SetupScene()` in `src/sandbox.cpp` (the students' file, planned change 5) creates a camera, the directional
light "Sun", the Viking room and a small white sphere in front of it (`CreateSphere("Sphere", 0.2f)` at
(0.5, 0.3, -1.2); the position was chosen without seeing the picture). `assets/viking_room/viking_room.gltf`
references `viking_room.ktx2`. The renderer decodes **KTX2 only**; a glTF that references PNG or JPEG loads geometry
with the default albedo and prints a warning. `viking_room.png` in the assets folder is unused by the engine. A
model that cannot be loaded prints an error and leaves an empty object; the rest of the scene is still shown.
The model is loaded before the first frame: the window stays empty for the moment the parse takes, then the engine's
loading overlay covers the texture and mesh uploads of the first frames.

## Expected console output in Debug

Validation layers are on in Debug (`ENABLE_VALIDATION_LAYERS` in `sandbox_impl.cpp`, until planned change 5 in `main.cpp`). One message is inherited from the
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

### Stale object files (incident of 2026-09-21)

Symptom: the Debug exe stopped at start-up in `ntdll.dll` with "A LIST_ENTRY has been corrupted (i.e. double
remove)". Cause: not the code. Planned change 4 added members to `class Renderer` in `renderer.h`; the next
incremental Debug build recompiled only the four `.cpp` files that had changed themselves and skipped six other files
that include `renderer.h` (`engine`, `imgui_system`, `model_loader`, `renderer_resources`, `renderer_utils`,
`scene_loading`), although MSBuild's tracking log listed `renderer.h` as their dependency. The exe then mixed two
layouts of `Renderer`, and the old-layout code wrote into the wrong members. The intermediate folder
`build/windows-msvc/VulkanRenderEngine.dir/<Config>/` had held tracking logs of two project paths since the repository
folder was renamed; MSBuild warned about it on every build (`MSB8028`, "can lead to incorrect clean and rebuild
behavior"). After both intermediate folders were deleted and everything was rebuilt, the warning is gone, and the
same kind of change (header plus some sources touched) recompiles all ten dependents again.

How to recognise it: in the build output only the list after "Compiling..." is what was compiled; the list after
"Scanning sources for module dependencies..." is a scan and says nothing about recompilation. An object file under
`build/windows-msvc/VulkanRenderEngine.dir/<Config>/` that is older than a header it includes is stale.
How to recover: close the engine, delete `build/windows-msvc/VulkanRenderEngine.dir/Debug` (and `Release`), build
again; in Visual Studio "Rebuild" does the same. Never ignore `MSB8028`.

## Known issues

- `ImGuiSystem::HandleKeyboard()` in the tutorial passes raw GLFW key codes to `ImGuiIO::AddKeyEvent()`. Dear ImGui 1.92
  asserts on that, so the first key press aborted a Debug build. Fixed 2026-09-13 with `GlfwKeyToImGuiKey()` in
  `src/imgui_system.cpp` (mirrors the official GLFW backend mapping). Until planned change 3 this was the only code
  in the port that is not in the tutorial; every later addition is listed under "Added code" in `docs/DELETIONS.md`.
- Since 2026-09-21 (planned change 3) `pbr.slang` is the tutorial's full PBR shader, trimmed: it lights the scene
  with the scene lights (storage buffer at PBR set 0, binding 6) and adds 10 % ambient (`scaleIBLAmbient` is set to
  1.0 by C++ now). A scene without a light is therefore dark: only the ambient part and emissive surfaces show. The
  Viking room glTF has no `KHR_lights_punctual` light; the room is lit by the light "Sun" that `SetupScene()` in
  `sandbox.cpp` creates (directional, rotation -45°, 45°, 0°, intensity 3.0; the owner tried 100.0 on 2026-09-21
  and went back to 3.0). Glass gets no light highlights: in the tutorial shader they only existed as a
  Forward+ tile loop, which was removed. (The old 109-line `pbr.slang` used the fixed light direction (1, 1, 1); the
  second mesh shader, `texturedMesh.slang`, was removed on 2026-09-21.)
- "Sampler max anisotropy" recreates every sampler but does not rewrite the descriptor sets that reference the old
  ones (tutorial behaviour, found by reading the code, not run): expect validation messages when dragging it in Debug.
- The "Exposure" slider goes down to 0.1, the code clamps at 0.2. Exposure affects the opaque scene only; transparent
  objects are drawn after the composite pass.
- Textures must be KTX2 (see Run).
- Crash minidumps are written to `src/crashes/` by `CrashReporter` (working directory at run time).
