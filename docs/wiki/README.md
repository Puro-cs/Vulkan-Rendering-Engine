# docs/wiki — the pages of the GitHub wiki

These Markdown files are the pages of the repository's GitHub wiki (written 2026-10-07 from the thesis project,
`C:\ClaudeWorkspace\BA`, plan step 4). They live here only until they are pushed to the wiki repository; the wiki is
the place students read them. File name = wiki page name (hyphens become spaces in the page title); `Home.md` is the
start page, `_Sidebar.md` the page list shown beside every page.

| File | Page |
|---|---|
| `Home.md` | Start |
| `Setup.md` | Setup |
| `Architektur.md` | Architektur (KI 1–8) |
| `NoS-1-Initialisierungskette.md` … `NoS-6-Kamera-und-Licht.md` | the six NoS pages |
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

Engine facts on the pages come from `docs/ARCHITECTURE.md`, `docs/BUILD.md`, `docs/ROADMAP.md`, `src/sandbox.h`,
`src/sandbox.cpp`, `src/sandbox_impl.cpp` and the shaders at commit `cf78c92`; the Begriffe definitions from the
Khronos documentation pages linked at the end of that page. When the engine changes, the page that quotes the
changed text changes with it (the thesis project keeps the inventory `docs/WIKI_CONTENT.md` with the source line per fact).
