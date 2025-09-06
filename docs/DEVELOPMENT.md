# Desarrollo y Mantenibilidad

Objetivo: mantener un codebase seguro, portable y fácil de evolucionar.

Convenciones
- C++20/C17; evitar dependencias innecesarias en rutas calientes.
- Modo Seguro (no destructivo) por defecto: nunca introducir rutas que omitan `safety::deletion_allowed()`.
- Preferir utilidades de `platform/util` para lógica específica de plataforma (Trash/Recycle, etc.).
- i18n: todas las cadenas visibles se deben marcar para traducción y pasar por el gestor i18n.

Formato y estilo
- `.editorconfig` y `.clang-format` incluidos. Ejecutar un formateo antes de PRs.

Opciones de compilación útiles
- `-DENABLE_WERROR=ON`: warnings como error (CI o desarrollo estricto).
- `-DENABLE_SANITIZERS=ON` (no MSVC): Address/UB sanitizers para detectar errores de memoria.

Pruebas
- Suite mínima estable (CLI) para cambios frecuentes: hashing, utils, safety, trash, dedupe.
- Tests condicionados a plataforma: MFT (sólo Windows), Trash (Linux/Windows) ya contemplados.

Traducciones
- Generar/actualizar con `scripts/generate_translations.sh` (requiere Qt tools: lupdate/lrelease).

Ramas y CI
- CI compila Linux/Windows (Qt 6 vía aqtinstall), genera `.qm` y ejecuta tests mínimos.

