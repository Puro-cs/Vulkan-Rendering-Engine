# 3 Phong-Modell im Shader

Diese Seite ist das Nachschlagewerk zu Aufgabe 3: der Template-Shader, die Eingaben, die die Engine jedem eigenen Shader liefert, mit ihren Namen, eigene Konstanten, das Slang, das ein Beleuchtungs-Shader braucht, und der Weg vom Rückgabewert von `PSMain` zum Bild im Fenster.

## Beispiel: der Template-Shader

Aus `src/shaders/template.slang`, Branch `learning` (auf `main` ohne die drei Kommentare `// ambient`, `// diffuse` und `// specular`). Darüber stehen in der Datei `import common_types;` und die Vertex-Eingaben `VSInput`:

```slang
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

// Vertex shader entry point
[[shader("vertex")]]
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4x4 instanceModelMatrix = input.InstanceModelMatrix;
    float4 worldPos = mul(ubo.model, mul(instanceModelMatrix, float4(input.Position, 1.0)));
    output.Position = mul(ubo.proj, mul(ubo.view, worldPos));
    output.WorldPos = worldPos.xyz;

    // Transform normals correctly: first by the per-instance normal matrix,
    // then by the entity model 3x3 (avoid double-applying instance transform).
    float3x3 instNormal = transpose(float3x3(input.InstanceNormal0.xyz, input.InstanceNormal1.xyz, input.InstanceNormal2.xyz));
    float3x3 model3x3 = (float3x3)ubo.model;
    float3 worldNormal = normalize(mul(model3x3, mul(instNormal, input.Normal)));
    output.Normal = worldNormal;

    output.UV = input.UV;

    return output;
}

// Fragment shader entry point
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

- `VSMain` läuft je Vertex: Es rechnet die Position aus dem Modellraum über die Welt in den Clip-Space und reicht die Weltposition (`WorldPos`), die Normale in Weltkoordinaten (`Normal`) und die Texturkoordinate (`UV`) an `PSMain` weiter.
- `PSMain` läuft je Fragment, bekommt diese Werte zwischen den Vertices interpoliert als `input` und gibt die Farbe als `float4` zurück: Rot, Grün, Blau und Alpha. Im Template ist das die Farbe der Basisfarbtextur, ohne Beleuchtung.
- An die Kommentare `// ambient`, `// diffuse` und `// specular` kommen die drei Terme; die `return`-Zeile bleibt die letzte Zeile von `PSMain`.

## Die Eingaben eines Shaders

Jede eigene Pipeline übernimmt das Layout der Pipeline `"pbr"`, und die Engine füllt diese Eingaben in jedem Frame. Die Namen gelten in `PSMain` des Templates:

| Eingabe | Name im Shader | Typ | Bedeutung |
|---|---|---|---|
| Normale | `input.Normal` | `float3` | die Normale der Fläche in Weltkoordinaten, von `VSMain` normalisiert und zwischen den Vertices interpoliert |
| Weltposition | `input.WorldPos` | `float3` | die Position des Oberflächenpunkts, den das Fragment zeigt, in Weltkoordinaten |
| Kameraposition | `ubo.camPos.xyz` | `float4`, davon `.xyz` | die Position der aktiven Kamera in Weltkoordinaten |
| Anzahl der Lichter | `ubo.lightCount` | `int` | wie viele Einträge `lightBuffer` hat |
| ein Licht | `lightBuffer[i]` | `LightData` | Eintrag `i`, von `0` bis `ubo.lightCount - 1`. Die Beispielszene hat genau ein Licht, `"Sun"`, also `lightBuffer[0]` |
| Lichtrichtung | `lightBuffer[0].position.xyz` | `float4`, davon `.xyz` | bei einem gerichteten Licht (`lightType == 1`) die Richtung, in die das Licht scheint (`w` = 0); bei einem Punkt- oder Spotlicht die Position des Lichts (`w` = 1) |
| Lichtfarbe | `lightBuffer[0].color.rgb` | `float4`, davon `.rgb` | Farbe mal Intensität des Lichts, also `SetColor()` × `SetIntensity()` |
| Lichttyp | `lightBuffer[0].lightType` | `int` | 0 Punktlicht, 1 gerichtetes Licht, 2 Spotlight, 3 Licht eines leuchtenden Materials |
| Basisfarbe | `baseColor` | `float4` | die Farbe der Basisfarbtextur an diesem Fragment, im Template aus `baseColorMap.Sample(uv)`. Eine Kugel hat die weiße Standardtextur der Engine |

Die übrigen Eingaben (die Matrizen `ubo.model`, `ubo.view` und `ubo.proj`, die weiteren Materialtexturen, die Materialfaktoren in `material`, Reichweite und Kegel eines Spotlights) braucht das Phong-Modell nicht.

## Eigene Konstanten

Eigene Werte, etwa die Konstanten des Materials, stehen als Konstanten in der Shader-Datei, oberhalb von `PSMain`. Eine C++-Schnittstelle für eigene Parameter gibt es nicht. Die Form, die die Module der Engine benutzen (`common_types.slang`):

```slang
static const float PI = 3.14159265359;
```

Ein Vektor wird ebenso deklariert, mit `float3` und drei Werten: `static const float3 name = float3(x, y, z);`. Ändern heißt: Datei ändern, bauen, starten.

## Slang, das ein Beleuchtungs-Shader braucht

| Schreibweise | Bedeutung |
|---|---|
| `float3(x, y, z)`, `float4(x, y, z, w)`, `float4(v, 1.0)` | einen Vektor bilden; `float4(v, 1.0)` hängt an einen `float3 v` die vierte Komponente an |
| `v.xyz`, `c.rgb`, `v.x` | Komponenten eines Vektors auswählen |
| `a + b`, `a - b`, `a * b` | bei zwei Vektoren komponentenweise; `s * v` multipliziert jede Komponente von `v` mit der Zahl `s` |
| `q - p` | der Vektor vom Punkt `p` zum Punkt `q` |
| `dot(a, b)` | das Skalarprodukt zweier Vektoren |
| `normalize(v)` | normalisiert `v`: der Vektor mit derselben Richtung und der Länge 1 |
| `length(v)` | die Länge eines Vektors |
| `max(a, b)` | der größere der beiden Werte, bei Vektoren je Komponente |
| `pow(x, n)` | x hoch n; `x` darf nicht negativ sein, für negatives `x` ist das Ergebnis nicht definiert |
| `saturate(x)` | x auf den Bereich 0 bis 1 begrenzt |

**Das Skalarprodukt und die Richtung.** Für zwei Vektoren der Länge 1 ist `dot(a, b)` der Kosinus des Winkels zwischen ihnen: 1, wenn beide in dieselbe Richtung zeigen, 0, wenn sie senkrecht aufeinander stehen, und negativ, wenn sie voneinander wegzeigen. Deshalb werden beide Vektoren vorher normalisiert. Die Vorlesung schreibt das Skalarprodukt aus Normale und Lichtrichtung in der Implementierung als `max(dot(n, l), 0.0)` mit normalisierten Vektoren `n` und `l`: Negative Werte werden zu 0, Rückseiten haben keine Beleuchtung (Vorlesungsskript, Folie VI-14).

## Bauen

Der Build übersetzt jede `.slang`-Datei in `src/shaders/` außer den vier Modulen, die andere Shader importieren, mit `slangc` aus dem Vulkan SDK nach SPIR-V und kopiert die `.spv`-Dateien nach `src/shaders/`, sobald alle Shader fehlerfrei übersetzt sind. Ein Fehler im Shader erscheint deshalb beim Bauen in der Build-Ausgabe als Fehler des Ziels `shaders`, mit Datei, Zeile und Meldung, und nicht in der Engine-Konsole. Bis zum nächsten fehlerfreien Build lädt die Engine die letzte fehlerfreie `.spv`. Jede Shader-Datei braucht die Einstiegspunkte `VSMain` und `PSMain`.

## Vom Rückgabewert zum Bild

`PSMain` gibt eine lineare Farbe zurück. Die Engine schreibt sie in ein Off-Screen-Farbbild, das das Format der Swap Chain hat, mit 8 Bit je Kanal; ein Wert über 1 wird dabei auf 1 abgeschnitten. Danach multipliziert der *composite*-Pass das Bild mit der Belichtung (*Exposure*, Standard 1,2, Schieberegler im Panel *Renderer* der Engine) und bildet es mit einem filmischen Tone Mapping ab, bevor es im Fenster erscheint. Daraus folgt:

- Die Helligkeit im Fenster ist nicht der Wert, den `PSMain` zurückgibt.
- Ergibt die Rückgabe in einem Kanal mehr als 1, wird dieser Kanal dort abgeschnitten: Die Fläche wird gleichmäßig hell, und ein Glanzlicht darin ist nicht zu sehen. Die Sonne der Beispielszene hat die Intensität 3, ihre Lichtfarbe ist also (3; 3; 3).

## Zwischenwerte als Farbe ausgeben

Shader-Variablen lassen sich nicht drucken, aber als Farbe ausgeben: Statt der Summe der Terme gibt die `return`-Zeile einen Zwischenwert zurück, als `float4` mit Alpha 1. Die Normale, ihre Komponenten x, y und z als Rot, Grün und Blau, von −1 bis 1 auf 0 bis 1 verschoben:

```slang
    return float4(normalize(input.Normal) * 0.5 + 0.5, 1.0);
```

Ein Term `t` vom Typ `float3` oder ein einzelner Wert `s` vom Typ `float` als Grau:

```slang
    return float4(t, 1.0);
    return float4(s, s, s, 1.0);
```

Wegen des Abschneidens, der Belichtung und des Tone Mappings wird eine solche Farbe danach gelesen, wo das Bild hell oder dunkel ist und welcher Farbkanal überwiegt, nicht nach ihrem Zahlenwert.

## Die Kamera bewegen

Die Kamerasteuerung der Engine bewegt die aktive Kamera: `W` und `S` vor und zurück, `A` und `D` nach links und rechts, `Q` und `E` nach oben und unten; mit gedrückter linker Maustaste dreht die Maus die Kamera. Unbewegt ist die Kamera direkt nach dem Start; ein Neustart stellt die Startposition aus `SetupScene()` wieder her.

Engine-Dokumentation: [`docs/ARCHITECTURE.md`, *Shaders*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#shaders-srcshaders) und [*One frame*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ARCHITECTURE.md#one-frame-rendererrender-renderer_renderingcpp).
