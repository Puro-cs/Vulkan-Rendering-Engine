# Working agreement

How the assistant works in this repository. Defaults apply until the owner changes a line here.

| Topic | Current setting |
|---|---|
| Rule 1 proposals | unified diff in the chat reply, nothing written to disk before approval (> TODO owner: confirm or change) |
| Building | only when the owner asks, or after a Rule 2 deletion to prove the build still passes. Owner, 2026-09-21: for steps 2 to 9 of `docs/IMPLEMENTATION_PLAN.md` the assistant configures and runs a Debug build after every change without asking; it still never runs the exe |
| Running the exe | do not run; the owner runs and reports (> TODO owner) |
| Verification after a deletion | Debug build with 0 errors; count of validation messages unchanged (see `docs/BUILD.md`) |
| Logging | every change appends to `docs/DELETIONS.md`; file-level changes also update `src/source-file-difference.md` |
| Version control | none. `VulkanRenderEngine` is not a git repository, so there is no diff or revert per session (> TODO owner: `git init` + baseline commit?) |
| Tutorial folder | read-only. Never `git pull` it without updating the pinned commit in `docs/CODE_CHANGE_RULES.md` |
| Response style | name the rule first, cite tutorial file and lines, then the change |

## Session start checklist for the assistant

1. Rules are in context via `CLAUDE.md`. Re-read `docs/DELETIONS.md` to know what is already gone.
2. Before editing: locate the same code in `C:\Dev\Vulkan-Tutorial\attachments\simple_engine`.
3. After editing: update the log files named above.
