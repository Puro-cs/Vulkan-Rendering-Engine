# NoS 4 Geometrie und Assets

Diese Seite zeigt, wie Modelle geladen und Kugeln erzeugt werden, wie Objekte und ihre Teile transformiert werden und wie das während des Renderings vom Terminal aus geht.

## Beispiel: ein Modell und eine Kugel in `SetupScene()`

Aus der Beispielszene (`src/sandbox.cpp`, Branch `main`):

```cpp
	// Load a model. The call waits until the model is loaded.
	SceneObject *room = sandbox.LoadModel("Room", "../assets/viking_room/viking_room.gltf");
	room->SetRotation({-90.0f, 0.0f, 0.0f});

	// Create a simple mesh that needs no file
	SceneObject *sphere = sandbox.CreateSphere("Sphere", 0.2f);
	sphere->SetPosition({0.5f, 0.3f, -1.2f});
```

Alle Szenenobjekte werden in `SetupScene()` erzeugt – nach der Kette, vor dem Rendering, in beliebiger Reihenfolge. Während des Renderings wird nichts erzeugt oder gelöscht. Die zurückgegebenen Objekte gehören der Engine und bleiben gültig, bis die `Sandbox` zerstört wird.

## `LoadModel`

```cpp
SceneObject *LoadModel(const std::string &name, const std::string &file);
```

- `file`: eine glTF-Datei (`.gltf` mit `.bin` daneben, oder `.glb`); ihre Texturen müssen **KTX2** sein ([[Eigene Modelle|Eigene-Modelle]]). Der Pfad ist relativ zum Arbeitsverzeichnis `src/`, daher `"../assets/..."`.
- Der Aufruf wartet, bis das Modell geladen ist; Meshes und Texturen werden während der ersten Frames hinter dem Lade-Overlay hochgeladen. Braucht die vollständige Kette.
- Misslingt das Laden: `LoadModel: "<file>" could not be loaded; the object "<name>" is empty` (nach `GLTF Error: ...` oder `Failed to parse GLTF file: ...`). Das leere Objekt ignoriert jeden Aufruf; der Rest der Szene wird gezeigt.
- Ein zweites `LoadModel()` hängt die Lichter seiner glTF-Datei an die des ersten an.

Das einzige Modell im Repository, der Viking Room, hat ein Material (`Texture1`), einen Mesh-Knoten (in der Datei um +90° um X gedreht, daher `SetRotation({-90.0f, 0.0f, 0.0f})` im Beispiel) und keine glTF-Lichter.

## Teile: `Part`

Ein geladenes Modell besteht aus **einem Teil je Material**, alle mit dem Transform des Objekts.

```cpp
SceneObject *Part(const std::string &materialName);
```

spricht einen Teil an; er heißt `<Objekt>.<Material>` (beim Viking Room `Room.Texture1`). Ein unbekanntes Material druckt `Part: the object "<name>" has no material "<material>". Its materials: "..."` und gibt ein leeres Objekt zurück. Die Materialnamen sind die der glTF-Datei.

## `CreateSphere`

```cpp
SceneObject *CreateSphere(const std::string &name, float radius);
```

Ein Mesh ohne Datei; die Engine stellt den GPU-Upload selbst in die Warteschlange. Die Dreiecke sind gegen den Uhrzeigersinn aufgezählt, sodass die Außenseite die Vorderseite ist. Eine Kugel bringt keine Textur mit; in der Beispielszene ist sie die kleine weiße Kugel vor dem Raum.

## Transformationen eines `SceneObject`

An alle Teile weitergereicht; Winkel in Grad um die x-, y- und z-Achse.

| absolut | relativ |
|---|---|
| `SetPosition(const glm::vec3 &position)` | `Move(const glm::vec3 &translation)` |
| `SetRotation(const glm::vec3 &degrees)` | `Rotate(const glm::vec3 &degrees)` |
| `SetScale(const glm::vec3 &scale)` | `Scale(const glm::vec3 &factors)` |

`GetName()` gibt den Namen zurück, der beim Erzeugen vergeben wurde. Die Platzierung aus dem Modellierwerkzeug (die Knoten-Transformation der glTF-Datei) bleibt erhalten; `SetPosition` / `SetRotation` / `SetScale` wirken darauf obendrauf.

## Terminalbefehle während des Renderings

In das Terminal, aus dem die Engine gestartet wurde, getippt und mit Enter bestätigt; wirksam im nächsten Frame:

```
Room.Move(1, 0, 0)
Sun.Rotate(0, 30, 0)
Sphere.Scale(2, 2, 2)
```

- `Name` ist der Name, der `LoadModel()`, `CreateSphere()` oder `CreateLight()` gegeben wurde; ein Befehl auf ein Objekt erreicht alle seine Teile. Leerzeichen dürfen überall stehen, ein `;` darf die Zeile beenden. `Rotate` in Grad.
- Erfolg druckt nichts. Alles andere antwortet mit einer Zeile auf stderr: `Terminal: <problem> Commands: Name.Move(x, y, z), Name.Rotate(x, y, z) in degrees, Name.Scale(x, y, z). Names: "...", ...` – Probleme: `"<line>" is not a command.`, `there is no object or light "<name>".`, `a light has no size, "<name>" cannot be scaled.`
- Kameras sind nicht ansprechbar. Zeilen, die während des Ladens getippt werden, wirken, wenn das Laden endet. Die Logausgabe der Engine kann sich mit der getippten Zeile mischen; die Zeile wird trotzdem als Ganzes gelesen.

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *The sandbox layer*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#the-sandbox-layer-planned-change-5-own-code) und [*Terminal commands*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#terminal-commands-planned-change-6-own-code), [`docs/ROADMAP.md`, *Scene*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ROADMAP.md#scene).
