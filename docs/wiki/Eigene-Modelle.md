# Eigene Modelle

Diese Seite zeigt, wie ein eigenes Modell mit seinen Texturen in die Engine kommt: Dateiformat, Texturformat, Ablageort und die Regeln, nach denen die Engine die Datei liest.

## Beispiel: ein eigenes Modell laden

Das Modell liegt in einem eigenen Ordner neben dem Viking Room und wird wie dieser geladen:

```
assets/
  viking_room/   viking_room.gltf, viking_room.bin, viking_room.ktx2
  haus/          haus.gltf, haus.bin, haus_basecolor.ktx2
```

```cpp
	SceneObject *haus = sandbox.LoadModel("Haus", "../assets/haus/haus.gltf");
	haus->SetPosition({0.0f, 0.0f, -3.0f});
```

Der Pfad ist relativ zum Arbeitsverzeichnis `src/` ([[Setup]]); Texturpfade in der glTF-Datei (`uri`) werden relativ zum Ordner der Modelldatei aufgelöst.

## Dateiformat

- `LoadModel()` liest `.gltf` (JSON, mit der `.bin` daneben) und `.glb` (binär; gewählt, wenn der Dateiname `.glb` enthält).
- **Geometrie:** die Vertex-Attribute `POSITION`, `NORMAL`, `TEXCOORD_0`, `TANGENT` (fehlende Tangenten werden erzeugt, MikkTSpace). Die Knotenhierarchie wird durchlaufen; die Transformation jedes Knotens (`matrix` oder Translation·Rotation·Skalierung) wird die Modellmatrix seines Meshes, ein von mehreren Knoten benutztes Mesh wird instanziert. Die Platzierung aus dem Modellierwerkzeug bleibt also erhalten; `SetPosition` / `SetRotation` / `SetScale` des Objekts wirken obendrauf. Der Viking Room trägt in der Datei eine Drehung um +90° um X, die das Beispiel mit −90° ausgleicht – ob ein eigener Export eine solche Korrektur braucht, zeigt der erste Ladeversuch.
- **Materialien:** jedes Material der Datei wird ein Teil des Objekts (`Part()` auf [[NoS 4|NoS-4-Geometrie-und-Assets]] spricht es mit dem Materialnamen an). Gelesen werden `pbrMetallicRoughness` (Basisfarbtextur und -faktor, Metallic-Roughness-Textur und -faktoren), `normalTexture`, `occlusionTexture`, `emissiveTexture` mit Faktor, `alphaMode` (`MASK` mit Cutoff → Alpha-Test; `BLEND` → die gemischte Pipeline der Engine) sowie `KHR_materials_pbrSpecularGlossiness`, `KHR_materials_emissive_strength` und `KHR_materials_transmission` (→ die Glas-Pipeline).
- **Lichter und Kameras in der Datei:** `KHR_lights_punctual`-Lichter werden importiert (ihre Intensität × 1/638) und an die Lichter der Szene angehängt. Die **erste Kamera** der Datei wird in die Entity namens `Camera` geschrieben (Position, Rotation, Blickwinkel, Clip-Ebenen; angelegt, falls es sie nicht gibt) – eine mit `CreateCamera("Camera")` erzeugte Kamera wird von einem Modell mit Kamera überschrieben. Abhilfe: der eigenen Kamera einen anderen Namen geben oder die Kamera aus dem Export entfernen.

## Texturen: nur KTX2

Die Engine dekodiert ausschließlich **KTX2**. Ein glTF, das PNG oder JPEG referenziert, lädt seine Geometrie mit der Standard-Albedo und druckt `Non-KTX2 images are not supported by the custom image loader (use KTX2).` sowie `Warning: No decoded bytes for baseColor texture index 0`.

- **Verweis in der Datei:** ein `images[]`-Eintrag mit externer `.ktx2` (`"mimeType": "image/ktx2", "uri": "<Datei>.ktx2"` – die Form des Viking Room) oder eine `KHR_texture_basisu`-Quelle; eine in eine `.glb` eingebettete KTX2 wird ebenfalls gelesen (nur ihre Stufe 0).
- **Inhalt:** unkomprimiertes RGBA8 oder Basis-superkomprimiert (beim Laden nach BC7, BC3, BC1 oder RGBA32 transkodiert, je nach GPU). Die Mip-Stufen der Datei werden genommen, wie sie sind; eine Datei mit einer Stufe wird ohne Mipmaps abgetastet.
- **Umwandlung** eines PNG mit `toktx` aus KTX-Software (nicht Teil des Vulkan SDK):
  ```
  toktx --t2 --target_type RGBA --lower_left_maps_to_s0t0 haus_basecolor.ktx2 haus_basecolor.png
  ```
  (vier Kanäle; Zeilen von unten nach oben gespeichert). Mit `--genmipmap` erzeugt `toktx` die Mip-Stufen in der Datei. Die Engine-Dokumentation vermerkt, dass dieser Befehl auf der Entwicklungsmaschine noch nicht ausprobiert wurde.
- **Farbraum nach Dateiname:** ob eine Textur als sRGB oder linear abgetastet wird, entscheidet die Engine am **Pfad** der Textur, nicht am glTF-Slot oder am KTX2-Header: enthält der Pfad `basecolor`, `base_color`, `albedo`, `diffuse`, `specgloss`, `specularglossiness` oder `emissive` (Groß-/Kleinschreibung egal), wird sRGB gewählt, sonst linear `UNORM`; das Format der Datei wird auf diese Wahl umgesetzt. Eine eigene Basisfarbtextur sollte deshalb eines dieser Wörter im Namen tragen (`haus_basecolor.ktx2`). `viking_room.ktx2` enthält keines – die Farbtextur des Raums wird linear abgetastet.

## Checkliste

1. Modell als `.gltf` + `.bin` (oder `.glb`) exportieren, Texturen als PNG daneben.
2. Jede Textur nach KTX2 wandeln, Basisfarbtexturen mit `basecolor` im Namen.
3. In der `.gltf` die `images[].uri` auf die `.ktx2`-Dateien setzen, `mimeType` `image/ktx2`.
4. Ordner nach `assets/<name>/`, laden mit `"../assets/<name>/<Datei>.gltf"`.
5. Beim ersten Start die Warnungen im Terminal lesen ([[Fehler und Debugging|Fehler-und-Debugging]]) und die Ausrichtung des Modells prüfen.

Engine-Dokumentation: [`docs/ROADMAP.md`, *Scene*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/ROADMAP.md#scene), [`docs/BUILD.md`, *Run* und *Known issues*](https://github.com/Puro-cs/Vulkan-Rendering-Engine/blob/main/docs/BUILD.md#run).
