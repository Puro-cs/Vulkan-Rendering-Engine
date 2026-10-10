# 2 Eigene Pipeline und eigene Szene

Diese Seite ist das Nachschlagewerk zu Aufgabe 2: eine eigene Pipeline aus einer Shader-Datei und ihren Einstellungen anlegen, ihr Objekte zuweisen und eine Szene aus Kamera, Lichtern und Objekten aufbauen, dazu der Block, den die Engine-Konsole für jede Pipeline druckt.

## Beispiel 1: die Szene der Beispielszene

Aus der Beispielszene (`src/sandbox.cpp`, Branch `main`):

```cpp
void SetupScene(Sandbox &sandbox)
{
	// Create a camera and make it the active one
	Camera *camera = sandbox.CreateCamera("Camera");
	camera->SetPosition({2.0f, 0.5f, -2.0f});
	camera->SetRotation({0.0f, 135.0f, 0.0f});
	sandbox.SetActiveCamera(camera);

	// Create a directional light. A light shines along the -Z axis; the rotation turns it down and sideways.
	Light *sun = sandbox.CreateLight("Sun", LightType::Directional);
	sun->SetRotation({-45.0f, 45.0f, 0.0f});
	sun->SetIntensity(3.0f);

	// Load a model. The call waits until the model is loaded.
	SceneObject *room = sandbox.LoadModel("Room", "../assets/viking_room/viking_room.gltf");
	room->SetRotation({-90.0f, 0.0f, 0.0f});

	// Create a simple mesh that needs no file
	SceneObject *sphere = sandbox.CreateSphere("Sphere", 0.2f);
	sphere->SetPosition({0.5f, 0.3f, -1.2f});
}
```

Alle Objekte der Szene entstehen in `SetupScene()`, nach der Kette und vor der Render-Schleife, in beliebiger Reihenfolge. Die Zeiger, die die `Create…`- und `Load…`-Aufrufe zurückgeben, gehören der Engine und bleiben gültig, bis die `Sandbox` zerstört wird. Jede Position, Rotation, Skalierung und Farbe ist ein `glm::vec3`, etwa `{0.5f, 0.3f, -1.2f}`; Winkel stehen in Grad. Die y-Achse zeigt nach oben, und eine Kamera ohne Rotation blickt entlang der −z-Achse.

## Beispiel 2: eine eigene Pipeline anlegen und ein Objekt zuweisen

Zwei Aufrufe an zwei Stellen der Sandbox-Datei:

```cpp
	// in main(), in der Initialisierungskette direkt nach CreatePipelines():
	sandbox.CreatePipelines();                            // the engine's own; "pbr" is every object's default
	sandbox.CreatePipeline("toon", "shaders/toon.slang");
	sandbox.CreateCommandBuffers();
```

```cpp
	// in SetupScene(), nachdem das Objekt existiert:
	SceneObject *room = sandbox.LoadModel("Room", "../assets/viking_room/viking_room.gltf");
	room->SetRotation({-90.0f, 0.0f, 0.0f});
	sandbox.AddToPipeline("toon", room);                  // unknown name: error
```

Das Objekt `Room` wird dann mit `shaders/toon.slang` gezeichnet statt mit der Pipeline `"pbr"` der Engine. Eine Pipeline mit einer Einstellung, die nicht der Standard ist:

```cpp
	sandbox.CreatePipeline("outline", "shaders/outline.slang", {.cullMode = CullMode::Front});
```

## Die eigene Pipeline

### Die Shader-Datei

- Eine eigene Shader-Datei beginnt als Kopie von `src/shaders/template.slang` unter neuem Namen im selben Ordner. Was in ihr steht und welche Eingaben sie bekommt: [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]].
- Der Build übersetzt jede `.slang`-Datei in `src/shaders/` außer den vier Modulen `common_types`, `pbr_utils`, `lighting_utils` und `tonemapping_utils` in eine `.spv`-Datei, die die Pipeline lädt. Die Liste der Shader-Dateien sammelt CMake nur beim Konfigurieren ein. Nach dem Anlegen einer Shader-Datei wird deshalb neu konfiguriert (`cmake --preset windows-msvc`, in Visual Studio den CMake-Cache neu erzeugen) und dann gebaut ([[Setup]]).
- Eine Pipeline aus dem unveränderten Template zeichnet ihre Objekte texturiert, aber unbeleuchtet: `PSMain` gibt die Farbe der Basisfarbtextur unverändert zurück. Ein Objekt ohne eigene Textur, etwa eine Kugel, bekommt die weiße Standardtextur der Engine und erscheint einfarbig.
- Sobald eine Pipeline aus dem Template existiert, meldet der Validation Layer in Debug bei jedem Start zusätzlich `Vertex attribute at location 3 not consumed by vertex shader`: Das Template liest das Vertex-Attribut an Location 3, die Tangente, nicht. Die Meldung ist erwartet.

### `CreatePipeline`

```cpp
bool CreatePipeline(const std::string &name, const std::string &shaderFile, const PipelineSettings &settings = {});
```

| Parameter | Bedeutung |
|---|---|
| `name` | der Name der Pipeline, den `AddToPipeline()` benutzt; frei gewählt, aber nur einmal vergeben. `"pbr"` ist die Pipeline der Engine und reserviert |
| `shaderFile` | die Shader-Datei relativ zu `src/`, etwa `"shaders/toon.slang"`. Geladen wird die `.spv`, die der Build daraus erzeugt (`shaders/toon.spv`). Die Datei braucht die Einstiegspunkte `VSMain` und `PSMain` |
| `settings` | Cull Mode und Tiefentest (siehe unten); ohne dritten Parameter gelten die Standardwerte |

- `CreatePipeline` ist kein Aufruf der Kette, braucht aber `CreatePipelines()`: *an own pipeline shares the layout of the engine's pipelines, and they do not exist yet.* Der Aufruf steht nach `CreatePipelines()` und vor der Render-Schleife.
- Er gibt `true` zurück, wenn die Pipeline angelegt wurde, und die Engine-Konsole druckt dann `Pipeline "<name>" created from <shaderFile>`.
- Eine eigene Pipeline übernimmt das Pipeline-Layout von `"pbr"`. Ihr Shader bekommt deshalb genau die Eingaben, die die Engine jedem Shader liefert ([[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]]).
- Die Engine liest die `.spv` beim Anlegen der Pipeline, also beim Start. Eine Änderung der Shader-Datei wirkt deshalb nach dem nächsten Bauen und Starten.

### Die Einstellungen

Aus `src/pipeline_settings.h`, das `sandbox.h` einbindet:

```cpp
enum class CullMode
{
	None,
	Front,
	Back
};

struct PipelineSettings
{
	CullMode cullMode  = CullMode::None;
	bool     depthTest = true;
};
```

| Einstellung | Standard | Bedeutung | Zeile im Block der Pipeline |
|---|---|---|---|
| `cullMode` | `CullMode::None` | welche Seite eines Dreiecks nicht gezeichnet wird. Die Vorderseite ist die Seite, von der aus die Ecken des Dreiecks gegen den Uhrzeigersinn umlaufen; die Modelle und Kugeln der Engine haben ihre Außenseite als Vorderseite. `None` zeichnet beide Seiten, `Front` lässt die Vorderseiten weg, `Back` die Rückseiten | `Rasterization` mit `cull mode: none`, `front` oder `back` |
| `depthTest` | `true` | an: jedes Fragment wird mit der Tiefe verglichen (`LessOrEqual`) und schreibt seine Tiefe; aus: kein Vergleich und keine Tiefe | `Depth test` mit `on, writes depth` oder `off` |

Eine Einstellung wird über den dritten Parameter mit ihrem Namen gesetzt, wie im Beispiel 2: `{.cullMode = CullMode::Front}`. Eine Einstellung, die dort nicht genannt ist, behält ihren Standardwert. Alles andere legt die Engine fest: *vertex layout, topology, blending, multisampling, pipeline layout, attachment formats*.

### `AddToPipeline`

```cpp
bool AddToPipeline(const std::string &name, SceneObject *object);
```

- Weist alle Teile eines Objekts der Pipeline `name` zu. Der Aufruf steht in `SetupScene()`, nachdem das Objekt angelegt ist, und braucht die vollständige Kette.
- *An object that was added to no pipeline is drawn with "pbr". An object that was added to several pipelines is drawn once per pipeline, in the order in which the pipelines were created.*
- Ein unbekannter Name gibt `false` zurück und druckt `AddToPipeline: unknown pipeline "<name>" for <entity>. Known pipelines: "pbr", ...`.

## Die Szene

### Kamera

```cpp
Camera *CreateCamera(const std::string &name);
void SetActiveCamera(Camera *camera);
```

| Aufruf auf `Camera` | Bedeutung |
|---|---|
| `SetPosition(const glm::vec3 &position)` | Position |
| `SetRotation(const glm::vec3 &degrees)` | Drehung um die x-, y- und z-Achse in Grad; *Without a rotation the camera looks along the -Z axis.* |
| `SetFieldOfView(float degrees)` | Öffnungswinkel in Grad, Standard 45° |

`SetActiveCamera()` legt fest, mit welcher Kamera die Szene gerendert wird; ohne aktive Kamera startet die Render-Schleife nicht ([[1 Initialisierungskette und Frame-Ablauf|1-Initialisierungskette-und-Frame‐Ablauf]]). Gezeichnet wird, was zwischen 0,1 und 100 Einheiten vor der Kamera liegt. Während die Engine läuft, bewegt ihre Kamerasteuerung die aktive Kamera (die Tasten stehen auf der Seite [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]]); Position und Rotation aus `SetupScene()` sind der Ausgangszustand bei jedem Start.

### Licht

```cpp
enum class LightType
{
	Directional,
	Point,
	Spot
};

Light *CreateLight(const std::string &name, LightType type);
```

| Aufruf auf `Light` | gilt für | Bedeutung |
|---|---|---|
| `SetPosition(const glm::vec3 &position)` | `Point`, `Spot` | Position |
| `SetRotation(const glm::vec3 &degrees)` | `Directional`, `Spot` | Drehung in Grad, die die Richtung des Lichts bestimmt; *Without a rotation the light shines along the -Z axis.* Bei einem Punktlicht wirkt sie nicht |
| `SetDirection(const glm::vec3 &direction)` | `Directional`, `Spot` | die Richtung, in die das Licht scheint; ersetzt die Rotation und muss nicht normalisiert sein |
| `SetColor(const glm::vec3 &color)` | alle | Farbe, Standard Weiß |
| `SetIntensity(float intensity)` | alle | *The color is multiplied by it.* Standard 1 |
| `SetRange(float range)` | `Point`, `Spot` | Reichweite, Standard 100 |
| `SetConeAngles(float innerDegrees, float outerDegrees)` | `Spot` | innerer und äußerer Kegelwinkel in Grad, Standard 0° und 45° |

Ein gerichtetes Licht (`Directional`) hat nur eine Richtung; seine Position spielt keine Rolle. Die Pipeline `"pbr"` schwächt ein gerichtetes Licht nicht ab, das Licht eines Punkt- oder Spotlichts dagegen mit dem Quadrat der Entfernung. Ohne Licht zeigt `"pbr"` nur einen schwachen Grundanteil der Oberflächenfarbe. Was von einem Licht im Shader ankommt: [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]].

### Objekte

```cpp
SceneObject *LoadModel(const std::string &name, const std::string &file);
SceneObject *CreateSphere(const std::string &name, float radius);
```

- `LoadModel` lädt eine glTF-Datei (`.gltf` mit der `.bin` daneben, oder `.glb`), deren Texturen KTX2-Dateien sind ([[Eigene Modelle|Eigene-Modelle]]). Der Pfad ist relativ zum Arbeitsverzeichnis `src/`, daher `"../assets/..."`. Der Aufruf wartet, bis das Modell geladen ist. Das einzige Modell im Repository ist der Viking Room, `"../assets/viking_room/viking_room.gltf"`; er liegt in der Datei um +90° um die x-Achse gedreht, daher `SetRotation({-90.0f, 0.0f, 0.0f})` im Beispiel. Misslingt das Laden, druckt die Engine `LoadModel: "<file>" could not be loaded; the object "<name>" is empty`, und das leere Objekt ignoriert jeden Aufruf.
- `CreateSphere` erzeugt eine Kugel mit dem Radius `radius` um den Ursprung, ohne Datei und ohne Textur.
- Der Name (`"Room"`, `"Sphere"`) steht in der Engine-Konsole unter `Objects` und in Terminalbefehlen; jedes Objekt bekommt deshalb einen eigenen Namen.

| Aufruf auf `SceneObject` | Bedeutung |
|---|---|
| `SetPosition(const glm::vec3 &position)` | Position |
| `SetRotation(const glm::vec3 &degrees)` | Drehung um die x-, y- und z-Achse in Grad |
| `SetScale(const glm::vec3 &scale)` | Skalierung |
| `Move(const glm::vec3 &translation)`, `Rotate(const glm::vec3 &degrees)`, `Scale(const glm::vec3 &factors)` | dasselbe relativ zum aktuellen Wert |

### Teile eines Modells und Terminalbefehle

- Ein geladenes Modell besteht aus einem Teil je Material. `SceneObject *Part(const std::string &materialName)` spricht einen Teil an; er heißt `<Objekt>.<Material>`, beim Viking Room `Room.Texture1`. Ein Teil lässt sich einzeln transformieren und einer Pipeline zuweisen. Ein unbekanntes Material druckt `Part: the object "<name>" has no material "<material>". Its materials: "..."` und gibt ein leeres Objekt zurück.
- Während die Engine rendert, bewegen Befehle in der Engine-Konsole Objekte und Lichter, jeweils mit Enter abgeschlossen: `Room.Move(1, 0, 0)`, `Sun.Rotate(0, 30, 0)` (Grad), `Sphere.Scale(2, 2, 2)`. Der Name ist der aus `LoadModel()`, `CreateSphere()` oder `CreateLight()`; Kameras sind nicht ansprechbar, Lichter lassen sich nicht skalieren. Ein Befehl, der gelingt, druckt nichts; jeder andere wird mit einer Zeile `Terminal: …` beantwortet, die die drei Formen und die bekannten Namen nennt. Die Befehle ändern nur die laufende Szene, nicht `SetupScene()`.

## Was die Engine-Konsole für eine Pipeline zeigt

Beim Start druckt die Engine-Konsole unter der Initialisierungskette einen Block je Pipeline: zuerst `"pbr"`, dann die eigenen in der Reihenfolge, in der sie angelegt wurden. Der Block einer eigenen Pipeline `"phong"` aus `shaders/phong.slang` mit der Einstellung `{.cullMode = CullMode::Back}`, der der Raum zugewiesen ist:

```
Pipeline "phong"    shaders/phong.slang -> shaders/phong.spv

  Input assembly     triangle list, engine vertex layout        fixed
        |
  Vertex shader      VSMain                                     yours
        |
  Rasterization      cull mode: back                            yours
        |
  Fragment shader    PSMain                                     yours
        |
  Depth test         on, writes depth                           yours
        |
  Color blending     off                                        fixed
        |
  Attachments        off-screen color image + depth image       fixed

  Objects            Room

```

| Zeile | zeigt |
|---|---|
| Kopfzeile | den Namen, die Shader-Datei und die `.spv`, die der Build daraus erzeugt |
| `Vertex shader`, `Fragment shader` | die Einstiegspunkte `VSMain` und `PSMain` der Shader-Datei, `yours` |
| `Rasterization` | den Cull Mode, `yours` |
| `Depth test` | den Tiefentest, `yours` |
| `Input assembly`, `Color blending`, `Attachments` | was die Engine festlegt, `fixed` |
| `Objects` | die Objekte, die diese Pipeline zeichnet; `none`, wenn sie keines zeichnet |

`yours` steht an den vier Zeilen, die Shader-Datei und Einstellungen einer eigenen Pipeline bestimmen; im Block `"pbr"` steht in jeder Zeile `fixed`. Unter `Objects` von `"pbr"` stehen alle Objekte, die keiner Pipeline zugewiesen sind, und die, die mit `AddToPipeline("pbr", …)` hinzugefügt wurden. Ein einzeln zugewiesener Teil steht dort als `<Objekt>.<Material>`. Die Begriffe der Stufen erklärt die Seite [[Begriffe]], die ganze Ausgabe die Seite [[Pipeline-Ansicht|Pipeline-Ansicht]].

## Meldungen

| Fall | Zeile in der Engine-Konsole |
|---|---|
| Pipeline angelegt | `Pipeline "<name>" created from <file>` |
| reservierter Name | `CreatePipeline("pbr"): the name "pbr" is reserved for the engine's own pipelines` |
| Name schon vergeben | `CreatePipeline("<name>"): a pipeline with this name already exists` |
| `CreatePipeline()` vor `CreatePipelines()` | die Kettenansicht mit *an own pipeline shares the layout of the engine's pipelines, and they do not exist yet.* |
| `.spv` fehlt: neue Shader-Datei ohne neues Konfigurieren, oder der Build ist fehlgeschlagen | `Failed to create pipeline "<name>" from <file>: Failed to open file: shaders/<x>.spv` |
| unbekannter Name in `AddToPipeline()` | `AddToPipeline: unknown pipeline "<name>" for <entity>. Known pipelines: "pbr", ...` |
| Szenenaufruf vor der vollständigen Kette | die Kettenansicht mit *the meshes and textures of the scene go into buffers and images on the GPU, which only the completely initialized engine can create. The initialization chain is not complete.* |

Wie diese Fehler aussehen und wo die richtige Verwendung steht: [[Fehler und Debugging|Fehler-und-Debugging]].

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *Named pipelines*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#named-pipelines-planned-change-4-own-code), [*The sandbox layer*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#the-sandbox-layer-planned-change-5-own-code) und [*Terminal commands*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#terminal-commands-planned-change-6-own-code).
