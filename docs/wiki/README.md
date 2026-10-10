# docs/wiki — the pages of the GitHub wiki

These Markdown files are the pages of the repository's GitHub wiki (written 2026-10-07 from the thesis project,
`C:\ClaudeWorkspace\BA`, plan step 4). They live here only until they are pushed to the wiki repository; the wiki is
the place students read them. File name = wiki page name (hyphens become spaces in the page title); `Home.md` is the
start page, `_Sidebar.md` the page list shown beside every page. Since 2026-10-10 the wiki has one page per learning
goal of the worksheet (NSL 1–4), named like the worksheet's tasks, plus seven frame pages (thesis rule W-2); the six
former NoS pages (`NoS-1-…` to `NoS-6-…`) are replaced by the four task pages and are to be deleted, here and in the
wiki repository. In the file names of task pages 1 and 3 the hyphen of "Frame-Ablauf" and "Phong-Modell" is U+2010
(HYPHEN), not the ASCII hyphen: GitHub turns an ASCII hyphen of a file name into a blank in the page title, so only this
character keeps the hyphen the worksheet uses in the page names; the links `[[…|…]]` use the same character.

| File | Page |
|---|---|
| `Home.md` | Start |
| `Setup.md` | Setup |
| `Architektur.md` | Architektur (KI 1–8) |
| `1-Initialisierungskette-und-Frame‐Ablauf.md` | task page 1 (NSL 1; NoS 1 and NoS 2) |
| `2-Eigene-Pipeline-und-eigene-Szene.md` | task page 2 (NSL 2; NoS 3, NoS 4 and NoS 6) |
| `3-Phong‐Modell-im-Shader.md` | task page 3 (NSL 3; NoS 5) |
| `4-Materialien-und-Referenzgrafiken.md` | task page 4 (NSL 4; no NoS of its own) |
| `Eigene-Modelle.md` | Eigene Modelle |
| `Pipeline-Ansicht.md` | Pipeline-Ansicht |
| `Fehler-und-Debugging.md` | Fehler und Debugging |
| `Begriffe.md` | Begriffe |
| `_Sidebar.md` | sidebar |

## Pushing them to the wiki

The wiki is its own git repository, `https://github.com/Puro-cs/Vulkan-Rendering-Engine.wiki.git`. It exists only
after the first page was created in the browser (repository → Wiki → *Create the first page*; the content does not
matter, `Home.md` replaces it).

```
git clone https://github.com/Puro-cs/Vulkan-Rendering-Engine.wiki.git
copy C:\Dev\Vulkan-Rendering-Engine\docs\wiki\*.md Vulkan-Rendering-Engine.wiki\
cd Vulkan-Rendering-Engine.wiki
git add -A
git commit -m "Wiki pages for the seminar worksheet"
git push
```

(`README.md` of this folder is not a wiki page; leave it out or delete it in the wiki clone.) Links between pages use
the wiki form `[[Text|Page-Name]]`; links into `docs/` are absolute GitHub URLs to `main`.

Engine facts on the frame pages come from `docs/ARCHITECTURE.md`, `docs/BUILD.md`, `docs/ROADMAP.md`, `src/sandbox.h`,
`src/sandbox.cpp`, `src/sandbox_impl.cpp` and the shaders at commit `cf78c92`; the Begriffe definitions from the
Khronos documentation pages linked at the end of that page. The four task pages (2026-10-10) were written against `main`
`82ed9d1`, `learning` `f608792` and `phong-solution` `5873cc7` from `src/sandbox.cpp` (both branches), `src/sandbox.h`,
`src/sandbox_impl.cpp`, `src/pipeline_settings.h`, `src/renderer_pipelines.cpp`, `src/renderer_rendering.cpp`,
`src/shaders/template.slang` (`learning`), `src/shaders/common_types.slang`, `src/shaders/composite.slang`,
`src/shaders/pbr.slang`, `CMakeLists.txt` and `docs/ARCHITECTURE.md`; the pipeline example of page 2 is the one of the
former `docs/ROADMAP.md` (removed from `main` in `82ed9d1`), the scene of page 4 is `SetupScene()` of the example scene with the two lines of worksheet Aufgabe 3, step 1 (as on
`phong-solution`, which the wiki does not name).
`docs/BUILD.md`, `docs/ROADMAP.md` and `docs/REQUIREMENTS.md` no longer exist on `main`; the end links of all pages
except Begriffe point to `docs/ARCHITECTURE.md` (Begriffe still names `BUILD.md` and `REQUIREMENTS.md` as sources of
nine entries and links them in its last line; open). When the engine changes, the page that quotes the changed text changes with it (the
thesis project keeps the inventory `docs/WIKI_CONTENT.md` with the source line per fact).
