# Code change rules

Mandatory for every change under `src/`, `CMakeLists.txt`, `CMakePresets.json` and `vcpkg.json`.
Binding for AI assistants and humans alike.

## The two trees

| Role | Path | Access |
|---|---|---|
| This project (the port) | `C:\Dev\VulkanRenderEngine` | editable |
| Reference (the tutorial) | `C:\Dev\Vulkan-Tutorial\attachments\simple_engine` | **read-only, never modified** |

The port is the tutorial engine with parts deleted. Every line in the port must be traceable to the tutorial.
Tutorial checkout pinned at commit `6dd48b5` (2026-09-07). If the tutorial is ever pulled, update this line
and re-run the comparison in `src/source-file-difference.md`.

## Rule 1 – No code out of thin air

Before any code is added:

1. Search the tutorial folder for an existing implementation of the same thing (function, member, shader, CMake target).
2. If it exists there: copy it 1:1 (Rule 3).
3. If it does not exist there: do **not** write it. Present the proposal to the owner and wait for explicit approval.

Proposal format (default until the owner says otherwise): a unified diff in the chat reply, with one sentence
per hunk saying why the tutorial has no equivalent. Nothing is written to disk before approval.

## Rule 2 – Removing functionality means deleting only

- Only delete: lines, blocks, functions, files, CMake entries.
- Do not rewrite, restructure or tidy surrounding code while deleting.
- If a deletion breaks the build and the fix needs **written** code (stub, changed signature, new include),
  that fix is a Rule 1 case: tutorial first, otherwise ask.
- Log the removal and any forced edit in `docs/DELETIONS.md`.

## Rule 3 – Missing functionality is copied 1:1 from the tutorial

1. Check the tutorial folder.
2. If present and needed: copy byte for byte (same file names, code, comments). No adapting, modernising or partial copies.
   Copy dependencies of the copied block the same way.
3. If absent: Rule 1.

## Checklist before touching code

- [ ] Name the rule the request falls under: add, remove, or restore.
- [ ] Add/restore: cite the tutorial file and line range the code comes from, or state that it is absent and stop for approval.
- [ ] Remove: confirm deletion only. List forced non-deletion edits separately for approval.
- [ ] Never write into `Vulkan-Tutorial`.
- [ ] Afterwards: update `docs/DELETIONS.md` and, if files changed, `src/source-file-difference.md`.

## Quick reference

| Situation | Allowed action |
|---|---|
| In tutorial, missing in port | Copy 1:1 |
| Exists nowhere | Propose, wait, then write |
| Should be removed | Delete only |
| Deletion breaks build | Fix only with tutorial code, otherwise ask |
| Anything in `Vulkan-Tutorial/` | Read only |
