# 1 Initialisierungskette und Frame-Ablauf

Diese Seite ist das Nachschlagewerk zu Aufgabe 1: die acht Aufrufe der Initialisierungskette und die sechs Aufrufe der Render-Schleife, ihre Reihenfolge, was jeder Aufruf vom Aufruf vor ihm braucht und was die Engine-Konsole dazu ausgibt.

## Beispiel: `main()` der Beispielszene

Aus der Beispielszene (`src/sandbox.cpp`, Branch `main`):

```cpp
	Sandbox sandbox;

	// The initialization chain: eight calls in this order, each one a group of Vulkan steps.
	// A call that is missing or out of order is reported, and the engine does not render.

	// 1. The window, the Vulkan instance, the device and the swap chain (calls 1 to 4)
	sandbox.InitializeWindow("Sandbox", WINDOW_WIDTH, WINDOW_HEIGHT);
	sandbox.CreateInstance();
	sandbox.PickDevice();
	sandbox.CreateSwapChain();

	// 2. The rendering set-up and the engine's own pipelines (calls 5 and 6)
	sandbox.InitializeRendering();
	sandbox.CreatePipelines();

	// 3. The command buffers and the synchronization (calls 7 and 8)
	sandbox.CreateCommandBuffers();
	sandbox.CreateSyncObjects();

	// Set up the scene
	SetupScene(sandbox);

	// The render loop: six calls per frame in this order, until the window is closed. A call that is
	// missing or out of order is reported by the next one, and rendering stops. The engine's loading
	// overlay covers the first frames while the meshes and textures are uploaded.
	while (sandbox.IsRunning())
	{
		sandbox.BeginFrame();        // wait until the GPU is done with this frame slot, acquire a swap chain image
		sandbox.UpdateScene();       // camera controls, terminal commands, lights and transforms into the uniform buffers
		sandbox.BeginRendering();    // begin the command buffer, clear the color and depth attachments
		sandbox.DrawScene();         // per object: bind its pipeline, descriptor sets and buffers, then draw
		sandbox.EndRendering();      // the engine's UI on top, end the command buffer
		sandbox.EndFrame();          // submit the command buffer, present the image
	}

	return 0;
```

`WINDOW_WIDTH` und `WINDOW_HEIGHT` sind die Konstanten 800 und 600 am Anfang der Datei. Zwischen Kette und Schleife steht `SetupScene(sandbox)` mit der Szene ([[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]]).

## Die Initialisierungskette

### Die acht Aufrufe

Die Kette steht in `main()` vor `SetupScene()`, in drei Gruppen, jede unter einem Kommentar, der mit ihrer Nummer beginnt. Jeder Aufruf gibt `bool` zurück: *True if the call was made in order and succeeded, false otherwise.* Die rechte Spalte ist der Text, den die Engine-Konsole unter `Initialization chain:` hinter `[ok]` druckt.

| # | Gruppe | Aufruf | Was der Aufruf anlegt |
|---|---|---|---|
| 1 | 1 | `InitializeWindow(const std::string &title, int width, int height)` | `the window and its input callbacks` |
| 2 | 1 | `CreateInstance()` | `Vulkan instance, debug messenger, window surface` |
| 3 | 1 | `PickDevice()` | `GPU, logical device with its queues, memory pool` |
| 4 | 1 | `CreateSwapChain()` | `swap chain and its image views` |
| 5 | 2 | `InitializeRendering()` | `dynamic rendering, depth image, off-screen color image` |
| 6 | 2 | `CreatePipelines()` | `the engine's pipelines, descriptor set layouts, light buffers` |
| 7 | 3 | `CreateCommandBuffers()` | `command pool, descriptor pool, default textures, command buffers` |
| 8 | 3 | `CreateSyncObjects()` | `semaphores and fences, worker threads, model loader, UI` |

`InitializeWindow` legt das Fenster mit Titel, Breite und Höhe an; die übrigen sieben Aufrufe haben keine Parameter. Die Begriffe der rechten Spalte erklärt die Seite [[Begriffe]].

### Warum jeder Aufruf den vor ihm braucht

Jeder Aufruf braucht, was der Aufruf direkt vor ihm anlegt. Deshalb ist die Reihenfolge fest, und die Engine prüft sie bei jedem Aufruf. Die Tabelle nennt, was ein Aufruf braucht, im Wortlaut der Engine. Denselben Text druckt die Engine-Konsole in der `[!!]`-Zeile hinter `too early:`, wenn der Aufruf zu früh steht. Der Text nennt nie den Aufruf, der fehlt; welcher Aufruf das Fehlende anlegt, steht in der Tabelle oben.

| Aufruf | braucht … |
|---|---|
| 1 `InitializeWindow` | nichts; er ist der erste Aufruf |
| 2 `CreateInstance` | *the instance asks the window system which extensions it needs, and the surface it creates is the drawing area of a window. No window exists yet.* |
| 3 `PickDevice` | *a GPU is picked from the devices the Vulkan instance lists, and it has to be able to present to the window surface. Neither exists yet.* |
| 4 `CreateSwapChain` | *a swap chain is created by a logical device, with an image format the GPU supports. No device exists yet.* |
| 5 `InitializeRendering` | *the depth image and the off-screen color image get the size of the swap chain images. No swap chain exists yet.* |
| 6 `CreatePipelines` | *a pipeline is built for the formats of the attachments it draws into. The rendering set-up with its attachments does not exist yet.* |
| 7 `CreateCommandBuffers` | *the descriptor sets that are allocated here are built after the descriptor set layouts of the pipelines. No pipeline exists yet.* |
| 8 `CreateSyncObjects` | *the semaphores and fences synchronize the command buffers of the frames, and the worker threads upload textures through the command pool. Neither exists yet.* |

### Was außerhalb der Kette von ihr abhängt

- `LoadModel`, `CreateSphere` und `AddToPipeline` brauchen alle acht Aufrufe; sie stehen deshalb in `SetupScene()`. `CreatePipeline` braucht `CreatePipelines()`. `CreateCamera`, `CreateLight` und `SetActiveCamera` werden nicht geprüft.
- Das erste `IsRunning()` braucht ebenfalls alle acht Aufrufe und dazu eine aktive Kamera. Fehlt sie, druckt es `IsRunning(): there is no active camera. Create one with CreateCamera() and pass it to SetActiveCamera() in SetupScene().`, und die Schleife startet nicht. Im Gerüst auf `learning` setzt die gegebene Zeile in `SetupScene()` eine Standardkamera aktiv.

## Die Render-Schleife

### `IsRunning()`

`bool IsRunning()` ist die Bedingung der Schleife `while (sandbox.IsRunning())`: *true until the window is closed*, und `false`, sobald das Rendering nach einem Fehler gestoppt wurde. Der Aufruf verarbeitet die Fensterereignisse und die Frame-Zeit des kommenden Frames. Der erste Aufruf prüft die Kette und die aktive Kamera und druckt die Kette und jede Pipeline in die Engine-Konsole. Wird das Fenster geschlossen, gibt `IsRunning()` `false` zurück, die Schleife endet, und `main()` endet mit `return 0;`.

### Die sechs Aufrufe

Ein Frame besteht aus sechs Aufrufen in fester Reihenfolge: Er beginnt mit `BeginFrame()` und endet mit `EndFrame()`, dazwischen stehen die vier anderen. Alle sechs sind `void` und haben keine Parameter. Die rechte Spalte ist der Text, den die Engine-Konsole unter `Frame sequence:` hinter `[ok]` druckt.

| # | Aufruf | Was der Aufruf tut |
|---|---|---|
| 1 | `BeginFrame()` | `wait for the GPU, acquire a swap chain image` |
| 2 | `UpdateScene()` | `camera controls, terminal commands, scene data -> uniform buffers` |
| 3 | `BeginRendering()` | `begin the command buffer, clear the color and depth attachments` |
| 4 | `DrawScene()` | `per object: bind pipeline, descriptor sets, buffers, draw` |
| 5 | `EndRendering()` | `the engine's UI on top, end the command buffer` |
| 6 | `EndFrame()` | `submit the command buffer, present the image` |

Ein `DrawScene()` zeichnet alle Objekte der Szene, jedes mit seiner Pipeline; einen Zeichenaufruf je Objekt gibt es nicht.

### Warum jeder Aufruf den vor ihm braucht

Wie in der Kette braucht jeder Frame-Aufruf, was der Aufruf vor ihm in diesem Frame getan hat. Den Text der Tabelle druckt die Engine-Konsole in der `[!!]`-Zeile hinter `too early:`.

| Aufruf | braucht … |
|---|---|
| 1 `BeginFrame` | nichts; er beginnt den Frame |
| 2 `UpdateScene` | *the scene data is written into the uniform buffers of a frame. No frame has been begun: the GPU may still be reading them.* |
| 3 `BeginRendering` | *the draw commands that are recorded from here on use the scene data of this frame. The uniform buffers have not been updated in this frame.* |
| 4 `DrawScene` | *draw commands are recorded into a command buffer, inside a rendering pass. Neither has been begun in this frame.* |
| 5 `EndRendering` | *the UI is drawn on top of the finished scene, and the scene has not been drawn in this frame.* |
| 6 `EndFrame` | *only a command buffer whose recording has ended can be submitted to the GPU. This frame has none.* |
| `IsRunning` vor dem nächsten Frame | *the frame that was begun is not finished. Its command buffer has not been submitted and its image has not been presented.* |

## Was die Engine-Konsole zeigt

### Beim Start: Kette und Pipelines

Einmal gedruckt vom ersten `IsRunning()`, nach dem Start-Log der Engine und vor dem ersten Frame. So sieht die Ausgabe beim ergänzten Gerüst auf `learning` aus:

```
Initialization chain:

  [ok] InitializeWindow      the window and its input callbacks
  [ok] CreateInstance        Vulkan instance, debug messenger, window surface
  [ok] PickDevice            GPU, logical device with its queues, memory pool
  [ok] CreateSwapChain       swap chain and its image views
  [ok] InitializeRendering   dynamic rendering, depth image, off-screen color image
  [ok] CreatePipelines       the engine's pipelines, descriptor set layouts, light buffers
  [ok] CreateCommandBuffers  command pool, descriptor pool, default textures, command buffers
  [ok] CreateSyncObjects     semaphores and fences, worker threads, model loader, UI

Pipeline "pbr"    shaders/pbr.slang -> shaders/pbr.spv

  Input assembly     triangle list, engine vertex layout        fixed
        |
  Vertex shader      VSMain                                     fixed
        |
  Rasterization      cull mode: back                            fixed
        |
  Fragment shader    PSMain                                     fixed
        |
  Depth test         on, writes depth                           fixed
        |
  Color blending     off (on for blended materials)             fixed
        |
  Attachments        off-screen color image + depth image       fixed

  Objects            none

```

Bei der Beispielszene auf `main` ist die Ausgabe dieselbe bis auf die letzte Zeile, `Objects            Room, Sphere`: Das Gerüst hat noch kein Objekt, deshalb zeichnet `"pbr"` nichts.

### Nach dem ersten Frame: der Frame-Ablauf

Einmal gedruckt, sobald der erste Frame vollständig gelaufen ist, also alle sechs Aufrufe in der Reihenfolge der Tabelle oben:

```
Frame sequence:

  [ok] BeginFrame      wait for the GPU, acquire a swap chain image
  [ok] UpdateScene     camera controls, terminal commands, scene data -> uniform buffers
  [ok] BeginRendering  begin the command buffer, clear the color and depth attachments
  [ok] DrawScene       per object: bind pipeline, descriptor sets, buffers, draw
  [ok] EndRendering    the engine's UI on top, end the command buffer
  [ok] EndFrame        submit the command buffer, present the image

```

### Das Fenster

Beim ergänzten Gerüst ist das Fenster schwarz: Die Engine löscht das Bild in jedem Frame auf Schwarz, und die Szene hat noch kein Objekt. Darüber liegt nur das Panel *Renderer* der Engine. Bei der Beispielszene zeigt das Fenster den beleuchteten Raum und die Kugel.

### Im Fehlerfall

Ein Aufruf, der zu früh oder zweimal kommt, stoppt die Initialisierung oder das Rendering. Die Engine-Konsole druckt dann eine Fehlerzeile, darunter die Aufrufe, die schon gelaufen sind, als `[ok]`-Zeilen, und den Aufruf, der aufgerufen wurde und nicht laufen konnte, als `[!!]`-Zeile mit seinem Grund aus den Tabellen oben, und nichts darunter. Danach tut jeder weitere Aufruf nichts, und das Programm endet normal; nach einem Fehler im Frame-Ablauf schließt sich das Fenster. Je Lauf gibt es so genau eine Fehleransicht. Wie die Fehlerzeilen lauten: [[Fehler und Debugging|Fehler-und-Debugging]].

Gelesen wird die Fehleransicht in drei Schritten: (1) Welcher Aufruf steht in der `[!!]`-Zeile? (2) Was braucht er laut seinem Grund? (3) Welcher Aufruf legt das laut den Tabellen oben an, und steht dieser in der eigenen Datei vor dem markierten Aufruf?

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *The initialization chain*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#the-initialization-chain-planned-change-7-own-code), [*The render loop*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#the-render-loop-planned-change-8-own-code) und [*Terminal output*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#terminal-output-planned-change-9-own-code).
