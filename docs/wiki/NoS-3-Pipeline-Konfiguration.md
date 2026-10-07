# NoS 3 Pipeline-Konfiguration

Diese Seite zeigt, wie eine eigene Pipeline aus einer Shader-Datei angelegt wird und wie Objekte ihr zugewiesen werden.

## Beispiel: eine Pipeline anlegen und ein Objekt zuweisen

Zwei Aufrufe an zwei Stellen der Sandbox-Datei (das Beispiel der Engine-Dokumentation, mit dem Klassennamen aus `sandbox.h`):

```cpp
	// in main(), in der Initialisierungskette nach CreatePipelines():
	sandbox.CreatePipelines();                            // the engine's own; "pbr" is every object's default
	sandbox.CreatePipeline("toon", "shaders/toon.slang"); // own pipeline; all four settings at their defaults
	sandbox.CreateCommandBuffers();
```

```cpp
	// in SetupScene(), nachdem das Objekt existiert:
	SceneObject *room = sandbox.LoadModel("Room", "../assets/viking_room/viking_room.gltf");
	room->SetRotation({-90.0f, 0.0f, 0.0f});
	sandbox.AddToPipeline("toon", room);                  // unknown name: error
```

Das Objekt `Room` wird dann mit `shaders/toon.slang` gezeichnet statt mit dem Shader der Engine. Die Shader-Datei liegt in `src/shaders/`; sie beginnt als Kopie von `template.slang` ([[NoS 5|NoS-5-Shader-Schnittstelle]]). Nach dem Anlegen der Datei: noch einmal konfigurieren ([[Setup]]).

## `CreatePipeline`

```cpp
bool CreatePipeline(const std::string &name, const std::string &shaderFile, const PipelineSettings &settings = {});
```

- Nicht Teil der Kette, braucht aber `CreatePipelines()`: *an own pipeline shares the layout of the engine's pipelines, and they do not exist yet.* Der Aufruf steht in der Kette nach `CreatePipelines()` oder in `SetupScene()` – in jedem Fall, bevor das Rendering beginnt.
- `name`: der Name, den `AddToPipeline()` benutzt. `"pbr"` ist die Pipeline der Engine und reserviert.
- `shaderFile`: `"shaders/x.slang"`; geladen wird `"shaders/x.spv"`, das der Build daraus erzeugt. Die Datei braucht die Einstiegspunkte `VSMain` und `PSMain`.
- `settings`: die drei Einstellungen unten; `{}` sind die Standardwerte.

Eine Pipeline je Shader-Datei, beliebig viele Objekte je Pipeline; eine Pipeline wird einmal erzeugt und ändert sich nicht mehr; Objekte besitzen keine Pipelines.

## Die vier Dinge, die eine eigene Pipeline ausmachen

```cpp
enum class CullMode { None, Front, Back };

struct PipelineSettings
{
	CullMode cullMode  = CullMode::None;
	bool     depthTest = true;
	bool     blending  = false;        // true: no depth writes, drawn in the transparent pass
};
```

| Einstellung | Bedeutung |
|---|---|
| die Shader-Datei | Vertex- und Fragment-Shader in einer Datei (`VSMain`, `PSMain`) |
| `cullMode` | welche Seite eines Dreiecks **nicht** gezeichnet wird; Vorderseiten sind die gegen den Uhrzeigersinn aufgezählten (`None`: beide Seiten zeichnen) |
| `depthTest` | an: Tiefenvergleich `LessOrEqual`, schreibt Tiefe – außer wenn `blending` an ist |
| `blending` | an: Alpha-Blending (*source alpha / one minus source alpha*), keine Tiefenschreibzugriffe, gezeichnet im transparenten Pass direkt auf das Swap-Chain-Bild |

Eine Einstellung, die nicht der Standard ist: `sandbox.CreatePipeline("outline", "shaders/outline.slang", {.cullMode = CullMode::Front});`

Alles andere legt die Engine fest: Vertex-Layout, Topologie (Triangle List), Multisampling, das Pipeline-Layout (das der PBR-Pipeline – deshalb bekommt jede eigene Pipeline genau die Shader-Eingaben der Seite [[NoS 5|NoS-5-Shader-Schnittstelle]]), die Attachment-Formate, Viewport und Scissor.

## `AddToPipeline`

```cpp
bool AddToPipeline(const std::string &name, SceneObject *object);
```

- Fügt alle Teile des Objekts hinzu (einen Teil über `Part()`, [[NoS 4|NoS-4-Geometrie-und-Assets]]); braucht die vollständige Kette.
- Ein Objekt, das keiner Pipeline zugewiesen ist, wird mit `"pbr"` gezeichnet.
- Ein Objekt in mehreren Pipelines wird einmal je Pipeline gezeichnet, in der Reihenfolge, in der die Pipelines erzeugt wurden (`"pbr"` zuerst, falls es mit `AddToPipeline("pbr", …)` hinzugefügt wurde) – etwa Shading plus Umriss.
- Unbekannter Name: `false` und `AddToPipeline: unknown pipeline "<name>" for <entity>. Known pipelines: "pbr", ...`

## Meldungen

| Fall | Zeile |
|---|---|
| Erfolg (stdout) | `Pipeline "<name>" created from <file>` |
| reservierter Name | `CreatePipeline("pbr"): the name "pbr" is reserved for the engine's own pipelines` |
| Name schon vergeben | `CreatePipeline("<name>"): a pipeline with this name already exists` |
| zu früh | die Kettenansicht mit *an own pipeline shares the layout of the engine's pipelines, and they do not exist yet.* |
| `.spv` fehlt (Datei neu, nicht konfiguriert, oder Build fehlgeschlagen) | `Failed to create pipeline "<name>" from <file>: Failed to open file: shaders/<x>.spv` |

## So zeigt die Terminalausgabe eine Pipeline

Der Block der eigenen Pipeline aus dem Beispiel oben; die vier Einstellungen sind `yours`, der Rest `fixed`:

```
Pipeline "toon"    shaders/toon.slang -> shaders/toon.spv

  Input assembly     triangle list, engine vertex layout        fixed
        |
  Vertex shader      VSMain                                     yours
        |
  Rasterization      cull mode: none                            yours
        |
  Fragment shader    PSMain                                     yours
        |
  Depth test         on, writes depth                           yours
        |
  Color blending     off                                        yours
        |
  Attachments        off-screen color image + depth image       fixed

  Objects            Room
```

Mit `blending` an ändern sich drei Zeilen: `Depth test  on, no depth writes`, `Color blending  on, drawn in the transparent pass`, `Attachments  swap chain image + depth image`. Die Pipeline der Engine, `"pbr"`, zeigt `cull mode: back`, `off (on for blended materials)` und in jeder Zeile `fixed`; undurchsichtig, gemischt oder Glas wählt sie aus dem glTF-Material. Wie der ganze Block gelesen wird: [[Pipeline-Ansicht|Pipeline-Ansicht]].

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *Named pipelines*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#named-pipelines-planned-change-4-own-code), [`docs/ROADMAP.md`, *Pipelines and shaders*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ROADMAP.md#pipelines-and-shaders).
