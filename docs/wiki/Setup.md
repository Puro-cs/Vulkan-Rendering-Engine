# Setup

Diese Seite sagt, was installiert sein muss und wie die Engine konfiguriert, gebaut und gestartet wird – in Visual Studio und auf der Kommandozeile.

## Voraussetzungen

| Komponente | Version / Ort |
|---|---|
| Visual Studio 2026 mit MSVC (Generator `Visual Studio 18 2026`, x64) | im Pool installiert |
| CMake ≥ 3.29 | Bestandteil der Visual-Studio-Installation oder eigenständig |
| Vulkan SDK ≥ 1.4.350.0 | Umgebungsvariable `VULKAN_SDK` zeigt auf die Installation; das SDK liefert `slangc`, `spirv-opt` und die Validation Layers |
| vcpkg | Umgebungsvariable `VCPKG_ROOT` zeigt auf die Installation; Manifest-Modus, die Pakete stehen in `vcpkg.json` |
| GPU mit Vulkan 1.3 und dem Feature `dynamicRendering` | sonst bricht der Start mit `Failed to find a suitable GPU. Make sure your GPU supports Vulkan and has the required extensions.` ab |

Beide Umgebungsvariablen, `VULKAN_SDK` und `VCPKG_ROOT`, braucht jeder der beiden Wege unten. Die Abhängigkeiten (glfw3, glm, ktx, nlohmann-json, tinygltf, stb, zstd) installiert vcpkg beim ersten Konfigurieren selbst; das braucht Netz und dauert einige Minuten. Vulkan-Header, Loader und `vulkan.hpp` kommen aus dem SDK, Dear ImGui liegt im Repository.

## Konfiguration: Debug

Gebaut und gestartet wird die **Debug**-Konfiguration. Nur in Debug sind die Validation Layers eingeschaltet (`ENABLE_VALIDATION_LAYERS`, ein Schalter beim Kompilieren), und nur dort entspricht die Terminalausgabe der, die das Aufgabenblatt beschreibt.

## Weg 1: Visual Studio

1. Repository klonen und den Branch für das Aufgabenblatt auschecken:
   ```
   git clone https://github.com/Puro-cs/Vulkan-Rendering-Engine
   cd Vulkan-Rendering-Engine
   git checkout learning
   ```
2. In Visual Studio **Ordner öffnen** (*Open a local folder*) und den Repository-Ordner wählen. Visual Studio liest `CMakePresets.json` und konfiguriert mit dem Preset `windows-msvc`; dabei installiert vcpkg die Pakete.
3. Konfiguration **Debug** (`windows-msvc-debug`) wählen, Ziel `VulkanRenderEngine.exe`, bauen.
4. Starten (F5 oder Strg+F5). Das Arbeitsverzeichnis ist auf `src/` eingestellt (`VS_DEBUGGER_WORKING_DIRECTORY`); das Konsolenfenster, das mit der Exe aufgeht, ist das Terminal, in dem die Ausgabe steht und in das Terminalbefehle getippt werden.

## Weg 2: Kommandozeile (aus dem Repository-Ordner)

```
cmake --preset windows-msvc
cmake --build --preset windows-msvc-debug
cd src
..\build\windows-msvc\Debug\VulkanRenderEngine.exe
```

Die erste Zeile konfiguriert (einmal; beim ersten Mal mit der vcpkg-Installation), die zweite baut, die letzten beiden starten **aus `src/`**: die Exe findet `shaders/*.spv` und `../assets/...` relativ zum Arbeitsverzeichnis. Aus einem anderen Ordner gestartet (etwa per Doppelklick auf die Exe) zeigen die Pfade ins Leere und die erste Shader-Datei wird nicht gefunden.

## Woran ein korrekter Start zu erkennen ist

- ein Fenster 800 × 600, im Titel Frame-Zähler, FPS und ms;
- im Terminal genau **eine** Meldung der Validation Layer (`robustBufferAccess2` ohne `robustBufferAccess`) – sie ist aus dem Tutorial geerbt und kein Fehler; jede andere Meldung ist neu;
- bei der Beispielszene zwei harmlose Warnungen des Model-Loaders für den Viking Room (`Warning: No decoded bytes for baseColor texture index 0`, `Warning: Failed to extract punctual lights from ...`);
- `Renderer: Ending load cycle without completion mark. Forcing completion to avoid deadlock.` ist das normale Ende des Ladens;
- danach die Ausgabe `Initialization chain:` mit acht `[ok]`-Zeilen und ein Block je Pipeline (siehe [[Pipeline-Ansicht|Pipeline-Ansicht]]).

## Nach einer neuen Shader-Datei: noch einmal konfigurieren

Die Liste der `*.slang`-Dateien wird beim Konfigurieren eingesammelt. Nach dem Anlegen oder Löschen einer Shader-Datei in `src/shaders/` deshalb **noch einmal konfigurieren** (`cmake --preset windows-msvc`; in Visual Studio den CMake-Cache neu erzeugen) und dann bauen. Ein Build-Baum, der eine gelöschte Shader-Datei noch kennt, scheitert im Ziel `shaders`.

Die kompilierten Shader (`*.spv`) liegen nicht im Repository: der Build erzeugt sie mit `slangc` aus dem Vulkan SDK und kopiert sie nach `src/shaders/`. Eine `.spv`-Datei wird nie von Hand angefasst; ihr Name taucht nur im Pipeline-Block der Terminalausgabe auf (`x.slang -> x.spv`).

Engine-Dokumentation: [`docs/BUILD.md`](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/BUILD.md) (*Toolchain*, *Commands*, *Run*, *Expected console output in Debug*).
