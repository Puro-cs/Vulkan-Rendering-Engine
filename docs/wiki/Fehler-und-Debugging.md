# Fehler und Debugging

Diese Seite ist die Liste der Fehler, die beim Arbeiten mit der Engine auftreten: wie jeder aussieht, und auf welcher Seite die richtige Verwendung steht.

## Wo die Ausgabe hingeht

Fehler und Warnungen gehen auf **stderr**, alles andere auf **stdout** – beides im selben Terminal (in Visual Studio das Konsolenfenster der Exe). Um sie getrennt zu lesen, aus `src/`:

```
cmd /c "..\build\windows-msvc\Debug\VulkanRenderEngine.exe > ..\build\run_stdout.txt 2> ..\build\run_stderr.txt"
```

## Die Validation Layer

Meldungen der Validation Layer stehen als `Validation layer: <message>` im Terminal – der Text der Layer selbst, nur mit Präfix, nicht übersetzt; Warnungen und Fehler auf stderr, die (sehr gesprächigen) Info-Meldungen auf stdout. Nur in **Debug**-Builds ([[Setup]]). Bei jedem Debug-Start kommt genau **eine** Meldung (`robustBufferAccess2` ohne `robustBufferAccess`): sie ist aus dem Tutorial geerbt und kein Fehler. Sobald eine eigene Pipeline aus dem Template existiert, kommt eine zweite dazu, `Vertex attribute at location 3 not consumed by vertex shader`: das Template liest die Tangente nicht ([[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]]); auch sie ist erwartet. Jede andere Meldung ist neu und gehört zum eigenen Code. Der Begriff: [[Begriffe]].

## Die Fehler

| Fehler | So sieht er aus | Richtige Verwendung |
|---|---|---|
| Reihenfolge der Initialisierungskette: ein Aufruf zu früh, zweimal oder ohne die vor ihm | `Initialization error: <Call>() cannot run yet. Initialization stopped.` / `... was called twice ...`, dann die Kettenansicht mit der `[!!]`-Zeile; danach druckt kein Aufruf mehr etwas, und das Programm endet normal | lesen: [[Pipeline-Ansicht|Pipeline-Ansicht]]; die acht Aufrufe und ihre Begründungen: [[1 Initialisierungskette und Frame-Ablauf|1-Initialisierungskette-und-Frame‐Ablauf]] |
| Reihenfolge des Frame-Ablaufs: ein Aufruf fehlt, steht falsch oder doppelt; `EndFrame()` vergessen | `Frame sequence error: <Call>() cannot run yet. Rendering stopped.` / `... was called twice in this frame ...`, dann die Frame-Ansicht; das Fenster schließt sich, das Programm endet normal ohne Crash Dump | lesen: [[Pipeline-Ansicht|Pipeline-Ansicht]]; die sechs Aufrufe und ihre Begründungen: [[1 Initialisierungskette und Frame-Ablauf|1-Initialisierungskette-und-Frame‐Ablauf]] |
| Ausnahme in einem Frame-Aufruf | `Exception: <text>`, dann die Frame-Ansicht mit `failed` | je nach Text; die Begriffe: [[Begriffe]] |
| Keine aktive Kamera | `IsRunning(): there is no active camera. Create one with CreateCamera() and pass it to SetActiveCamera() in SetupScene().` – die Schleife startet nie | `SetActiveCamera()`: [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]] |
| Szenenaufruf vor der vollständigen Kette (`LoadModel`, `CreateSphere`, `AddToPipeline`) | die Kettenansicht mit *the meshes and textures of the scene go into buffers and images on the GPU … The initialization chain is not complete.* | `SetupScene()` steht nach der Kette: [[1 Initialisierungskette und Frame-Ablauf|1-Initialisierungskette-und-Frame‐Ablauf]], [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]] |
| `CreatePipeline()` vor `CreatePipelines()` | die Kettenansicht mit *an own pipeline shares the layout of the engine's pipelines, and they do not exist yet.* | [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]] |
| Pipeline-Name reserviert, doppelt oder unbekannt | `CreatePipeline("pbr"): the name "pbr" is reserved …` / `CreatePipeline("<name>"): a pipeline with this name already exists` / `AddToPipeline: unknown pipeline "<name>" for <entity>. Known pipelines: "pbr", ...` | [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]] |
| `.spv` fehlt: neue Shader-Datei ohne erneutes Konfigurieren, oder Build fehlgeschlagen | `Failed to create pipeline "<name>" from <file>: Failed to open file: shaders/<x>.spv` | noch einmal konfigurieren und bauen: [[Setup]], [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]]; der Build der Shader: [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]] |
| Slang-Fehler im Shader | erscheint beim **Bauen** als Fehler des Ziels `shaders` in der Build-Ausgabe (Datei, Zeile, Meldung), nicht zur Laufzeit; die `.spv` wird erst nach einem fehlerfreien Build kopiert, bis dahin lädt die Engine die letzte gute `.spv` | [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]] |
| Shader ohne `VSMain` / `PSMain` | der Build bzw. das Anlegen der Pipeline scheitert; jede Datei braucht beide Einstiegspunkte | [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]] |
| Modell lädt nicht | `GLTF Error: ...` oder `Failed to parse GLTF file: ...`, dann `LoadModel: "<file>" could not be loaded; the object "<name>" is empty`; der Rest der Szene wird gezeigt | Pfad relativ zu `src/`, Format: [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]], [[Eigene Modelle|Eigene-Modelle]] |
| Unbekanntes Material in `Part()` | `Part: the object "<name>" has no material "<material>". Its materials: "..."` | [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]] |
| Textur ist kein KTX2 | `Non-KTX2 images are not supported by the custom image loader (use KTX2).` und `Warning: No decoded bytes for baseColor texture index 0`; die Geometrie erscheint mit der Standard-Albedo. Beim Viking Room sind diese Warnung und `Warning: Failed to extract punctual lights from ...` normal | [[Eigene Modelle|Eigene-Modelle]] |
| Terminalbefehl nicht verstanden | `Terminal: <problem> Commands: Name.Move(x, y, z), Name.Rotate(x, y, z) in degrees, Name.Scale(x, y, z). Names: "...", ...` | [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]] |
| Watchdog | `WATCHDOG: APPLICATION HAS HUNG!` … `Aborting to generate stack trace...`, wenn 10 s lang kein Frame fertig wird (etwa ein Modell, dessen Parsen länger dauert); die Startzeile `[Watchdog] Started - will abort if no frame updates for 10+ seconds (60s during loading)` nennt eine längere Frist, die nicht mehr gesetzt wird | kleineres Modell; [[Architektur]] |
| Die Kugel eines eigenen Shaders ist auf der Lichtseite gleichmäßig hell, ein Glanzlicht ist nicht zu sehen | kein Fehler in der Engine-Konsole; die Rückgabe von `PSMain` liegt dort in einem Kanal über 1 und wird abgeschnitten, etwa mit der Sonne der Beispielszene (Intensität 3) | der Weg vom Rückgabewert zum Bild: [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]]; die Bedingungen der Referenzgrafiken: [[4 Materialien und Referenzgrafiken|4-Materialien-und-Referenzgrafiken]] |
| Das Bild eines eigenen Shaders ist nicht wie erwartet (schwarz, falsche Seite hell, Flecken) | kein Fehler in der Engine-Konsole | einen Zwischenwert als Farbe ausgeben und die Eingaben prüfen: [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]] |
| Absturz | ein Minidump in `src/crashes/` (dem Arbeitsverzeichnis) | — |
| Build: `MSB8028` (*stale object files*) | MSBuild warnt bei jedem Build; die Exe kann mit gemischten Objektdateien starten und abstürzen | `build/windows-msvc/VulkanRenderEngine.dir/<Config>/` löschen oder in Visual Studio *Rebuild*; `MSB8028` nie ignorieren |
| Build: `slangc not found. Skipping shader compilation step.` beim Konfigurieren | der Build läuft ohne Shader durch, die Exe scheitert in `CreatePipelines()`, weil `shaders/pbr.spv` fehlt | Vulkan SDK installieren, `VULKAN_SDK` setzen, neu konfigurieren: [[Setup]] |
| Build: eine gelöschte Shader-Datei wird noch gebaut | das Ziel `shaders` scheitert | noch einmal konfigurieren: [[Setup]] |

## Verhalten, das wie ein Fehler aussieht, aber keiner ist

- Der Schieberegler *Sampler max anisotropy* erzeugt in Debug Meldungen der Validation Layer (Verhalten des Tutorials).
- Der Schieberegler *Exposure* hat unter 0,2 keine Wirkung; er wirkt nur auf die undurchsichtige Szene.
- Sechs Leerzeilen nach `Features queried successfully` im Start-Log.
- `Renderer: Ending load cycle without completion mark. Forcing completion to avoid deadlock.` ist das normale Ende des Ladens.
- Ein Objekt, das verschwindet, wenn es klein auf dem Bildschirm oder weit weg ist: das Distanz-LOD ([[Architektur]]).

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *Terminal output*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#terminal-output-planned-change-9-own-code) und [*Build files*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#build-files).
