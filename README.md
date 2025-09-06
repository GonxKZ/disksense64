# DiskSense64 – Disk Analysis Suite (Safe by Default)

DiskSense64 es una suite de análisis de disco multiplataforma (C/C++/Qt) con enfoque en seguridad de datos, rendimiento y visualización avanzada.

Principios
- Modo Seguro por defecto: la aplicación NO borra archivos del usuario salvo que lo autorices explícitamente.
- Experiencia guiada: asistente al inicio, tour guiado y UI accesible.
- Rendimiento: escaneo ultrarrápido en NTFS (Windows) vía MFT/USN.

Características clave
- Visualización: Treemap 2D, Sunburst (radial), gráficos por tipo/edad, pestaña de Tendencias (snapshots).
- Duplicados: detección por tamaño, firma head/tail y hash completo; acciones seguras (hardlinks/mover a Papelera).
- Residuo: detección de .tmp/.log y directorios vacíos; cuarentena/Trash con deshacer.
- Seguridad: YARA desde la UI; borrado seguro (presets NIST: Clear/Purge) opcional y bloqueado si está el Modo Seguro.
- Salud de discos (SMART), Top N, búsqueda y escaneos remotos (vía ssh).
- i18n: ES/EN, con posibilidad de ampliar.

Requisitos
- CMake 3.20+
- Compilador C++20
- Qt 6.6+ (Widgets, Svg, Tools, Declarative, ShaderTools, ImageFormats)
- Linux: libarchive, X11/mesa básicos; Windows: SDK/VS2022

Compilación rápida (CLI + tests mínimos)
```bash
cmake -S . -B build-cli -G Ninja -DENABLE_GUI=OFF -DBUILD_CLI_ONLY=ON -DENABLE_TESTING=ON -DMINIMAL_UNIT_TESTS=ON
cmake --build build-cli --parallel
ctest --test-dir build-cli --output-on-failure
```

Compilación GUI (Linux, con Qt instalado vía aqtinstall)
```bash
# Instalar Qt 6.6.2 (una sola vez)
python3 -m pip install --user aqtinstall
python3 -m aqt install-qt --outputdir $HOME/Qt linux desktop 6.6.2 gcc_64 -m qtbase qtsvg qttools qtdeclarative qtshadertools qtimageformats
export PATH="$HOME/Qt/6.6.2/gcc_64/bin:$PATH"

# Generar traducciones (.qm)
bash scripts/generate_translations.sh

# Configurar, compilar y probar
cmake -S . -B build -G Ninja -DENABLE_GUI=ON -DUSE_QT_GUI=ON -DENABLE_TESTING=ON -DMINIMAL_UNIT_TESTS=ON -DCMAKE_PREFIX_PATH=$HOME/Qt/6.6.2/gcc_64
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Uso rápido (GUI)
- Onboarding: al iniciar, elige carpeta y pulsa “Comenzar”.
- Deduplicación/Residuo: por defecto acciones seguras (Trash/Quarantine). “Delete (Permanent)” se oculta/bloquea en Modo Seguro.
- Visualization: activa “Show Sunburst” y usa “Refresh Trends” tras escaneos.
- Tools: “Restore from Trash…” (Linux y Windows), “Secure Delete…” (NIST Clear/Purge; bloqueado por Safety si no se habilita).
- Settings: cambia idioma (ES/EN) y preferencias.

### Command Line Tools

Standalone tools are available in the `tools/` directory (when implemented).

Rendimiento (Windows)
- “Use MFT Reader” activa enumeración NTFS a través de DeviceIoControl (FSCTL_ENUM_USN_DATA) y reconstrucción de rutas por FRN/Parent FRN.
- Si no hay permisos o no es NTFS, se vuelve al escaneo recursivo.

Test mínimos incluidos
- Seguridad y Trash: mueve/lista/restaura Trash (Linux XDG y Windows Recycle Bin).
- Safety/Dedupe: simula/no destructivo bajo Modo Seguro.
- Hashing/Utils: básicos.

Estructura (resumen)
- apps/DiskSense.Gui: aplicación Qt (UI, componentes y diálogos)
- core/…: escaneo, operaciones (cleanup/dedupe/secure_delete), índice LSM, etc.
- libs/…: bibliotecas funcionales (hash, metadata, compression, etc.)
- platform/util: utilidades de plataforma (Trash/Recycle, i18n manager en platform/)
- scripts/: utilidades (traducciones, build)
- tests/: pruebas unitarias (mínimas y modulares)

Licencia
MIT. Consulta LICENSE.

Contribuir
Ver docs/CONTRIBUTING.md. Se agradecen PRs con tests y sin romper Modo Seguro por defecto.
