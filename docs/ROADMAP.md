# Roadmap

Owner: Paulo. Purpose of the port and the planned changes, in order. Each entry names the rule that applies
(`docs/CODE_CHANGE_RULES.md`). Status: `todo`, `in progress`, `done`, `dropped`.

## Purpose

This engine serves as a learning material for Vulkan and graphics programming at the HTWK Leipzig. It is not intended to be a production-ready engine.
It got developed in the scope of the bachelore theosis of Paulo Hoheisel.

## Planned changes

No planned changes yet

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

## Candidates already visible in the code (not decided)

- Remove the four compiled-but-unreferenced files: `pipeline.*`, `descriptor_manager.*`, `renderdoc_debug_system.*`,
  `resource_manager.*` (the last is owned by Engine but never called). Rule 2.
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
  direction, diffuse only, `scaleIBLAmbient` never set). These are the files students will edit; decide whether the
  labels should change or the shaders should become what the labels say. New text or shader code: Rule 1.
- Let a raster shader read the scene lights (PBR binding 6 is uploaded every frame and unread). Rule 1; closes the
  light part of `tr:szenenmanagement` in `docs/REQUIREMENTS.md`.
- "Sampler max anisotropy" recreates the samplers without rewriting the descriptor sets that reference the old ones
  (tutorial behaviour). A fix is new code: Rule 1. Removing the slider is Rule 2.
- Remove the loading watchdog thread. Rule 2. `watchdogSuppressed` is always false since the AS builds are gone.
