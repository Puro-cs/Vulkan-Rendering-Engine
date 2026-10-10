# 4 Materialien und Referenzgrafiken

Diese Seite ist das Nachschlagewerk zu Aufgabe 4: die Szene, die die Referenzgrafiken zeigen, die Bedingungen, unter denen ein eigenes Bild mit ihnen vergleichbar ist, wo ein Material im eigenen Shader steht und was im Bild außer dem Material noch wirkt.

## Die Szene der Referenzgrafiken

Die Referenzgrafiken zeigen die Beispielszene mit den zwei Änderungen aus Aufgabe 3, Schritt 1: Die Kugel ist der Pipeline `"phong"` zugewiesen, und die Sonne hat die Intensität 1. Das `SetupScene()` dieser Szene, die beiden geänderten Zeilen markiert:

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
	sun->SetIntensity(1.0f);                        // geändert: 1 statt 3 (Aufgabe 3, Schritt 1)

	// Load a model. The call waits until the model is loaded.
	SceneObject *room = sandbox.LoadModel("Room", "../assets/viking_room/viking_room.gltf");
	room->SetRotation({-90.0f, 0.0f, 0.0f});

	// Create a simple mesh that needs no file
	SceneObject *sphere = sandbox.CreateSphere("Sphere", 0.2f);
	sphere->SetPosition({0.5f, 0.3f, -1.2f});
	sandbox.AddToPipeline("phong", sphere);         // neu (Aufgabe 3, Schritt 1)
}
```

Die Pipeline `"phong"` legt `main()` nach `CreatePipelines()` an, wie in Aufgabe 2 ([[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]]). Der Raum bleibt bei `"pbr"`.

## Die Bedingungen der Referenzgrafiken

Ein eigenes Bild ist nur mit einer Referenzgrafik vergleichbar, wenn es unter denselben Bedingungen entstanden ist:

| Bedingung | Wert |
|---|---|
| Szene | die Szene oben: die Kugel in `"phong"`, der Raum in `"pbr"` |
| Licht | `"Sun"`, gerichtet, Rotation (−45°, 45°, 0°), Intensität 1, Farbe Weiß |
| Kamera | unbewegt, also direkt nach dem Start: Position (2; 0,5; −2) und Rotation (0°, 135°, 0°) aus `SetupScene()` |
| Belichtung | 1,2, der Standardwert des Schiebereglers *Exposure* im Panel *Renderer* der Engine |
| Fenster | 800 × 600, die Konstanten `WINDOW_WIDTH` und `WINDOW_HEIGHT` unverändert |
| Bildausschnitt | ein Ausschnitt um die Kugel aus dem Fenster |

## Das Material im Shader

Ein Material besteht aus den drei Konstanten des eigenen Shaders aus Aufgabe 3 für k<sub>diff</sub>, k<sub>spec</sub> und n: zwei `float3` mit Rot, Grün und Blau und ein `float`. Ein anderes Material heißt: die Werte der Konstanten ändern, bauen, starten. Die Engine liest den Shader beim Anlegen der Pipeline, also beim Start; einen Regler für eigene Werte gibt es nicht. Wie Konstanten geschrieben werden: [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]].

## Was im Bild außer dem Material wirkt

Weicht ein eigenes Bild von einer Referenzgrafik ab, kann die Ursache auch außerhalb der drei Konstanten liegen:

| Was | Wirkung im Bild | So ist es wie bei den Referenzgrafiken |
|---|---|---|
| Belichtung | Der *composite*-Pass multipliziert das ganze Bild mit dem Wert des Schiebereglers *Exposure* und bildet es danach mit einem filmischen Tone Mapping ab; ein anderer Wert macht das ganze Bild heller oder dunkler. Der Regler zeigt 0,1 bis 4, die Engine rechnet mit mindestens 0,2 | Regler nicht bewegen; nach einem Neustart steht er wieder auf 1,2 |
| Abschneiden über 1 | Die Engine schreibt die Rückgabe von `PSMain` in ein Bild mit 8 Bit je Kanal und schneidet dabei jeden Wert über 1 auf 1 ab, vor der Belichtung. Wo die Summe der Terme in einem Kanal 1 übersteigt, wird die Fläche gleichmäßig hell, und ein Glanzlicht darin ist nicht zu sehen | Sonne mit Intensität 1 |
| Licht | Richtung, Farbe und Intensität der Sonne bestimmen, welche Seite der Kugel hell ist und wie hell | Rotation, Farbe und Intensität wie in der Tabelle oben |
| Kamera | Die Kamerasteuerung (`W`, `A`, `S`, `D`, `Q`, `E`, Maus mit gedrückter linker Taste) bewegt die Kamera, auch durch einen versehentlichen Tastendruck; das Glanzlicht wandert dann | Neustart, danach Tastatur und Maus im Fenster nicht benutzen |
| Fenster | Eine andere Fenstergröße ändert das Seitenverhältnis und die Größe der Kugel im Bild | Konstanten unverändert lassen |

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *Shaders*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#shaders-srcshaders) (die Zeile `composite.slang`) und [*One frame*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#one-frame-rendererrender-renderer_renderingcpp).
