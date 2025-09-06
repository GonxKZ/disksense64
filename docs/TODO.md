# TODOs: Build, CI y Mejoras Técnicas

Estado 2025-09-05
- Safety Mode global no destructivo: añadido core/safety + gates en cleanup/dedupe/secure_delete. [done]
- UI Safety Banner + desactivar/ocultar opciones de borrado permanente. [done]
- Visual: tema oscuro modernizado (QSS), coherencia en botones/inputs. [done]
- Visualización: gráfico de distribución por edad (últimos 12 meses). [done]
- Limpieza de repo: eliminado `test.c`, `libs/security/security.c.backup` y `build.sh` redundante en raíz. [done]

- CMake (network): Reemplazar `exec_program(pcap-config ...)` por `execute_process()` o `find_package(PCAP)` en `libs/network/CMakeLists.txt` para cumplir CMP0153.
- CI: Añadir workflow de GitHub Actions que instale dependencias (Qt6, libarchive, libpcap, libbsd, xvfb) y ejecute `./build.sh` (con `xvfb-run`).
- Benchmarks: Reducir tiempo de `perf_benchmarks` parametrizando tamaños/iteraciones para CI (objetivo <30s).
- Charts: Sustituir placeholders en `ChartWidget` por renderizado real en `core/gfx/charts.{h,cpp}` (tarta/barras/líneas, interacción y tooltips). [done]
- 3D: Completar renderer OpenGL de `TreemapWidget3D` (shaders, buffers, navegación fluida, picking).
- Plugins: Completar `PluginManager` (carga/descarga runtime, firma, actualización) y tests dedicados.
- Exportación GUI: Añadir diálogo de exportación (JSON/CSV) conectado al índice activo con selección de campos.
- Accesibilidad: Revisar contraste, tamaños, atajos y soporte teclado; añadir pruebas de accesibilidad.

---

# TODOs: Features de Producto

- Limpieza avanzada: más patrones de residuos (cachés de navegadores, miniaturas, compilados), modo “analizar sin eliminar”, y “lista blanca”. [in-progress]
- Limpieza con cuarentena y deshacer: mover a directorio seguro, manifest y `undo`. [done]
- Confirmaciones de seguridad: diálogo con texto “DELETE” antes de acciones destructivas. [done]
- Vista duplicados en GUI con aplicar acción y opciones (full hash, min size, hardlink/move/delete). [done]
- Visualización: selector de tipo de gráfico (Pie/Bar/Line), filtro por nombre y combinación con treemap. [done]
- Exportación GUI: menú File → Export Results (JSON/CSV). [done]

Siguientes features (investigadas y planificadas)
- Escaneo ultra-rápido leyendo la MFT en NTFS (similar a WizTree) y vista por edad/tipo. [partial]
- Programación de informes/escaneos y exportación automática (similar a TreeSize y WinDirStat). [todo]
- Integración S.M.A.R.T. (smartmontools) para estado de disco y alertas tempranas. [todo]
- Línea de tiempo forense: USN Journal + VSS diffs de snapshots (comparación de escaneos). [todo]
- Reglas/YARA: escaneo de patrones (libs/yara ya presente) desde UI con informe de hallazgos. [todo]
- Vista “Top N”: ficheros más grandes, por tipo, por carpeta; acciones masivas. [todo]
- Borrado seguro (shred) – presets NIST (Clear/Purge) con UI en Tools. [done]
- Soporte de red/SSH: escaneo de rutas remotas (inspiración Baobab) y almacenamiento en caché local. [todo]
- Búsqueda rápida global con filtros (tamaño, fecha, tipo) y resaltado en treemap. [todo]
- Internacionalización ES/EN con Qt Linguist y selección de idioma en Settings. [todo]
- Accesibilidad: atajos, foco por teclado, tamaños consistentes, textos alternativos. [todo]
- Dedupe seguro: vista previa con comparación byte a byte opcional y registro de deshacer.
- Tendencias: persistir escaneos y generar series temporales (tamaño total, duplicados, por tipo). [done]
- Filtros UI: por tamaño, fecha, tipo, extensión; búsqueda rápida.
- Internacionalización: soporte i18n para ES/EN (Qt Linguist) y selección de idioma.
- Telemetría opcional local-only: métricas de uso anónimas (opt‑in) guardadas en disco.
