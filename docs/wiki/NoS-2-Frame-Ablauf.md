# NoS 2 Frame-Ablauf

Diese Seite zeigt die Render-Schleife: `IsRunning()` als Bedingung und die sechs Aufrufe, die jeden Frame in fester Reihenfolge bilden.

## Beispiel: die Schleife, wie sie in `main()` steht

Aus der Beispielszene (`src/sandbox.cpp`, Branch `main`):

```cpp
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

Die sechs Aufrufe sind **eine Einheit**: ein Frame beginnt mit `BeginFrame()` und endet mit `EndFrame()`, dazwischen stehen die vier anderen in dieser Reihenfolge.

## `IsRunning()`

`bool IsRunning()` ist `true`, bis das Fenster geschlossen wird oder das Rendering gestoppt wurde; es verarbeitet die Fensterereignisse und die Frame-Zeit des kommenden Frames. Der erste Aufruf prüft die Kette und die aktive Kamera ([[NoS 1|NoS-1-Initialisierungskette]]) und druckt die Kette und jede Pipeline ins Terminal ([[Pipeline-Ansicht|Pipeline-Ansicht]]). Die Schleife läuft auch, während ein Modell noch lädt: die Engine rendert dann vollständige Frames, die ihr Lade-Overlay statt der Szene zeigen.

## Die sechs Aufrufe

Alle `void`. Die Beschreibung ist die, die das Terminal hinter `[ok]` druckt.

| # | Aufruf | Was der Aufruf tut |
|---|---|---|
| 1 | `BeginFrame()` | `wait for the GPU, acquire a swap chain image` |
| 2 | `UpdateScene()` | `camera controls, terminal commands, scene data -> uniform buffers` |
| 3 | `BeginRendering()` | `begin the command buffer, clear the color and depth attachments` |
| 4 | `DrawScene()` | `per object: bind pipeline, descriptor sets, buffers, draw` |
| 5 | `EndRendering()` | `the engine's UI on top, end the command buffer` |
| 6 | `EndFrame()` | `submit the command buffer, present the image` |

Ein `DrawScene()` zeichnet **alle** Objekte; es gibt keinen Zeichenaufruf je Objekt. Innerhalb von `DrawScene()` liegt auch das Tone Mapping (der *composite*-Pass zwischen den undurchsichtigen und den transparenten Objekten). Der Command Buffer beginnt in `BeginRendering()` und nicht schon in `BeginFrame()`, weil die Uniform-Buffer- und Descriptor-Schreibzugriffe von `UpdateScene()` vor der Aufzeichnung liegen müssen. Ändert sich die Fenstergröße, erstellt `BeginFrame()` die Swap Chain neu, und der Rest dieses Frames tut nichts.

## Warum jeder Aufruf den vor ihm braucht

Der Text, den die Engine im Fehlerfall hinter `too early:` druckt:

| Aufruf | braucht … |
|---|---|
| 2 `UpdateScene` | *the scene data is written into the uniform buffers of a frame. No frame has been begun: the GPU may still be reading them.* |
| 3 `BeginRendering` | *the draw commands that are recorded from here on use the scene data of this frame. The uniform buffers have not been updated in this frame.* |
| 4 `DrawScene` | *draw commands are recorded into a command buffer, inside a rendering pass. Neither has been begun in this frame.* |
| 5 `EndRendering` | *the UI is drawn on top of the finished scene, and the scene has not been drawn in this frame.* |
| 6 `EndFrame` | *only a command buffer whose recording has ended can be submitted to the GPU. This frame has none.* |
| `IsRunning` | *the frame that was begun is not finished. Its command buffer has not been submitted and its image has not been presented.* |

## Die Prüfung

Ein Frame-Aufruf, der fehlt oder in der falschen Reihenfolge steht, wird vom nächsten Aufruf gemeldet; ein vergessenes `EndFrame()` meldet das nächste `IsRunning()`. Danach tut jeder Frame-Aufruf nichts, `IsRunning()` ist `false`, und das Programm endet normal, ohne Crash Dump. Die Zeilen (stderr), jeweils gefolgt von der Frame-Ansicht:

```
Frame sequence error: <Call>() cannot run yet. Rendering stopped.
Frame sequence error: <Call>() was called twice in this frame. Rendering stopped.
Exception: <text>
```

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *The render loop*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#the-render-loop-planned-change-8-own-code) und [*One frame*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#one-frame-rendererrender-renderer_renderingcpp).
