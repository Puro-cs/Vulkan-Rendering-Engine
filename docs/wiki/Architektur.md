# Architektur

Diese Seite zeigt, wie die Engine aufgebaut ist und was sie in den acht Bereichen der Kern-Infrastruktur (KI 1–8) im Hintergrund erledigt – zum Nachlesen neben den Aufgaben, keine Aufgabe setzt sie voraus.

## Schichten und Besitz

```
sandbox.cpp        die eigene Datei: SetupScene() und main(); bindet nur sandbox.h ein
  Sandbox          sandbox.h / sandbox_impl.cpp: die Engine, wie sandbox.cpp sie sieht – keine Vulkan-Typen
    SceneObject, Camera, Light   Handles auf Entities, im Besitz der Sandbox
    Engine         Platform (GLFW-Fenster, Eingabe-Callbacks), Renderer, ModelLoader, ImGuiSystem, die Entities
      Renderer     alles Vulkan: MemoryPool, ThreadPool, SwapChain, VulkanDevice
```

Jede Gruppe von Vulkan-Schritten ist **ein** Aufruf der Sandbox-Datei (die Seiten NoS 1 bis NoS 6). Was die Sandbox-Datei nicht erreicht, ist die Kern-Infrastruktur dieser Seite: die Vulkan-API selbst, ImGui, Descriptor Sets, die Speicherverwaltung, das Innere der Swap Chain und des Rendering-Aufbaus. Die Engine nutzt Vulkan 1.3 über `vulkan.hpp` (RAII-Handles, dynamischer Dispatch), durchgehend *dynamic rendering* und *synchronization2*, keine Render Passes und Framebuffers; C++20, Slang-Shader, Dear ImGui 1.92.

## Entities und Komponenten

Die Szene besteht aus Entities; jede Entity ist eine Liste von Komponenten:

| Komponente | Inhalt |
|---|---|
| `TransformComponent` | Position, Rotation, Skalierung → Modellmatrix |
| `CameraComponent` | View- und Projektionsmatrix |
| `MeshComponent` | Vertices, Indizes, Instanzen, Material |
| `LightComponent` | Typ, Farbe, Intensität, Reichweite, Kegelwinkel; Position und Richtung kommen aus dem Transform der Entity |
| `TerminalCommandComponent` | wendet einmal pro Frame die getippten Terminalbefehle an |

Ein geladenes glTF-Modell wird zu **einer Entity je Material** (benannt `<Modell>_Material_<Index>_<Materialname>`), alle mit dem Transform des Objekts; `Part()` auf der Seite [[NoS 4|NoS-4-Geometrie-und-Assets]] spricht eine davon an.

## Ein Frame: sechs Aufrufe außen, vier Pässe innen

Die Sandbox-Datei macht sechs Aufrufe je Frame ([[NoS 2|NoS-2-Frame-Ablauf]]) und sieht **einen** Pass. Innen laufen vier: (1) die undurchsichtigen Objekte in ein Off-Screen-Farbbild; (1b) *composite* – dieses Bild wird mit Belichtung (*exposure*) und filmischem Tone Mapping auf das Swap-Chain-Bild gezeichnet; (2) die transparenten Objekte direkt auf das Swap-Chain-Bild; (3) die Bedienoberfläche der Engine. Ein Pass ist ein `beginRendering` … `endRendering`-Paar im Command Buffer mit seinen Attachments (den Bildern, in die gezeichnet wird: Farbe und Tiefe); darin werden beliebig viele Objekte mit beliebig vielen Pipelines gezeichnet. Eine eigene Pipeline wird für die Attachments ihres Passes gebaut – deshalb ändert sich ihre `Attachments`-Zeile in der [[Pipeline-Ansicht|Pipeline-Ansicht]] mit `blending`: aus = Pass 1, an = Pass 2. Die Sandbox-Datei kann keinen Pass hinzufügen.

## Was die Engine sonst noch tut

- **Ein Rasterisierungspfad, ein Engine-Shader.** Jedes Objekt, das keiner eigenen Pipeline zugewiesen ist, wird mit `pbr.slang` gezeichnet (undurchsichtig, gemischt oder Glas, je nach glTF-Material). Entfernt wurden Physik, Audio, glTF-Animation, Ray Query, Forward+, planare Spiegelungen, andere Plattformen und der alte Phong-Pfad – *Phong shading is something the students implement themselves.*
- **Culling und Distanz-LOD.** Ein Objekt außerhalb des Kamera-Frustums wird nicht gezeichnet; ebenso ein Objekt, dessen Hüllkugel auf weniger Pixel projiziert als eine Schwelle (Standard 1,5 px undurchsichtig, 2,5 px transparent, im Panel einstellbar). Es gibt keine LOD-Meshes, nichts wird vereinfacht. Ein kleines oder fernes Objekt, das verschwindet, ist dies und kein Fehler.
- **Laden.** Ein Lade-Overlay deckt die ersten Frames ab, während Meshes und Texturen hochgeladen werden; Texturen werden auf einem Thread-Pool dekodiert; ein Watchdog bricht ab, wenn 10 s lang kein Frame fertig wird; ein Crash Reporter schreibt Minidumps nach `src/crashes/`.

## Die Kern-Infrastruktur KI 1–8

Je Bereich: was Vulkan verlangt, was die Engine davon übernimmt, und was davon in der Sandbox-Datei oder im Terminal noch zu sehen ist.

### KI 1 Initialisierung und Swapchain-Verwaltung

Vulkan verlangt die Erstellung der Instanz samt Debug Messenger und Surface, die Auswahl des physischen Geräts, das logische Gerät mit seinen Queues und die Konfiguration der Swap Chain einschließlich ihrer Neuerstellung bei einer Größenänderung des Fensters. Die Engine: `InitializeWindow` legt das GLFW-Fenster mit vier Eingabe-Callbacks an; `CreateInstance` den Dispatcher, die Instanz, den Debug Messenger und die Surface; `PickDevice` wählt das Gerät (Vulkan 1.3, Queue-Familien, Erweiterungen, Swap-Chain-Unterstützung, `dynamicRendering`; eine diskrete GPU wird bevorzugt), erzeugt das logische Gerät mit Queues und den Memory Pool; `CreateSwapChain` die Swap Chain mit ihren Image Views; bei Resize und wenn Acquire oder Present *out of date* melden, wird die Swap Chain neu erstellt. **Sichtbar:** die Kettenaufrufe 1–4 mit ihren `[ok]`-Zeilen und die Start-Logzeilen (`Creating Vulkan instance...` … `Image views created successfully`).

### KI 2 Dynamic Rendering und Attachments

Anstelle von Render-Pass- und Framebuffer-Objekten nutzt die Engine das *dynamic rendering* von Vulkan 1.3; die Farb- und Tiefen-Attachments und die Layout-Übergänge sind vorkonfiguriert. Die Engine: `InitializeRendering` legt die Attachment-Beschreibungen, das Tiefenbild und die Off-Screen-Farbbilder an; jeder Pass ist ein `beginRendering` / `endRendering`-Paar; Layout-Übergänge sind explizite `pipelineBarrier2`-Aufrufe; Farbe wird auf Schwarz (0, 0, 0, 1) gelöscht, Tiefe auf 1,0. **Sichtbar:** Kettenaufruf 5; `BeginRendering` (*clear the color and depth attachments*); die `Attachments`-Zeile jedes Pipeline-Blocks.

### KI 3 Frames in Flight

Für die parallele Auslastung von CPU und GPU werden mehrere Frames gleichzeitig in Bearbeitung gehalten, mit je eigenen Command Buffers, Fences und Semaphores. Die Engine: `MAX_FRAMES_IN_FLIGHT = 2`; je Frame-Slot Command Buffer, *image-available*- und *render-finished*-Semaphore und eine *in-flight*-Fence; eine Timeline-Semaphore für Uploads. **Sichtbar:** Kettenaufrufe 7–8; `BeginFrame` wartet, bis die GPU mit dem Frame-Slot fertig ist; der Begründungstext von `UpdateScene` (*the GPU may still be reading them*).

### KI 4 Explizites Memory Management

Vulkan überlässt die Speicherverwaltung vollständig dem Entwickler. Die Engine: ein `MemoryPool`, von `PickDevice` angelegt, unterallokiert Gerätespeicher für alle Buffer und Bilder. **Sichtbar:** *memory pool* in der Beschreibung von `PickDevice`; Modelle und Kugeln ohne eine Zeile Buffer-Code.

### KI 5 Pipeline-Generierung und Shader-Kompilierung

Vulkan verlangt das Befüllen monolithischer Pipeline-Strukturen und `VkShaderModule`-Instanzen aus SPIR-V. Die Engine: CMake übersetzt beim Bauen jede `src/shaders/*.slang` außer den vier Modulen mit `slangc -target spirv -profile spirv_1_3` und `spirv-opt`; zur Laufzeit lädt `createShaderModule()` das SPIR-V; `CreatePipelines` baut die eigenen Pipelines der Engine (PBR undurchsichtig / gemischt / Glas, composite); `CreatePipeline` macht aus den vier Einstellungen eine vollständige Pipeline und kopiert alles andere von der undurchsichtigen PBR-Pipeline. **Sichtbar:** die Aufrufe der Seite [[NoS 3|NoS-3-Pipeline-Konfiguration]]; der Pipeline-Block mit `<Datei>.slang -> <Datei>.spv`; das erneute Konfigurieren nach einer neuen Shader-Datei.

### KI 6 Descriptor-Management und UBO-Mapping

Die Übersetzung der Szenendaten in den GPU-Speicher: Descriptor Pools, Layouts der Descriptor Sets, das Memory-Mapping der Uniform Buffer Objects. Die Engine: Descriptor Pool (`CreateCommandBuffers`), PBR-Descriptor-Set-Layouts (`CreatePipelines`), je Entity und Frame Uniform Buffers, dauerhaft gemappt; verzögerte Descriptor-Updates, wenn Texturen eintreffen; Materialfaktoren als Push Constants; die Lichter als Storage Buffer, jeden Frame hochgeladen. **Sichtbar:** die festen Shader-Eingaben mit ihren Set- und Binding-Nummern ([[NoS 5|NoS-5-Shader-Schnittstelle]]).

### KI 7 Eingabeverwaltung und Panel

Die nativen Eingabe-Events des Windowing-Systems (GLFW) bleiben in der Engine; sie stellt eine Kamerasteuerung und ein Bedienfeld bereit. Die Engine: GLFW-Callbacks; Tasten `W`/↑ vor, `S`/↓ zurück, `A`/← links, `D`/→ rechts, `Q`/Bild↑ hoch, `E`/Bild↓ runter, linke Maustaste gedrückt dreht den Blick; `Esc` ist nicht belegt (Fenster schließen beendet); angewandt in `UpdateScene`; über einem Panel geht die Eingabe an ImGui. Das Panel *Renderer*: *Culling & LOD* (Frustum Culling, Distance LOD, zwei Schwellen, Sampler-Anisotropie, Culling-Statistik) und *Tone Mapping & Tuning* (*Exposure*, Schieberegler 0,1–4,0, Standard **1,2**; der Code begrenzt nach unten auf 0,2). **Sichtbar:** Tasten, Maus, das Panel.

### KI 8 Kapselung der Textur-Pipeline

Der ganze Lebenszyklus einer Textur – Staging, Layout-Übergänge, Mipmaps, Image View, Sampler. Die Engine: libktx dekodiert auf dem Thread-Pool; Staging-Upload; sRGB- oder lineares Format; gemeinsame Standardtexturen; Descriptor-Refresh, wenn eine Textur eintrifft. **Nur KTX2:** ein glTF, das PNG oder JPEG referenziert, lädt seine Geometrie mit der Standard-Albedo und einer Warnung. **Sichtbar:** die KTX2-Regel und die Warnungen ([[Eigene Modelle|Eigene-Modelle]]).

Engine-Dokumentation: [`docs/ARCHITECTURE.md`](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md) (*Ownership*, *The initialization chain*, *One frame*, *What a mesh shader gets from the engine*), [`docs/REQUIREMENTS.md`](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/REQUIREMENTS.md).
