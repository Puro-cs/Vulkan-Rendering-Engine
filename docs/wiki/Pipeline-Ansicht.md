# Pipeline-Ansicht

Diese Seite zeigt, wie die Terminalausgabe der Engine gelesen wird: die Initialisierungskette, jede Pipeline mit ihren Objekten, der Frame-Ablauf – beim Start und im Fehlerfall.

## Beispiel: die Ausgabe der Beispielszene beim Start

Gedruckt **einmal** vom ersten `IsRunning()`, nach dem Start-Log und vor dem ersten Frame (stdout):

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

  Objects            Room, Sphere
```

Nach dem ersten vollständigen Frame folgt einmal, zwischen den Log-Zeilen der ersten Uploads, der Frame-Ablauf:

```
Frame sequence:

  [ok] BeginFrame      wait for the GPU, acquire a swap chain image
  [ok] UpdateScene     camera controls, terminal commands, scene data -> uniform buffers
  [ok] BeginRendering  begin the command buffer, clear the color and depth attachments
  [ok] DrawScene       per object: bind pipeline, descriptor sets, buffers, draw
  [ok] EndRendering    the engine's UI on top, end the command buffer
  [ok] EndFrame        submit the command buffer, present the image
```

Nie je Frame. Die Ausgabe ist Text mit festen Spalten, keine Zeichnung der GPU-Pipeline; sie benutzt die amerikanische Schreibweise des Codes (`color`, `Rasterization`).

## Die drei Teile

**Die Aufrufliste** (Kette und Frame-Ablauf): eine Zeile je Aufruf, `[ok]`, der Name, dann, was der Aufruf anlegt bzw. tut. Die Namensspalte ist so breit wie der längste gedruckte Name plus zwei. Die Zeilen sind die Aufrufe, **die die Sandbox-Datei gemacht hat** – die Ansicht nennt nie einen Aufruf, der nicht gemacht wurde.

**Ein Pipeline-Block** je Pipeline, `"pbr"` zuerst, dann die eigenen in der Reihenfolge ihrer Erzeugung ([[NoS 3|NoS-3-Pipeline-Konfiguration]]). Die Kopfzeile nennt den Namen, die `.slang`-Datei und die `.spv`, in die der Build sie übersetzt. Darunter sieben Stufen, durch `|` verbunden:

| Stufe | zeigt |
|---|---|
| `Input assembly` | Triangle List und das Vertex-Layout der Engine – immer `fixed` |
| `Vertex shader` | den Einstiegspunkt `VSMain` |
| `Rasterization` | `cull mode: none` / `front` / `back` |
| `Fragment shader` | den Einstiegspunkt `PSMain` |
| `Depth test` | `on, writes depth` / `on, no depth writes` / `off` |
| `Color blending` | `off` / `on, drawn in the transparent pass`; bei `"pbr"` `off (on for blended materials)` |
| `Attachments` | `off-screen color image + depth image`; mit Blending `swap chain image + depth image` – immer `fixed` |

`yours` steht an den vier Einstellungen einer eigenen Pipeline (Shader, Cull Mode, Depth Test, Blending), `fixed` an allem, was die Engine festlegt – und an jeder Zeile von `"pbr"`. Die letzte Zeile `Objects` nennt die Objekte, die die Pipeline zeichnet: Objekte, die keiner Pipeline zugewiesen sind, stehen unter `"pbr"`; ein einzeln hinzugefügter Teil heißt `Room.Texture1`; `none`, wenn die Pipeline nichts zeichnet.

## Der Fehlerfall

Bei einem Fehler in der Kette oder im Frame-Ablauf druckt die Engine auf **stderr** die Fehlerzeile, dann die Aufrufe, die gemacht sind, als `[ok]`-Zeilen, dann den Aufruf, der gemacht wurde und nicht laufen konnte, als `[!!]`-Zeile mit dem Grund – und **nichts darunter**. Ein Beispiel (`PickDevice()` fehlt):

```
Initialization error: CreateSwapChain() cannot run yet. Initialization stopped.

  [ok] InitializeWindow  the window and its input callbacks
  [ok] CreateInstance    Vulkan instance, debug messenger, window surface
  [!!] CreateSwapChain   <- too early: a swap chain is created by a logical device, with an image
                            format the GPU supports. No device exists yet.
```

Ein zweites (`BeginRendering()` fehlt):

```
Frame sequence error: DrawScene() cannot run yet. Rendering stopped.

  [ok] BeginFrame   wait for the GPU, acquire a swap chain image
  [ok] UpdateScene  camera controls, terminal commands, scene data -> uniform buffers
  [!!] DrawScene    <- too early: draw commands are recorded into a command buffer, inside a
                       rendering pass. Neither has been begun in this frame.
```

Die drei Gründe: `too early: …` (der Aufruf braucht etwas, das noch nicht existiert – der Text sagt in Vulkan-Begriffen, was), `called twice: this step is already done.` (ein Kettenaufruf, der schon gemacht ist, oder ein Frame-Aufruf, der in diesem Frame schon gemacht ist), `failed` (die Arbeit der Engine schlug fehl oder warf eine Ausnahme; der Text davor nennt sie). Weil der erste Fehler die Initialisierung bzw. das Rendering stoppt, gibt es **eine** Ansicht je Lauf.

## Was die Ansicht nicht sagt – und wo es steht

Die Ansicht nennt **nie den fehlenden Aufruf** und listet keine Aufrufe, die nicht gemacht wurden. Der Grund sagt, was der markierte Aufruf braucht; welcher Aufruf das anlegt und wo er in der Reihenfolge steht, steht auf [[NoS 1 Initialisierungskette|NoS-1-Initialisierungskette]] (die acht Aufrufe mit ihren Begründungen) und [[NoS 2 Frame-Ablauf|NoS-2-Frame-Ablauf]] (die sechs Aufrufe mit ihren Begründungen) – und in der Beispielszene auf `main`. So wird die Ansicht gelesen: (1) die `[!!]`-Zeile – welcher Aufruf ist es? (2) der Grund – was fehlt ihm? (3) auf NoS 1 / NoS 2 – welcher Aufruf legt das an, und steht er in der eigenen Datei vor dem markierten? Die Begriffe der Gründe: [[Begriffe]].

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *Terminal output*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#terminal-output-planned-change-9-own-code).
