# Performance audit results

Capturas representativas generadas por `tools/performance/run_benchmark.sh`.

- `vanilla_gameplay.csv`: commit pre-RmlUi `1dc660b`.
- `before_gameplay.csv`: build actual antes de las optimizaciones de esta auditoría.
- `after_gameplay.csv`: build final, UI oculta.
- `after_gameplay_bypass.csv`: build final con procesamiento/render UI desactivado.
- `before_*` / `after_*`: comparación de menú in-game y scoreboard.
- `after_settings.csv`: coste sostenido con Settings visible.
- `filter_sequence.csv`: nearest, trilinear y anisotropía 16× dentro del mismo proceso.
- `resource_cycle.csv`: dos visitas a siete pantallas, con recursos vivos.
- `settings_transition.csv`: pico de primera apertura de Settings.
- `summary.json`: salida de `tools/performance/analyze.py`.

Los CSV antiguos no contienen las columnas añadidas posteriormente (GPU y recursos vivos); el analizador acepta ambos formatos.
