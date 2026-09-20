# VulkanRenderEngine

Port of the Vulkan-Tutorial "simple engine" (`C:\Dev\Vulkan-Tutorial\attachments\simple_engine`, read-only, pinned
at commit `6dd48b5`) reduced by deletion to a Windows-only Vulkan rendering study system.

Mandatory rules for every code change, read before editing anything:

1. No invented code. Check the tutorial folder first; if the code is not there, ask the owner before writing it.
2. Removing functionality = deleting only. Any code that must be written because of a removal falls under rule 1.
3. Missing functionality is copied 1:1 from the tutorial, never rewritten.
4. `C:\Dev\Vulkan-Tutorial` is never modified.

Full rules and project context:

@docs/CODE_CHANGE_RULES.md
@docs/WORKFLOW.md
@docs/BUILD.md
@docs/ARCHITECTURE.md
@docs/DELETIONS.md
@docs/ROADMAP.md

`src/source-file-difference.md` holds the per-file comparison with the tutorial; read it when a task names a file.
