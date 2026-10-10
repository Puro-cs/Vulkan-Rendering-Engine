# Start

Diese Seite sagt, was die Render-Engine ist, in welcher Datei gearbeitet wird und wie dieses Wiki zu lesen ist.

## Was die Engine ist

Die Engine ist ein Lehrmaterial für Vulkan und Grafikprogrammierung an der HTWK Leipzig, entwickelt im Rahmen einer Bachelorarbeit; sie ist keine produktionsreife Engine. Sie ist für Studierende gedacht, die noch nie mit Vulkan gearbeitet haben, eigene einfache Shader schreiben und dabei die Grundlagen von Vulkan und Computergrafik kennenlernen. Der Renderer wurde deshalb auf die Teile reduziert, die ein erster Shader berührt.

Technisch ist die Engine eine Portierung der *simple engine* des Vulkan-Tutorials (Holochip / Khronos, Apache-2.0), durch Löschen auf ein Windows-System zum Studium des Renderings reduziert. Eigener Code kam dort hinzu, wo die Studierenden arbeiten: die Sandbox-Schicht mit der Initialisierungskette, den Frame-Aufrufen und der Terminalausgabe, die benannten Pipelines, die Terminalbefehle, die Lichtkomponente und der Template-Shader.

## Die Datei, in der gearbeitet wird

Gearbeitet wird in **einer** Datei: `src/sandbox.cpp`. Sie bindet nur `sandbox.h` ein, und dieser Header enthält keine Vulkan-Typen und keine Engine-Klassen: *the Vulkan API, ImGui, descriptor sets, memory management and the swap chain stay hidden behind it.*

Die Datei hat immer dieselbe Form, in dieser Reihenfolge:

1. die **Initialisierungskette** – acht Aufrufe in fester Reihenfolge in `main()` ([[1 Initialisierungskette und Frame-Ablauf|1-Initialisierungskette-und-Frame‐Ablauf]]),
2. **`SetupScene()`** – Kamera, Lichter, Modelle und Pipelinezuordnung ([[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]]),
3. die **Render-Schleife** – `while (sandbox.IsRunning())` um sechs Frame-Aufrufe ([[1 Initialisierungskette und Frame-Ablauf|1-Initialisierungskette-und-Frame‐Ablauf]]).

Die Beschränkung gilt nur für die Sandbox-Datei: die Engine selbst darf gelesen und verändert werden.

## Branches

| Branch | Inhalt |
|---|---|
| `main` | die **Beispielszene**: eine vollständige `sandbox.cpp` mit Kamera, dem gerichteten Licht `"Sun"`, dem Viking Room und einer Kugel; jede Aufrufgruppe trägt einen Kommentar, der ihren Zweck nennt |
| `learning` | das **Gerüst** für das Aufgabenblatt: dieselbe Datei mit denselben Gruppenkommentaren, in der die Aufrufe der ersten Gruppe der Initialisierungskette fehlen und zwei Aufrufe der Render-Schleife vertauscht sind; eine Zeile, die eine Standardkamera aktiv setzt, ist gegeben. Der Template-Shader trägt hier die drei Kommentare `// ambient`, `// diffuse` und `// specular` |

Die Beispielszene bleibt während aller Aufgaben im Repository; ein Blick nach `main` ist jederzeit erlaubt.

## Aufbau des Repositorys

| Pfad | Was dort liegt |
|---|---|
| `src/sandbox.cpp` | die eigene Datei (siehe oben) |
| `src/sandbox.h` | die Aufrufe, die `sandbox.cpp` zur Verfügung stehen, mit einem Kommentar je Aufruf |
| `src/pipeline_settings.h` | `CullMode` und `PipelineSettings` für `CreatePipeline()`; wird über `sandbox.h` eingebunden und muss nicht geöffnet werden |
| `src/shaders/` | `template.slang` (der Ausgangspunkt für eigene Shader), `pbr.slang` (der Shader der Engine) und die importierten Module `common_types`, `pbr_utils`, `lighting_utils`, `tonemapping_utils`; eigene Shader kommen hierhin |
| `assets/viking_room/` | das einzige Modell im Repository |
| `docs/` | die Dokumentation der Engine (englisch): `ARCHITECTURE.md`, `DELETIONS.md`; in `docs/wiki/` die Seiten dieses Wikis |
| `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json` | das Buildsystem; Bedienung auf der Seite [[Setup]] |

`sandbox.h` bindet `<glm/glm.hpp>` ein: jede Position, Rotation, Skalierung und Farbe, die an die Engine übergeben wird, ist ein `glm::vec3`, z. B. `{2.0f, 0.5f, -2.0f}`.

## Wie dieses Wiki zu lesen ist

Das Wiki ist ein **Nachschlagewerk, kein Kurs**. Es wird von den Aufgaben des Aufgabenblatts aus benutzt: jede Aufgabe nennt unter *Voraussetzung* genau eine Seite, die dabei offen sein sollte, und bei einem Fehler zeigt die Seite [[Fehler und Debugging|Fehler-und-Debugging]], wie er aussieht und wo die richtige Verwendung steht. Jede Seite hat dieselbe Form: ein Satz, wozu sie dient; das kleinste lauffähige Beispiel aus der Engine; die Referenz, die die Aufgabe braucht; am Ende der Link in die Engine-Dokumentation.

Die Engine-Dokumentation unter `docs/` ist die tiefere Schicht: sie beschreibt die Engine als Ganzes und wird hier nicht wiederholt. Wer mehr wissen will als eine Aufgabe braucht, folgt dem Link am Seitenende.

## Seiten

| Seite | Wozu |
|---|---|
| [[Setup]] | Toolchain, Konfigurieren, Bauen, Starten – in Visual Studio und auf der Kommandozeile |
| [[Architektur]] | wie die Engine aufgebaut ist und was sie in den acht Bereichen der Kern-Infrastruktur (KI 1–8) abnimmt |
| [[1 Initialisierungskette und Frame-Ablauf|1-Initialisierungskette-und-Frame‐Ablauf]] | Aufgabe 1: die acht Aufrufe der Initialisierungskette und die sechs eines Frames, ihre Reihenfolge, die Begründung je Aufruf, die Ausgabe der Engine-Konsole |
| [[2 Eigene Pipeline und eigene Szene|2-Eigene-Pipeline-und-eigene-Szene]] | Aufgabe 2: eine eigene Pipeline mit ihren Einstellungen anlegen und Objekte zuweisen; Kamera, Lichter, Modelle, Kugeln, Teile, Transformationen, Terminalbefehle; der Pipeline-Block |
| [[3 Phong-Modell im Shader|3-Phong‐Modell-im-Shader]] | Aufgabe 3: der Template-Shader, die Eingaben eines Shaders, eigene Konstanten, Slang, das Bauen, der Weg der Ausgabe ins Bild, Zwischenwerte als Farbe |
| [[4 Materialien und Referenzgrafiken|4-Materialien-und-Referenzgrafiken]] | Aufgabe 4: die Szene und die Bedingungen der Referenzgrafiken, das Material im Shader, was im Bild außer dem Material wirkt |
| [[Eigene Modelle|Eigene-Modelle]] | ein eigenes Modell mit Texturen in die Engine bringen |
| [[Pipeline-Ansicht|Pipeline-Ansicht]] | die Terminalausgabe lesen: Kette, Pipelines, Frame-Ablauf, Fehlerfall |
| [[Fehler und Debugging|Fehler-und-Debugging]] | die Fehler, die auftreten, und wo die richtige Verwendung steht |
| [[Begriffe]] | die Vulkan-Begriffe der Terminalausgabe, englisch mit deutscher Erklärung |

Engine-Dokumentation: [`docs/ARCHITECTURE.md`](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md) (Einleitung, *The sandbox layer*).
