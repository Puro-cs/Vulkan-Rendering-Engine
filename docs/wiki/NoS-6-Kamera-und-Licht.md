# NoS 6 Kamera und Licht

Diese Seite zeigt, wie Kamera und Lichter angelegt und eingestellt werden, welche Kamera rendert und was davon im Shader ankommt.

## Beispiel: Kamera und Licht in `SetupScene()`

Aus der Beispielszene (`src/sandbox.cpp`, Branch `main`):

```cpp
	// Create a camera and make it the active one
	Camera *camera = sandbox.CreateCamera("Camera");
	camera->SetPosition({2.0f, 0.5f, -2.0f});
	camera->SetRotation({0.0f, 135.0f, 0.0f});
	sandbox.SetActiveCamera(camera);

	// Create a directional light. A light shines along the -Z axis; the rotation turns it down and sideways.
	Light *sun = sandbox.CreateLight("Sun", LightType::Directional);
	sun->SetRotation({-45.0f, 45.0f, 0.0f});
	sun->SetIntensity(3.0f);
```

## Kamera

```cpp
Camera *CreateCamera(const std::string &name);
void   SetActiveCamera(Camera *camera);
```

| Aufruf auf `Camera` | Bedeutung |
|---|---|
| `SetPosition(const glm::vec3 &position)` | Position |
| `SetRotation(const glm::vec3 &degrees)` | Drehung um x, y, z in Grad; *Without a rotation the camera looks along the -Z axis* |
| `SetFieldOfView(float degrees)` | Öffnungswinkel, Standard 45° |

Near-Plane 0,1 und Far-Plane 100 sind fest. `SetActiveCamera()` legt fest, mit welcher Kamera die Szene gerendert wird, und gibt ihr das Seitenverhältnis des Fensters. **Ohne aktive Kamera startet die Schleife nicht**: das erste `IsRunning()` druckt `IsRunning(): there is no active camera. Create one with CreateCamera() and pass it to SetActiveCamera() in SetupScene().` ([[NoS 1|NoS-1-Initialisierungskette]]). Die Kamerasteuerung der Engine (Tasten `W A S D`, `Q`/`E`, Maus mit gedrückter linker Taste; [[Architektur]], KI 7) wirkt auf die aktive Kamera: die in `SetupScene()` gesetzte Rotation ist die Startausrichtung. Kameras sind über Terminalbefehle nicht ansprechbar. Ein geladenes Modell, das eine Kamera enthält, überschreibt die Entity namens `Camera` ([[Eigene Modelle|Eigene-Modelle]]).

## Licht

```cpp
enum class LightType { Directional, Point, Spot };
Light *CreateLight(const std::string &name, LightType type);
```

| Aufruf auf `Light` | gilt für | Bedeutung |
|---|---|---|
| `SetPosition(const glm::vec3 &position)` | Punktlicht, Spotlight | Position |
| `SetRotation(const glm::vec3 &degrees)` | alle | Drehung in Grad; *Without a rotation the light shines along the -Z axis* |
| `SetDirection(const glm::vec3 &direction)` | gerichtetes Licht, Spotlight | die Richtung, in die das Licht scheint; ersetzt die Rotation; muss nicht normiert sein; ein Nullvektor wird ignoriert |
| `SetColor(const glm::vec3 &color)` | alle | Farbe, Standard Weiß |
| `SetIntensity(float intensity)` | alle | *The color is multiplied by it*; Standard 1 |
| `SetRange(float range)` | Punktlicht, Spotlight | Reichweite, Standard 100 |
| `SetConeAngles(float innerDegrees, float outerDegrees)` | Spotlight | innerer und äußerer Kegelwinkel, Standard 0° und 45° |

Die Lichttypen und die Vorlesung: Punktlicht ↔ `Point`; unendlich weit entfernte Lichtquelle mit parallelen Strahlen ↔ `Directional`; gerichtete Lichtquelle (Spotlight) ↔ `Spot`. Ambientes Licht und ausgedehnte Lichtquellen gibt es in der Engine nicht – ein ambienter Anteil ist eine Konstante im Shader ([[NoS 5|NoS-5-Shader-Schnittstelle]]).

Mit dem Shader der Engine (`"pbr"`) zeigt eine Szene ohne Licht nur den ambienten Anteil und emissive Flächen; der Viking Room wird vom Licht `"Sun"` beleuchtet. Lichter lassen sich im Terminal bewegen und drehen (`Sun.Rotate(0, 30, 0)`), nicht skalieren ([[NoS 4|NoS-4-Geometrie-und-Assets]]).

## Was im Shader ankommt

- Jedes Licht der Szene – die mit `CreateLight()` angelegten, die `KHR_lights_punctual`-Lichter geladener Modelle und die Lichter emissiver Materialien, bis zu 1024 – wird jeden Frame neu in den Storage Buffer an Binding 6 geschrieben: `lightBuffer[i]` mit `lightType`, `position`, `color`, `direction`, `range`, `innerConeAngle`, `outerConeAngle`; die Anzahl steht in `ubo.lightCount`. Bei einem gerichteten Licht ist `position.xyz` die Richtung, in die das Licht läuft; bei Punkt- und Spotlights die Position. Die Felder im Einzelnen: [[NoS 5|NoS-5-Shader-Schnittstelle]].
- Die aktive Kamera füllt jeden Frame `ubo.view`, `ubo.proj` und `ubo.camPos`.

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *The sandbox layer*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#the-sandbox-layer-planned-change-5-own-code) und [*What a mesh shader gets from the engine*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#what-a-mesh-shader-gets-from-the-engine).
