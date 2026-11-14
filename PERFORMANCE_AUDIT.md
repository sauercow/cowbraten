# Cowbraten Performance Audit

Fecha: 2026-11-13
Rama auditada: `feature/rmlui-icon-pack`
HEAD auditado: `6dfcb66`
Baseline pre-RmlUi: `1dc660b` (padre de `91f8095`, primer commit de RmlUi)

## A. Executive Summary

La ruta normal de gameplay sin documentos visibles ya no ejecuta el árbol de RmlUi: se midieron **0 llamadas efectivas a `Context::Update`, 0 a `Context::Render` y 0 draw calls de RmlUi**. El trabajo residual de modelos fue 0.0029 ms/frame en la ejecución final. El bypass diagnóstico completo no mostró una diferencia causal estable: en distintas repeticiones, la variante normal fue unas veces más rápida y otras más lenta que el bypass, mientras el coste interno medido de UI oculta permaneció entre 0.003 y 0.006 ms. La variación global observada es mayor que ese coste.

La sospecha sobre filtrado de texturas quedó confirmada. En una comparación dentro del mismo proceso, sobre `base/complex` a 1280×720:

- nearest (`bilinear=0`, `trilinear=0`, `aniso=0`): GPU mediana 0.767 ms;
- bilinear + trilinear, sin anisotropía: 0.849–0.851 ms, aproximadamente +10%;
- anisotropía 16×: 0.933 ms, otro +9–10% sobre trilinear y +22% frente a nearest.

En otra secuencia el tramo 16× llegó a +17% frente al tramo base. El efecto es real y dependiente de escena/GPU. Sin embargo, no fue introducido por RmlUi: `trilinear=1`, `bilinear=1` y `aniso=0` son los mismos defaults en vanilla y en Cowbraten actual. La UI añadió controles para esos valores, pero no cambió los defaults. Una configuración persistida con `aniso` activado sí puede explicar una caída considerable.

Se corrigieron cuatro problemas demostrados:

1. El scoreboard ensuciaba 14 variables de data binding cada frame aunque no cambiaran. Su `Context::Update` bajó de 0.0699 a 0.0283 ms (−59.5%).
2. El backend repetía binds de textura, uniformes, VAO y estados de scissor. Los binds bajaron 81→67 en el menú in-game y 22→17 en el scoreboard, sin cambiar draw calls ni apariencia.
3. La geometría compilada retenía punteros a memoria temporal de RmlUi. Un reset OpenGL podía reusar memoria inválida. Ahora conserva una copia propia y liberable.
4. El chat oculto y vacío construía strings y recorría colecciones cada frame. Ahora retorna inmediatamente en ese estado.

La instrumentación incorporada mide CPU, GPU asíncrona, subsistemas de UI, draw calls, cambios de estado, documentos y recursos vivos. Su coste por-frame queda desactivado fuera de una captura.

No se declara recuperación total. El FPS sin UI quedó dentro del rango de vanilla, pero la variación entre ejecuciones fue elevada. Tampoco estuvieron disponibles `perf`, Valgrind, heaptrack, RenderDoc o apitrace; por ello no se midieron allocations globales, CPU por hilo ni VRAM real. La primera apertura de Settings conserva un pico de 83–98 ms debido a carga/compilación inicial de un documento de 4,778 líneas; queda como problema pendiente.

## B. Baseline y metodología

### Entorno

- CPU: AMD Ryzen 5 4600H, 6C/12T.
- GPU: AMD Radeon Graphics, Mesa/radeonsi, OpenGL 4.6.
- SO: Fedora, kernel 7.3.3.
- Compilador: Clang 23.1.5.
- Build de motor: Release, `-O3 -fomit-frame-pointer -ffast-math`.
- RmlUi: CMake `Release`.
- Resolución de benchmark: 1280×720, ventana.
- VSync/FPS caps: `vsync=0`, `maxfps=0`, `menufps=0`, `maxtps=0`.
- Escena: `base/complex`, cámara inicial estática.
- Audio: driver SDL dummy.
- Warm-up: 5 segundos; captura habitual: 6 segundos.

Se extrajo `1dc660b` a un directorio temporal aislado y se compiló con flags Release equivalentes. No se modificó el worktree actual para construir vanilla.

### Variantes

- **A — Vanilla:** `1dc660b`, anterior a RmlUi.
- **B — Actual oculto:** build actual, mapa cargado, cero documentos visibles.
- **C — Actual visible:** menú principal, menú in-game, scoreboard, Settings, Profile y Demos.
- **D — Bypass:** build actual con `ui_perf_bypass=1`, sin `cowui::update()` ni `cowui::render()`.

Ocultar UI y desactivarla fueron pruebas separadas. En B se comprobó que los modelos auxiliares siguen sincronizándose, pero el contexto y el renderer no trabajan. D elimina también esos modelos.

### Métricas

`perf_capture_start <archivo>` reserva memoria y captura CSV sin I/O por frame. `perf_capture_stop` escribe el resultado. Los timer queries GPU usan un anillo de 64 consultas y sólo leen resultados disponibles; nunca llaman `glFinish`, `glFlush` ni esperan activamente. Las muestras GPU no disponibles se escriben como `-1` y el analizador las excluye.

El 1% low y 0.1% low se calculan como la inversa de la media de los frametimes más lentos, no como percentiles de FPS instantáneo. Los percentiles de frametime usan interpolación lineal.

### Variabilidad observada

Tres ejecuciones equivalentes de vanilla dieron 623.65–736.37 FPS; las tres ejecuciones actuales previas dieron 624.80–690.14 FPS; el bypass previo dio 518.78–654.41 FPS. La deriva térmica/DVFS y del driver supera el coste residual de UI oculta. Por eso las optimizaciones se aceptaron principalmente con tiempos internos y contadores, y las comparaciones de filtros se hicieron dentro del mismo proceso.

### Baseline representativo de gameplay oculto

Los valores de Vanilla y Antes son la ejecución mediana de tres repeticiones por FPS promedio. Después es la captura final. No se deben interpretar diferencias pequeñas como significativas sin más repeticiones controladas.

| Métrica | Vanilla | Antes | Después | Diferencia vs vanilla |
|---|---:|---:|---:|---:|
| FPS promedio | 631.37 | 680.34 | 633.14 | +0.28% |
| FPS mediana instantánea | 707.04 | 756.44 | 698.72 | −1.18% |
| FPS 1% low | 263.49 | 272.94 | 339.11 | +28.70% |
| FPS 0.1% low | 161.11 | 156.30 | 167.54 | +3.99% |
| Frametime p50 | 1.414 ms | 1.322 ms | 1.431 ms | +1.20% |
| Frametime p95 | 2.483 ms | 2.400 ms | 2.138 ms | −13.89% |
| Frametime p99 | 3.189 ms | 3.076 ms | 2.510 ms | −21.29% |
| CPU frame time promedio | 1.582 ms | 1.467 ms | 1.576 ms | −0.38% |
| GPU game time promedio | N/A | N/A | 0.762 ms | N/A |
| RmlUi model update | N/A | 0.0032 ms | 0.0029 ms | N/A |
| RmlUi context update | N/A | 0.0000 ms | 0.0000 ms | N/A |
| RmlUi render | N/A | 0.0000 ms | 0.0000 ms | N/A |
| RmlUi draw calls | N/A | 0 | 0 | N/A |

## C. Findings

### PERF-001 — Trabajo de documentos ocultos

- Severidad: alta históricamente; resuelto antes de esta auditoría y verificado.
- Archivo/función: `src/cowbraten/ui/ui.cpp`, `cowui::update()` y `cowui::render()`.
- Causa raíz histórica: actualizar el contexto completo recorría documentos cacheados ocultos; Settings contiene miles de elementos.
- Evidencia: commit `b1910cc` cambió a `Context::Update(false)` y añadió early-outs. Captura B final: 0 context update, 0 context render, 0 draw calls, 0 documentos visibles.
- Coste actual: 0.0029 ms/frame de modelos; contexto y renderer en cero.
- Corrección aplicada en esta auditoría: fast-path adicional para chat vacío.
- Riesgo: bajo; la reapertura fuerza actualización del documento visible y los tests cubren estilos/layout diferidos.

### PERF-002 — Filtrado bilinear/trilinear/anisotrópico

- Severidad: alta cuando anisotropía está habilitada en GPU limitada.
- Archivo/función: `src/engine/texture.cpp`, `setuptexparameters()`; variables `bilinear`, `trilinear`, `aniso`.
- Causa raíz: mayor número/calidad de muestras de textura en GPU, especialmente en superficies oblicuas.
- Evidencia: secuencia continua con timer query GPU:

| Estado | GPU mediana | GPU media | FPS promedio del tramo |
|---|---:|---:|---:|
| nearest, aniso off | 0.767 ms | 0.775 ms | 765.5 |
| bilinear + trilinear, aniso off | 0.849 ms | 0.859 ms | 712.6 |
| bilinear + trilinear, aniso 16× | 0.933 ms | 0.937 ms | 651.8 |
| vuelta a trilinear, aniso off | 0.851 ms | 0.860 ms | 677.9 |

- Aislamiento: cambiar anisotropía o trilinear en runtime no produjo un stall relevante en esta máquina; máximos observados de la secuencia de transición fueron 2.55 y 3.16 ms, respectivamente. El coste demostrado es sostenido en GPU.
- Historial: vanilla y actual comparten defaults `bilinear=1`, `trilinear=1`, `aniso=0`. Los commits `46784a0`, `4aecbb3` y `c804f4d` expusieron controles y callbacks, sin cambiar defaults.
- Corrección: no se cambió la calidad ni la preferencia del usuario. Se añadió el estado del filtro a cada muestra para diagnóstico reproducible.
- Recomendación: verificar el `config.cfg` real del usuario; usar `aniso 0`, `2` u `8` si el presupuesto GPU lo exige.

### PERF-003 — Dirtying incondicional del scoreboard

- Severidad: media.
- Archivo/función: `src/cowbraten/ui/rml_scoreboard.cpp`, `RmlScoreboard::sync()`.
- Causa raíz: 14 escalares se asignaban y marcaban dirty en cada frame, aunque fueran idénticos.
- Evidencia antes: model 0.0208 ms, context update 0.0699 ms.
- Corrección: comparación valor-a-valor; sólo se llama `DirtyVariable` cuando cambia.
- Resultado después: model 0.0168 ms (−19.4%), context update 0.0283 ms (−59.5%). Render 0.0656→0.0619 ms; binds 22→17.
- Riesgo: bajo; todos los escalares siguen sincronizándose y existe test de regresión estático.

### PERF-004 — Estado OpenGL redundante en RmlUi

- Severidad: media.
- Archivo/función: `src/cowbraten/ui/rml_render.cpp`, `RenderGeometry()`, `EnableScissorRegion()`, `SetScissorRegion()`.
- Causa raíz: cada geometría subía translation/use-texture, enlazaba textura/VAO y repetía scissor aunque el estado fuera idéntico.
- Evidencia: menú in-game 132 draw calls, 81 binds; scoreboard 37 draw calls, 22 binds.
- Corrección: caché válida sólo dentro de cada frame RmlUi para textura, VAO, uniformes y scissor. El estado externo aún se guarda/restaura.
- Resultado: menú 67 binds (−17.3%); scoreboard 17 (−22.7%). Render medido del menú 0.1654→0.1576 ms; scoreboard 0.0656→0.0619 ms. Los draw calls no cambian.
- Riesgo: bajo-medio; se preserva backup/restore completo. Requiere prueba manual en drivers Windows.

### PERF-005 — Punteros de geometría inválidos después de reset OpenGL

- Severidad: alta por corrección/estabilidad; impacto de rendimiento indirecto.
- Archivo/función: `src/cowbraten/ui/rml_render.cpp`, `Geometry`, `CompileGeometry()`, `afterglreset()`.
- Causa raíz: `Geometry` guardaba `vertices.data()` e `indices.data()` recibidos de RmlUi. Esa memoria no está garantizada después de `CompileGeometry`, pero se reutilizaba para re-subir buffers tras reset.
- Evidencia: inspección de ciclo de vida y ruta `initgl()`→`uploadgeometry()`.
- Corrección: copia CPU propia de vértices/índices, liberada en el destructor; contadores de bytes vivos.
- Resultado: reset ya no depende de memoria temporal. Coste: espejo CPU explícito; en el ciclo de siete pantallas fueron 1.22 MiB de geometría más 2.38 MiB de texturas retenidas para restauración.
- Riesgo: bajo; consumo de RAM mayor que el puntero colgante anterior, pero ahora es memoria válida y cuantificada.

### PERF-006 — Trabajo de chat vacío

- Severidad: baja.
- Archivo/función: `src/cowbraten/ui/rml_chat.cpp`, `RmlChat::update()`.
- Causa raíz: construcción de cuatro strings y recorridos `remove_if` incluso sin documento, input ni mensajes.
- Corrección: early return sólo cuando el chat está completamente inactivo y vacío.
- Resultado: model update oculto final 0.0029 ms; antes representativo 0.0032–0.0036 ms.
- Riesgo: bajo; si existe input o un mensaje pendiente, se conserva la ruta original.

### PERF-007 — Pico de primera apertura de Settings

- Severidad: media-alta para frametime.
- Archivo/función: `src/cowbraten/ui/ui.cpp::show()`, `RmlSettings::activate()`, `data/cowbraten/ui/default/screens/settings.rml`.
- Causa raíz probable: carga, parseo, estilo y compilación inicial de un documento de 338 KiB, 4,778 líneas y 249 draw calls visibles. Es una inferencia sustentada por el tamaño y el patrón de primera carga; no se aisló cada fase individual.
- Evidencia: máximo 98.42 ms en la captura de transición previa y 83.36 ms en el ciclo final. Las transiciones aisladas de filtros no reprodujeron el pico.
- Corrección aplicada: ninguna; precargar movería el hitch a inicio y aumentaría memoria residente, y dividir el documento es un cambio amplio.
- Riesgo pendiente: microstutter al abrir Settings por primera vez durante una partida online.

### PERF-008 — Recursos al abrir/cerrar pantallas

- Severidad: informativa/media.
- Archivo/función: caché de documentos en `src/cowbraten/ui/ui.cpp`, backend RmlUi.
- Evidencia: tras visitar siete documentos: 1,965 geometrías vivas, 23 texturas y 3.606 MiB de copias CPU del backend. En la segunda pasada no aumentaron documentos, texturas ni geometrías de forma progresiva. Se observaron 1,996 compilaciones y 31 liberaciones durante la primera carga.
- Interpretación: la memoria crece de forma escalonada hasta cachear cada pantalla; no se demostró una fuga durante dos ciclos. DOM, heap general y VRAM no están incluidos.
- Corrección: contadores permanentes de bajo coste, visibles sólo al capturar.

### PERF-009 — Renderizado HiDPI a dimensiones de drawable

- Severidad: alta condicional; hipótesis de regresión no medida en este equipo.
- Archivos: `src/engine/main.cpp`, `src/engine/rendergl.cpp`, `src/cowbraten/ui/ui.cpp`.
- Historial: commits de septiembre habilitaron `SDL_WINDOW_ALLOW_HIGHDPI` y migraron viewport/render targets de tamaño lógico a tamaño de drawable.
- Impacto esperado: con escala 2× por eje se rasterizan aproximadamente 4× píxeles. Esto afecta motor y UI y puede coincidir con una caída “en ambos”.
- Evidencia: diff de historial y rutas actuales; el benchmark se ejecutó en un entorno 1×, por lo que no cuantifica esta hipótesis.
- Recomendación: comparar manualmente window size vs drawable size en el monitor de 27 pulgadas y repetir a resolución física idéntica.

### PERF-010 — Escaneo inicial del navegador de demos

- Severidad: media potencial, no confirmada con una biblioteca grande.
- Archivo/función: `src/cowbraten/ui/demo_library.cpp`, `Library::begin_refresh()`.
- Observación: el parseo de demos se limita a un archivo por frame, pero la enumeración recursiva y ordenación de todas las rutas se hace sincrónicamente al activar.
- Evidencia actual: la biblioteca disponible era pequeña; no se midió un stall relevante representativo.
- Recomendación: si existen miles de demos, perfilar y, sólo entonces, convertir también la enumeración en incremental.

### PERF-011 — Coste de UI activa

- Severidad: media, inherente al diseño actual.
- Evidencia final:

| Pantalla | Model update | Context update | Rml render | Draw calls | Texture binds |
|---|---:|---:|---:|---:|---:|
| Menú in-game | 0.0123 ms | 0.0294 ms | 0.1576 ms | 132 | 67 |
| Scoreboard | 0.0168 ms | 0.0283 ms | 0.0619 ms | 37 | 17 |
| Settings | 0.0031 ms | 0.4309 ms | 0.4422 ms | 249 | 61 |

- Corrección: se redujeron cambios de estado, pero no se intentó reescribir el backend ni hacer batching entre elementos RmlUi.
- Pendiente: Settings domina el coste de UI visible. Reducirlo requiere simplificar/split del DOM o cambios de batching, con mayor riesgo visual.

### PERF-012 — Herramientas no disponibles

- Severidad: limitación de evidencia.
- No instalados: Linux `perf`, Valgrind/Callgrind, heaptrack, RenderDoc, apitrace.
- Disponible: instrumentación interna, timer queries GPU, compilación Release, pruebas funcionales.
- No medido: CPU por hilo, allocations/free globales por frame, heap flamegraph, VRAM y overdraw exacto.

## D. Before/After Benchmarks

### Subsistemas RmlUi activos

| Métrica | Antes | Después | Cambio |
|---|---:|---:|---:|
| Scoreboard model | 0.0208 ms | 0.0168 ms | −19.4% |
| Scoreboard context update | 0.0699 ms | 0.0283 ms | −59.5% |
| Scoreboard render | 0.0656 ms | 0.0619 ms | −5.7% |
| Scoreboard texture binds | 22 | 17 | −22.7% |
| Menú in-game context update | 0.0622 ms | 0.0294 ms | −52.7% observado* |
| Menú in-game render | 0.1654 ms | 0.1576 ms | −4.7% |
| Menú in-game texture binds | 81 | 67 | −17.3% |

\* No se modificó directamente el modelo del menú; parte de esta diferencia puede ser variación entre runs. La reducción de binds sí es determinista por contador.

Los CSV completos y el JSON generado están en `performance_audit_results/`.

## E. Cambios implementados

- `src/cowbraten/ui/rml_render.cpp`, `.h`
  - caché intra-frame de estado OpenGL redundante;
  - ownership correcto de geometría CPU para resets;
  - contadores de draws, compiles/releases, binds, uniforms, VAO, scissor y recursos vivos;
  - profiling apagado fuera de capturas.
- `src/cowbraten/ui/rml_scoreboard.cpp`
  - dirtying incremental de escalares.
- `src/cowbraten/ui/rml_chat.cpp`
  - fast-path oculto/vacío.
- `src/cowbraten/ui/ui.cpp`, `.h`
  - tiempos separados de modelos, Context Update y Context Render;
  - conteo de documentos visibles y coordinación del profiler.
- `src/engine/main.cpp`
  - `perf_capture_start`, `perf_capture_stop`, `ui_perf_bypass`;
  - CSV en memoria, timing CPU y GPU asíncrono;
  - estado de filtros por muestra.
- `tools/performance/run_benchmark.sh`
  - escenarios reproducibles A/B/C/D, filtros, transiciones y ciclo de recursos.
- `tools/performance/analyze.py`
  - FPS promedio/mediana, lows correctos, percentiles y resumen JSON.
- `tests/performance_analysis_tests.py`
  - validación matemática y manejo de GPU no disponible.
- `tests/scoreboard_ui_tests.cpp`, `tests/chat_ui_tests.cpp`, `tests/Makefile`
  - regresiones para los fast paths.
- `performance_audit_results/`
  - capturas representativas y resumen.

No se cambiaron opciones gráficas, calidad, CSS, layout, gameplay, networking, física ni comportamiento de demos.

## F. Problemas pendientes

1. Dividir o cargar progresivamente Settings para eliminar el pico inicial de 83–98 ms.
2. Repetir A/B en el monitor objetivo registrando tamaño lógico y drawable HiDPI.
3. Perfilar una biblioteca real con miles de demos antes de rediseñar su enumeración.
4. Ejecutar heaptrack/Valgrind durante 30–60 minutos y muchos cambios de mapa; no disponibles aquí.
5. Medir VRAM, overdraw y pipeline con RenderDoc en Linux/Windows.
6. Medir CPU por hilo con `perf` o Tracy; no se añadió una dependencia permanente.
7. Ejecutar playback, grabación HD y exportación con una demo fija proporcionada por el usuario. No había una secuencia determinista representativa en el entorno.
8. Validar Windows: restauración OpenGL, DPI, fullscreen, VSync y drivers propietarios.
9. Considerar batching sólo si un perfil GPU confirma que 132–249 draw calls de UI son cuello de botella en hardware objetivo.

## G. Validación

- `make -C src -j2`: correcto, Release.
- `make -C tests -j2 run`: **249 tests pasaron, 0 fallaron**.
- `git diff --check`: correcto.
- `bash -n tools/performance/run_benchmark.sh`: correcto.
- Analizador Python y sus cinco pruebas: correcto.
- Escenarios ejecutados: vanilla, oculto, bypass, menú principal, menú in-game, scoreboard, Demos, Profile, Settings, transición Settings, cambios de filtros, secuencias continuas de filtros y dos ciclos de siete pantallas.
- No se hizo commit ni push.

Limitaciones de validación:

- El mapa estático reduce variabilidad de gameplay, pero no sustituye una demo determinista.
- La advertencia de mapshot `packages/base/base/complex.jpg` apareció en algunos menús; no afecta la escena 3D, pero esa textura faltante debe revisarse por separado.
- GPU vanilla es N/A porque la instrumentación de timer query sólo está en el build actual.
- Las pruebas prolongadas, multiplayer real, HD demo y Windows quedan para revisión manual.

## H. Revisión manual requerida

1. Respaldar el `config.cfg` real y anotar `bilinear`, `trilinear`, `aniso`, resolución, VSync y caps.
2. En el monitor de 27 pulgadas, comparar tamaño de ventana y drawable; probar la misma resolución física en vanilla y actual.
3. Ejecutar una demo fija durante al menos 60 segundos, cinco veces por variante A/B/D.
4. Repetir filtros con `aniso 0`, `2`, `8`, `16` sin cambiar ninguna otra opción.
5. Abrir/cerrar cada menú, Settings, Profile, Demos y scoreboard; comprobar foco, mouse, teclado, Escape y scroll.
6. Cambiar fullscreen, resolución, MSAA y depth buffer varias veces para forzar reset OpenGL; verificar que UI/texto/iconos sobreviven.
7. Confirmar que tras RmlUi el HUD, texto legacy y preview 3D no presentan estado GL corrupto.
8. Probar scoreboard con muchos jugadores, espectadores y ambos equipos; verificar columnas y actualización de ping.
9. Probar chat público/equipo, consola completa, expiración/fade y reapertura de menús.
10. Probar biblioteca grande de demos, filtros, trash, rename, reproducción normal, HD y auto-recording.
11. Mantener una sesión de 30–60 minutos con cambios de mapa y registrar RSS/VRAM.
12. Repetir en Windows y comparar p95/p99, no sólo FPS promedio.

### Comandos de reproducción

```bash
tools/performance/run_benchmark.sh \
  /home/gh/cowbraten \
  /home/gh/cowbraten/src/cowbraten_client \
  /ruta/a/sauerbraten \
  /tmp/cowbraten-benchmark \
  gameplay_hidden 8000

python3 tools/performance/analyze.py \
  /tmp/cowbraten-benchmark/gameplay_hidden/capture.csv \
  --output /tmp/cowbraten-benchmark/summary.json
```

Escenarios disponibles incluyen `main_menu`, `gameplay_hidden`, `gameplay_bypass`, `ingame_menu`, `scoreboard`, `demo_browser`, `profile`, `settings`, `settings_transition`, `anisotropy_transition`, `trilinear_transition`, `filter_sequence_a`, `filter_sequence_b` y `screen_cycle`.
