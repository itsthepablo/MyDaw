# Estructura de la Carpeta `Source/Playlist/`

Documentación de todos los archivos del sistema de Playlist del DAW MyPianoRoll.
Cada módulo tiene una responsabilidad única para mantener el código organizado y mantenible.

---

## 📄 Archivos Raíz

### `PlaylistComponent.h / .cpp`
**Componente principal del Playlist.**
- Contiene la clase `PlaylistComponent`, que es el God Object central reducido.
- Maneja el estado global: `hZoom`, `hS`, `vS`, `trackHeight`, `clips`, `playheadAbsPos`, `selectedClipIndices`.
- Coordina el ciclo de vida: constructor, destructor, `timerCallback`, `resized`, `paint`.
- **`paint()`** delega a `PlaylistGridRenderer` y `PlaylistRenderer` para todo el renderizado visual.
- Gestiona los eventos de mouse (`mouseDown`, `mouseDrag`, `mouseUp`, `mouseMove`, `mouseWheelMove`).
- Gestiona el drag & drop externo (archivos de audio) e interno (clips entre pistas).
- Expone callbacks públicos: `onPlayheadSeekRequested`, `onVerticalScroll`, `getPlaybackPosition`.

---

### `PlaylistClip.h`
**Definición de la estructura `TrackClip`.**
- Define el struct `TrackClip` que representa un clip en la playlist.
- Campos: `trackPtr` (pista propietaria), `startX` (posición en unidades de tiempo), `width`, `name`, `linkedAudio`, `linkedMidi`.
- Es la unidad de datos fundamental que conecta pistas con contenido de audio o MIDI.

---

### `PlaylistAnalyzer.h / .cpp`
**Grabación de datos de análisis en tiempo real.**
- Función estática: `recordAnalysisData(tracksRef, masterTrackPtr, playheadAbsPos, isDraggingTimeline)`
- Detecta pistas de tipo `TrackType::Loudness`, `TrackType::Balance` y `TrackType::MidSide`.
- Lee los datos del master track (`postLoudness`, `postBalance`, `postMidSide`) y los graba en el historial de la pista correspondiente usando `addPoint()`.
- Llamado desde `PlaylistComponent::timerCallback()` a 60 Hz.

---

### `PlaylistCoordinateUtils.h`
**Utilidades de coordenadas (header-only).**
- `getTrackY(Track*, tracksRef, topOffset, vBar, trackHeight)` → Y en pantalla de un track. Retorna -1 si el track está oculto (dentro de carpeta colapsada).
- `getTrackAtY(y, topOffset, vBar, tracksRef, trackHeight)` → Índice del track en una posición Y del mouse.
- `getClipAt(x, y, hNavigator, hZoom, topOffset, vBar, tracksRef, trackHeight, clips)` → Índice del clip en una posición (x, y) del mouse.
- Incluye el comentario `// BUG FIX` que documenta el comportamiento de tracks ocultos por carpetas.

---

### `PlaylistActionHandler.h / .cpp`
**Acciones de edición sobre clips.**
- `deleteSelectedClips(playlist)` → Elimina todos los clips seleccionados.
- `deleteClipsByName(playlist, name, isMidi)` → Elimina clips por nombre (usado al borrar patrones MIDI del Piano Roll).
- `purgeClipsOfTrack(playlist, track)` → Elimina todos los clips de una pista específica.
- `handleDoubleClick(playlist, event)` → Lógica del doble clic (abrir Piano Roll, crear clip, etc.).

---

### `PlaylistDropHandler.h / .cpp`
**Manejo de drag & drop.**
- `processExternalFiles(files, x, y, playlist)` → Procesa archivos de audio soltados desde el explorador del SO.
- `processInternalItem(sourceDetails, playlist)` → Procesa ítems arrastrados desde el Browser interno del DAW (plugins, samples, patrones).

---

## 📁 Subcarpeta `View/`

Contiene los renderizadores visuales. Se invocán desde `PlaylistComponent::paint()`.

### `PlaylistGridRenderer.h / .cpp`
**Renderizado de la rejilla y elementos temporales.**
- `drawGrid(...)` → Fondo alternado cada 4 compases (FL Studio Style), líneas verticales dinámicas con culling, y **separadores horizontales de 2px** entre pistas.
- `drawTimelineRuler(...)` → Barra de regla con números de compás centrados sobre la línea de beat, con densidad adaptativa (`measureMod`).
- `drawPlayhead(...)` → Cabezal de reproducción con estela difuminada (40px), línea blanca y triángulo superior.

### `PlaylistRenderer.h / .cpp`
**Renderizado de clips, carpetas, análisis y overlays.**
- `drawFolderSummaries(...)` → Resúmenes de carpeta estilo Reaper: recolecta descendientes de forma recursiva, calcula el rango global de notas MIDI y dibuja miniaturas de onda y MIDI en la fila de la carpeta.
- `drawTracksAndClips(...)` → Renderiza todos los clips con culling horizontal y vertical inteligente. Incluye: blindaje de coordenadas anti-overflow, transparencia `0.3f` para clips muteados, shell de audio con header + nombre, y delegación a `WaveformRenderer` / `MidiClipRenderer`.
- `drawSpecialAnalysisTracks(...)` → Renderiza pistas de Loudness, Balance y Mid-Side usando sus respectivos renderers.
- `drawAutomationOverlays(...)` → Renderiza las capas de automatización visibles (Ableton Style) usando `AutomationRenderer::drawAutomationLane`.
- `drawDragOverlays(...)` → Overlay azul translúcido con texto diferenciado para drag externo ("SUELTA TU AUDIO AQUI") vs interno ("SUELTA EL CLIP AQUI").
- `drawMinimap(...)` → Minimapa en el navegador horizontal: muestra todos los clips escalados con el color de su pista.

También define el struct `PlaylistViewContext` que agrupa todos los parámetros de vista para no saturar las firmas de los métodos.

---

## 📁 Subcarpeta `ScrollBar/`

### `PlaylistNavigator.h / .cpp`
**Barra de navegación horizontal (minimap superior).**
- Scroll horizontal y zoom del Playlist.
- Dibuja el minimapa de clips.
- Expone callbacks: `onScrollMoved`, `onZoomChanged`, `onDrawMinimap`.

### `VerticalNavigator.h / .cpp`
**Barra de scroll vertical.**
- Controla el desplazamiento vertical entre pistas.
- Expone callbacks: `onScrollMoved`.

---

## 📁 Subcarpeta `PlaylistMenuBar/`

### `PlaylistMenuBar.h / .cpp`
**Barra de herramientas del Playlist.**
- Botones de selección de herramienta activa (Pointer, Scissor, Eraser, Mute).
- Control de snap (cuantización del grid).
- Botones de Undo/Redo.
- Expone callbacks: `onToolChanged`, `onSnapChanged`, `onUndo`, `onRedo`.

---

## 📁 Subcarpeta `Tools/`

Herramientas de edición intercambiables. Todas heredan de `PlaylistTool`.

### `PlaylistTool.h`
**Interfaz base para todas las herramientas.**
- Declara los métodos virtuales: `mouseDown`, `mouseDrag`, `mouseUp`, `mouseMove`.

### `PointerTool.h`
**Herramienta de selección y movimiento (la más compleja, ~23KB).**
- Selección por click y rectángulo.
- Movimiento y redimensionado de clips.
- Snap al grid.

### `ScissorTool.h`
**Herramienta de corte.**
- Divide un clip en dos en el punto de click.

### `EraserTool.h`
**Herramienta de borrado.**
- Elimina el clip bajo el cursor al hacer click.

### `MuteTool.h`
**Herramienta de silenciado.**
- Activa/desactiva el mute del clip bajo el cursor.
