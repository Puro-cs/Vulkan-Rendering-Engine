# NoS 5 Shader-Schnittstelle

Diese Seite ist die Referenz des Template-Shaders: seine Stufen, die festen Eingaben, die jede eigene Pipeline von der Engine bekommt, eigene Konstanten, das Bauen und der Weg der Ausgabe auf den Bildschirm.

## Beispiel: der Template-Shader

`src/shaders/template.slang` ist der Ausgangspunkt für jeden eigenen Shader: eine Kopie davon unter neuem Namen in `src/shaders/`, dann eine Pipeline daraus ([[NoS 3|NoS-3-Pipeline-Konfiguration]]). Seine Form (Branch `learning`):

```slang
import common_types;

// Input from vertex buffer
struct VSInput {
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float2 UV;
    [[vk::location(3)]] float4 Tangent;

    // Per-instance data
    [[vk::location(4)]] column_major float4x4 InstanceModelMatrix;
    [[vk::location(8)]] float4 InstanceNormal0;
    [[vk::location(9)]] float4 InstanceNormal1;
    [[vk::location(10)]] float4 InstanceNormal2;
};

// Output from vertex shader / Input to fragment shader
struct VSOutput {
    float4 Position : SV_POSITION;
    float3 WorldPos;
    float3 Normal;
    float2 UV : TEXCOORD0;
};

// Bindings
[[vk::binding(0, 0)]] ConstantBuffer<UniformBufferObject> ubo;
[[vk::binding(1, 0)]] Sampler2D baseColorMap;
[[vk::binding(6, 0)]] StructuredBuffer<LightData> lightBuffer;
[[vk::push_constant]] PushConstants material;

[[shader("vertex")]]
VSOutput VSMain(VSInput input)
{
    // Objektkoordinaten -> Welt -> Clip-Space; Weltposition und Normale an PSMain weiterreichen
    ...
}

[[shader("fragment")]]
float4 PSMain(VSOutput input) : SV_TARGET
{
    float2 uv = float2(input.UV.x, 1.0 - input.UV.y);
    float4 baseColor = baseColorMap.Sample(uv);

    // ambient
    // diffuse
    // specular

    return baseColor;
}
```

`VSMain` rechnet die Position vom Modell in den Clip-Space (`proj · view · ubo.model · InstanceModelMatrix · Position`) und reicht Weltposition, Normale und UV weiter; `PSMain` gibt die Basisfarbtextur ohne Beleuchtung zurück. Die drei Kommentare `ambient`, `diffuse`, `specular` sind die Stellen, an die die drei Terme kommen. Die Engine lädt die Datei nur, wenn eine Pipeline aus ihr erzeugt wird.

## Die Stufen und ihre Einstiegspunkte

Jede Shader-Datei braucht `VSMain` (Vertex-Stufe, `[[shader("vertex")]]`) und `PSMain` (Fragment-Stufe, `[[shader("fragment")]]`). `VSMain` wird je Vertex ausgeführt und gibt `VSOutput` zurück; `Position : SV_POSITION` ist die Clip-Space-Position, die anderen Felder werden interpoliert und kommen in `PSMain` als `input` an. `PSMain` wird je Fragment ausgeführt und gibt die Farbe als `float4` (RGBA, linear) zurück.

## Die festen Eingaben

Jede eigene Pipeline benutzt das Layout der PBR-Pipeline, also genau diese Eingaben – die Engine füllt sie jeden Frame.

**Vertex-Attribute** (`VSInput`):

| Location | Feld | Inhalt |
|---|---|---|
| 0 | `Position` (`float3`) | Position im Modellraum |
| 1 | `Normal` (`float3`) | Normale im Modellraum |
| 2 | `UV` (`float2`) | Texturkoordinate |
| 3 | `Tangent` (`float4`) | Tangente |
| 4–7 | `InstanceModelMatrix` (`column_major float4x4`) | Modellmatrix der Instanz (die Knoten-Transformation aus der glTF-Datei) |
| 8–10 | `InstanceNormal0..2` (`float4`) | Normalenmatrix der Instanz, eine **Spalte** je Location – `float3x3(a, b, c)` nimmt Zeilen, das Ergebnis muss transponiert werden: `transpose(float3x3(input.InstanceNormal0.xyz, input.InstanceNormal1.xyz, input.InstanceNormal2.xyz))` |

**Set 0, Binding 0** – `ConstantBuffer<UniformBufferObject> ubo`. Die Felder, die gefüllt werden:

| Feld | Inhalt |
|---|---|
| `model`, `view`, `proj` (`float4x4`) | Modellmatrix des Objekts (`SetPosition` / `SetRotation` / `SetScale`), View- und Projektionsmatrix der aktiven Kamera |
| `camPos` (`float4`) | Position der aktiven Kamera in Weltkoordinaten (`.xyz`) |
| `lightCount` (`int`) | Anzahl der Lichter in `lightBuffer` |
| `exposure`, `gamma`, `scaleIBLAmbient` (= 1,0), `screenDimensions`, `padding0` | von der Engine gesetzt; die übrigen Felder des Structs sind Reste entfernter Funktionen und werden nie geschrieben |

**Set 0, Bindings 1–5** – die Materialtexturen als `Sampler2D`: 1 `baseColorMap`, 2 Metallic-Roughness, 3 Normal Map, 4 Occlusion, 5 Emissive. Der Template-Shader deklariert nur Binding 1. Abtasten: `baseColorMap.Sample(uv)` mit `uv = float2(input.UV.x, 1.0 - input.UV.y)` (V-Koordinate gespiegelt).

**Set 0, Binding 6** – `StructuredBuffer<LightData> lightBuffer`, Fragment-Stufe; `ubo.lightCount` Einträge, jeden Frame aus allen Lichtern der Szene neu aufgebaut ([[NoS 6|NoS-6-Kamera-und-Licht]]):

| Feld | Inhalt |
|---|---|
| `lightType` (`int`) | 0 Punktlicht, 1 gerichtetes Licht, 2 Spotlight, 3 Licht aus einem emissiven Material |
| `position` (`float4`) | gerichtetes Licht (`lightType == 1`): `position.xyz` ist die **Richtung, in die das Licht läuft** (`w` = 0) – die Richtung zum Licht ist also `normalize(-light.position.xyz)`; Punkt- und Spotlight: die Position (`w` = 1), die Richtung zum Licht `normalize(light.position.xyz - worldPos)` |
| `color` (`float4`) | `color.rgb` = Farbe × Intensität (`SetColor()` × `SetIntensity()`) |
| `direction` (`float4`) | Richtung eines Spotlights (die −Z-Achse seines Transforms) |
| `range`, `innerConeAngle`, `outerConeAngle` (`float`) | Reichweite; Kegelwinkel in **Radiant** |
| `lightSpaceMatrix` (`float4x4`) | nicht benutzt |

**Push Constants** – `PushConstants material`, Fragment-Stufe: `baseColorFactor` (`float4`), `metallicFactor`, `roughnessFactor`, die Textur-Indizes (`baseColorTextureSet` usw.; negativ = keine Textur), `alphaMask`, `alphaMaskCutoff`, `emissiveFactor`, `emissiveStrength`, `transmissionFactor`, `useSpecGlossWorkflow`, `glossinessFactor`, `specularFactor`, `ior`, `hasEmissiveStrengthExt`. Die Werte kommen aus dem glTF-Material; der Shader der Engine multipliziert `baseColorMap.Sample(uv) * material.baseColorFactor`.

**Set 1, Binding 0** – das Off-Screen-Farbbild der undurchsichtigen Szene; nur die Glas-Pipeline der Engine liest es.

## Eigene Konstanten

Es gibt keine C++-Schnittstelle für eigene Parameter: ein eigener Wert ist eine Konstante in der Shader-Datei, in der Form, die die Module der Engine benutzen:

```slang
static const float PI = 3.14159265359;
static const float3 kSpec = float3(1.0, 1.0, 0.0);
```

Ändern heißt: Datei ändern, bauen, starten.

## Slang, das ein Beleuchtungs-Shader braucht

Typen `float2`, `float3`, `float4`, `float3x3`, `float4x4`; Funktionen `mul(a, b)` (Matrix × Vektor), `dot`, `normalize`, `length`, `max`, `pow`, `saturate`, `transpose`; Strukturfelder mit `.`, Swizzles wie `.xyz`; `[[vk::binding(b, s)]]`, `[[vk::push_constant]]`, `StructuredBuffer<T>` mit `[i]`; eine `for`-Schleife `for (uint i = 0; i < (uint)ubo.lightCount; ++i)`; `static const`. `reflect` wird für die Form der Vorlesungsformel (Halbvektor) nicht gebraucht.

## Bauen

CMake übersetzt jede `src/shaders/*.slang` beim **Bauen** (`slangc ... -target spirv -profile spirv_1_3`, dann `spirv-opt`) und kopiert die `.spv` nach `src/shaders/` – erst, wenn das Ziel `shaders` fehlerfrei durchlief. Ein Slang-Fehler erscheint daher **in der Build-Ausgabe** als Fehler des Ziels `shaders` (Datei, Zeile, Meldung), nicht zur Laufzeit. Eine neue Datei braucht vorher ein erneutes Konfigurieren ([[Setup]]).

## Der Weg der Ausgabe auf den Bildschirm

`PSMain` gibt eine **lineare** Farbe zurück. In einer Pipeline ohne `blending` geht sie in das Off-Screen-Bild, und der *composite*-Pass der Engine wendet die Belichtung (*Exposure*, Standard **1,2**, Schieberegler im Panel) und ein filmisches Tone Mapping an, bevor sie auf dem Bildschirm erscheint. Mit `blending` wird direkt auf das Swap-Chain-Bild gezeichnet, nach dem *composite*-Pass, ohne Tone Mapping.

Folge für das Debuggen mit Farben: Shader-Variablen lassen sich nicht drucken, aber als Farbe ausgeben – `return float4(n * 0.5 + 0.5, 1.0);` zeigt die Normale, `return float4(diffuse, 0.0, 0.0, 1.0);` einen Term im Rotkanal. Weil die Ausgabe skaliert und getont wird, wird eine solche Farbe nach **Richtung und Kanal** gelesen (wo ist es hell, wo dunkel, welcher Kanal), nicht nach ihrem Zahlenwert.

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *Shaders*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#shaders-srcshaders) und [*What a mesh shader gets from the engine*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#what-a-mesh-shader-gets-from-the-engine), [`docs/BUILD.md`, *Commands*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/BUILD.md#commands-run-from-the-repository-root).
