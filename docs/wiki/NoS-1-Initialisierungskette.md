# NoS 1 Initialisierungskette

Diese Seite zeigt die acht Aufrufe, mit denen `main()` die Engine aufsetzt, ihre Reihenfolge und die Begründung, warum jeder Aufruf den vor ihm braucht.

## Beispiel: die Kette, wie sie in `main()` steht

Aus der Beispielszene (`src/sandbox.cpp`, Branch `main`):

```cpp
	Sandbox sandbox;

	// The initialization chain: eight calls in this order, each one a group of Vulkan steps.
	// A call that is missing or out of order is reported, and the engine does not render.
	sandbox.InitializeWindow("Sandbox", WINDOW_WIDTH, WINDOW_HEIGHT);
	sandbox.CreateInstance();
	sandbox.PickDevice();
	sandbox.CreateSwapChain();
	sandbox.InitializeRendering();
	sandbox.CreatePipelines();
	sandbox.CreateCommandBuffers();
	sandbox.CreateSyncObjects();

	// Set up the scene
	SetupScene(sandbox);
```

`WINDOW_WIDTH` und `WINDOW_HEIGHT` sind die Konstanten 800 und 600 am Anfang der Datei. Nach der Kette folgt `SetupScene(sandbox)`, dann die Render-Schleife ([[NoS 2|NoS-2-Frame-Ablauf]]).

## Die acht Aufrufe

Jeder Aufruf gibt `bool` zurück: *True if the call was made in order and succeeded.* Die Beschreibung ist die, die das Terminal in der [[Pipeline-Ansicht|Pipeline-Ansicht]] hinter `[ok]` druckt.

| # | Aufruf | Was der Aufruf anlegt |
|---|---|---|
| 1 | `InitializeWindow(const std::string &title, int width, int height)` | `the window and its input callbacks` |
| 2 | `CreateInstance()` | `Vulkan instance, debug messenger, window surface` |
| 3 | `PickDevice()` | `GPU, logical device with its queues, memory pool` |
| 4 | `CreateSwapChain()` | `swap chain and its image views` |
| 5 | `InitializeRendering()` | `dynamic rendering, depth image, off-screen color image` |
| 6 | `CreatePipelines()` | `the engine's pipelines, descriptor set layouts, light buffers` |
| 7 | `CreateCommandBuffers()` | `command pool, descriptor pool, default textures, command buffers` |
| 8 | `CreateSyncObjects()` | `semaphores and fences, worker threads, model loader, UI` |

Der erste Aufruf heißt `InitializeWindow`, nicht `CreateWindow` – das ist ein Makro aus `windows.h`. `CreateInstance()` und `PickDevice()` schalten in Debug-Builds die Validation Layers ein. Die Begriffe der rechten Spalte erklärt die Seite [[Begriffe]].

## Warum jeder Aufruf den vor ihm braucht

Das ist der Text, den die Engine im Fehlerfall hinter `too early:` druckt – was der Aufruf in Vulkan-Begriffen braucht, ohne den fehlenden Aufruf zu nennen.

| Aufruf | braucht … |
|---|---|
| 2 `CreateInstance` | *the instance asks the window system which extensions it needs, and the surface it creates is the drawing area of a window. No window exists yet.* |
| 3 `PickDevice` | *a GPU is picked from the devices the Vulkan instance lists, and it has to be able to present to the window surface. Neither exists yet.* |
| 4 `CreateSwapChain` | *a swap chain is created by a logical device, with an image format the GPU supports. No device exists yet.* |
| 5 `InitializeRendering` | *the depth image and the off-screen color image get the size of the swap chain images. No swap chain exists yet.* |
| 6 `CreatePipelines` | *a pipeline is built for the formats of the attachments it draws into. The rendering set-up with its attachments does not exist yet.* |
| 7 `CreateCommandBuffers` | *the descriptor sets that are allocated here are built after the descriptor set layouts of the pipelines. No pipeline exists yet.* |
| 8 `CreateSyncObjects` | *the semaphores and fences synchronize the command buffers of the frames, and the worker threads upload textures through the command pool. Neither exists yet.* |

## Die Prüfung

Ein Aufruf, der zweimal, zu früh oder ohne die Aufrufe vor ihm gemacht wird, meldet den Fehler und **stoppt die Initialisierung**: er und jeder spätere Aufruf tun nichts und drucken nichts (`LoadModel()` und `CreateSphere()` geben ein leeres Objekt zurück), `IsRunning()` gibt `false` zurück, ohne zu rendern, und das Programm endet normal. Die Fehlerzeilen (stderr), jeweils gefolgt von der Kettenansicht:

```
Initialization error: <Call>() cannot run yet. Initialization stopped.
Initialization error: <Call>() was called twice. Initialization stopped.
<Call>() failed. Initialization stopped.
```

Wie die Kettenansicht im Fehlerfall aussieht und wie sie gelesen wird: [[Pipeline-Ansicht|Pipeline-Ansicht]].

## Aufrufe außerhalb der Kette, die von ihr abhängen

- `LoadModel`, `CreateSphere`, `AddToPipeline` und `IsRunning` brauchen alle acht Aufrufe. Begründung der Szenenaufrufe: *the meshes and textures of the scene go into buffers and images on the GPU, which only the completely initialized engine can create. The initialization chain is not complete.*
- `CreatePipeline` braucht `CreatePipelines()` – es kopiert das dort erzeugte Layout ([[NoS 3|NoS-3-Pipeline-Konfiguration]]).
- `CreateCamera`, `CreateLight` und `SetActiveCamera` werden nicht geprüft; sie legen nur Entities an.

## Der Startcheck des ersten `IsRunning()`

Vor dem ersten Frame prüft `IsRunning()` zweierlei. Ist die Kette unvollständig, wird das wie ein Kettenaufruf gemeldet (*the initialization chain is not complete; the engine cannot render yet.*). Fehlt die aktive Kamera:

```
IsRunning(): there is no active camera. Create one with CreateCamera() and pass it to SetActiveCamera() in SetupScene().
```

In beiden Fällen startet die Schleife nie. Die Kamera: [[NoS 6|NoS-6-Kamera-und-Licht]].

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *The initialization chain*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#the-initialization-chain-planned-change-7-own-code).
