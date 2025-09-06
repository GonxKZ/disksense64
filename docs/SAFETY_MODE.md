Safety Mode (Non‑Destructive by Default)

Resumen
- DiskSense64 funciona en modo seguro por defecto: no borra archivos del usuario.
- Todas las operaciones potencialmente destructivas se bloquean o se redirigen a cuarentena.

Detalles técnicos
- El módulo core/safety verifica la variable de entorno `DISKSENSE_ALLOW_DELETE`.
- Si NO está establecida a `1`, `y` o `true`, las funciones de borrado quedan deshabilitadas.
- cleanup_apply mueve a cuarentena y evita eliminar directorios cuando el modo seguro está activo.
- deduplicate fuerza simulación cuando el modo seguro bloquea borrado.
- secure_delete_file devuelve error con el mensaje "Deletion disabled by Safety Mode".

Uso para desarrollo
- Para pruebas controladas en un entorno de desarrollo/CI:
  - Establecer `DISKSENSE_ALLOW_DELETE=1` antes de ejecutar la app.
  - La UI mostrará opciones adicionales (p.ej., "Delete"), pero úsese con extrema precaución.

UI/UX
- La ventana principal muestra un banner de “Modo seguro activo”.
- Las acciones de borrado permanente se ocultan/inhabilitan cuando el modo seguro está activo.

Buenas prácticas
- Preferir siempre cuarentena y permitir deshacer.
- Exportar reportes antes de aplicar cambios.

