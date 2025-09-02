# DiskSense64 - Suite de Análisis de Disco Multiplataforma

[![Estado de Construcción](https://img.shields.io/badge/construcción-exitosa-brightgreen)](https://github.com/tunombre/disksense64)
[![Licencia](https://img.shields.io/badge/licencia-MIT-blue)](https://opensource.org/licenses/MIT)
[![Plataformas](https://img.shields.io/badge/plataformas-Windows%20%7C%20Linux-lightgrey)](https://github.com/tunombre/disksense64)

Una suite completa y multiplataforma de análisis de disco que proporciona cuatro capacidades principales:
1. **Deduplicación Exacta de Archivos** con enlaces duros
2. **Visualización del Espacio en Disco** con mapas de árbol
3. **Detección y Limpieza de Residuos** de archivos huérfanos
4. **Detección de Duplicados Percibidos** para imágenes y audio

## Características

- 🖥️ **Multiplataforma**: Funciona en Windows (32/64-bit) y Linux
- ⚡ **Alto Rendimiento**: Optimizado con E/S IOCP (Windows) y epoll (Linux)
- 🔒 **Operaciones Seguras**: Modo simulación, instantáneas y acciones reversibles
- 📊 **Análisis Visual**: Mapa de árbol interactivo
- 🧠 **Deduplicación Inteligente**: Filtrado multinivel para eficiencia
- 🎯 **Similitud Percibida**: Encuentra imágenes/audio similares con pHash/min-hash
- 🧹 **Limpieza de Residuos**: Detecta y elimina archivos y directorios huérfanos
- 🛠️ **Sin Dependencias Externas**: Implementación pura en C/C++

## Plataformas Soportadas

| Plataforma | Arquitectura | Estado |
|------------|--------------|--------|
| Windows    | x86 (32-bit) | ✅ Soportado |
| Windows    | x64 (64-bit) | ✅ Soportado |
| Linux      | x86 (32-bit) | ✅ Soportado |
| Linux      | x64 (64-bit) | ✅ Soportado |

## Arquitectura

```
DiskSense64/
├── apps/
│   ├── DiskSense.Cli/      # Interfaz de línea de comandos
│   └── DiskSense.Gui/      # Interfaz gráfica (parcialmente implementada)
├── core/
│   ├── engine/             # Programador de E/S (IOCP/epoll)
│   ├── scan/                # Escáner del sistema de archivos
│   ├── index/               # Índice LSM con mmap
│   ├── model/               # Estructuras de datos
│   ├── ops/                 # Operaciones de archivos
│   ├── gfx/                 # Visualización (solo GUI)
│   ├── rules/               # Heurísticas de limpieza
│   └── platform/           # Abstracciones de plataforma
├── libs/
│   ├── chash/              # Hash criptográfico
│   ├── phash/              # Hash perceptual
│   ├── audfp/              # Huellas de audio
│   └── utils/              # Funciones de utilidad
└── tests/
    ├── unit/               # Pruebas unitarias
    └── perf/               # Benchmarks de rendimiento
```

## Instalación

### Windows (Binarios Precompilados)

1. Descarga la última versión desde [GitHub Releases](https://github.com/tunombre/disksense64/releases)
2. Extrae el archivo
3. Ejecuta `DiskSense.Cli.exe` o `DiskSense.Gui.exe`

### Linux (Binarios Precompilados)

1. Descarga la última versión desde [GitHub Releases](https://github.com/tunombre/disksense64/releases)
2. Extrae el archivo
3. Ejecuta `./DiskSense.Cli` o `./DiskSense.Gui`

### Construir desde el Código Fuente

#### Requisitos Previos

**Windows:**
- Visual Studio 2022 o MinGW-w64
- CMake 3.20+
- Ninja (opcional)

**Linux:**
- GCC 9+ o Clang 10+
- CMake 3.20+
- Ninja (opcional)

#### Construcción

**Windows (Visual Studio):**
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

**Windows (MinGW-w64):**
```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

**Linux:**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Compilación Cruzada (Linux a Windows):**
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw64.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Uso

### Interfaz de Línea de Comandos

```bash
# Escanear un directorio y construir índice
DiskSense.Cli escanear /ruta/al/directorio

# Encontrar duplicados (modo simulación)
DiskSense.Cli deduplicar /ruta/al/directorio

# Encontrar duplicados y crear enlaces duros
DiskSense.Cli deduplicar --accion=enlaceduro /ruta/al/directorio

# Generar visualización de mapa de árbol (solo GUI)
DiskSense.Gui mapa_de_arbol /ruta/al/directorio

# Encontrar archivos similares
DiskSense.Cli similares /ruta/al/directorio

# Limpiar archivos residuales
DiskSense.Cli limpiar /ruta/al/directorio
```

### Interfaz Gráfica

Ejecuta `DiskSense.Gui` para la experiencia gráfica completa con:
- Visualización de mapa de árbol interactivo
- Monitoreo en tiempo real del uso del disco
- Deduplicación con un solo clic
- Detección de similitud visual

## Características Principales

### 1. Deduplicación Exacta de Archivos

Filtrado multinivel para máxima eficiencia:
1. **Filtrado por tamaño** - Eliminación O(1) de archivos de tamaño único
2. **Firmas cabeza/cola** - Huella de 32KB para filtrado rápido de candidatos
3. **Verificación hash completa** - BLAKE3/SHA-256 para verificación final
4. **Fragmentación definida por contenido** - Detecta duplicados desplazados/inserciones

**Acciones:**
- **Simular** - Mostrar ahorros potenciales sin hacer cambios
- **Enlace duro** - Crear enlaces duros del sistema de archivos (ahorro de espacio)
- **Mover a Papelera** - Mover duplicados a la papelera del sistema
- **Eliminar** - Eliminar duplicados permanentemente

### 2. Visualización del Espacio en Disco

Mapa de árbol interactivo con:
- **Algoritmo cuadrificado** - Minimiza las proporciones de los rectángulos
- **Diseño jerárquico** - Visualización de la estructura de directorios
- **Zoom y panorámica** - Navegar por grandes estructuras de directorios
- **Codificación por colores** - Por tipo de archivo, tamaño o propietario
- **Información emergente** - Información detallada del archivo al pasar el mouse

### 3. Detección de Residuos

Limpieza inteligente de:
- **Archivos huérfanos** - Archivos en directorios de programas sin referencias
- **Archivos temporales** - Archivos de caché y temporales antiguos
- **Archivos de registro** - Archivos de registro grandes que pueden ser truncados
- **Directorios duplicados** - Directorios vacíos o redundantes

### 4. Similitud Percibida

Encuentra archivos visual o auditivamente similares:
- **pHash de imagen** - Hash perceptual basado en DCT 32×32
- **Huella digital de audio** - Energía cromática con min-hash
- **Indexación LSH** - Búsqueda aproximada de vecinos cercanos rápida
- **Umbrales ajustables** - Controlar la sensibilidad de similitud

## Rendimiento

| Operación | Rendimiento Objetivo |
|-----------|---------------------|
| Escaneo de archivos | ≥ 200-400k archivos/minuto (SSD NVMe) |
| Hashing (BLAKE3) | ≥ 1.5 GB/s |
| Hashing (SHA-256) | ≥ 0.6-1.0 GB/s |
| Computación pHash | ≥ 20k imágenes/minuto |
| Renderizado de mapa de árbol | ≤ 16ms (60 FPS) |

## Configuración

Crea un archivo `DiskSense64.config` en el directorio de la aplicación:

```ini
[General]
BufferSize=1048576
MaxConcurrentOps=8
ExcludePaths=/tmp;/var/log

[Deduplicacion]
MinFileSize=1024
ComputeFullHash=false
DefaultAction=simulate

[MapaDeArbol]
ColorMode=size
ShowFileNames=true

[Similitud]
ImageThreshold=5
AudioThreshold=10
```

## Seguridad y Privacidad

- 🔐 **Procesamiento Local** - Todo el análisis se realiza en tu máquina
- 🛡️ **Sin Transmisión de Datos** - Ningún archivo sale de tu computadora
- 🧾 **Transparencia** - El modo simulación muestra todas las acciones antes de ejecutarlas
- 🔁 **Reversible** - Todas las operaciones se pueden deshacer
- 📜 **Auditoría** - Registros detallados de todas las operaciones

## Contribuciones

1. Haz fork del repositorio
2. Crea una rama de características (`git checkout -b caracteristica/FuncionIncreible`)
3. Confirma tus cambios (`git commit -m 'Agregar alguna FuncionIncreible'`)
4. Haz push a la rama (`git push origin caracteristica/FuncionIncreible`)
5. Abre una Solicitud de Extracción

### Configuración de Desarrollo

```bash
git clone https://github.com/tunombre/disksense64.git
cd disksense64
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## Pruebas

Ejecutar pruebas unitarias:
```bash
cd build
ctest
```

Ejecutar benchmarks de rendimiento:
```bash
./bin/bench_hash
./bin/bench_io
./bin/bench_phash
```

## Licencia

Este proyecto tiene licencia MIT - consulta el archivo [LICENSE](LICENSE) para más detalles.

## Reconocimientos

- [BLAKE3](https://github.com/BLAKE3-team/BLAKE3) - Función hash criptográfica
- [Direct2D](https://docs.microsoft.com/en-us/windows/win32/direct2d/direct2d-portal) - API de gráficos 2D acelerada por hardware
- [epoll](https://en.wikipedia.org/wiki/Epoll) - Facilidad de notificación de eventos de E/S de Linux
- [Puertos de Finalización de E/S](https://docs.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports) - Modelo escalable de E/S de Windows

## Soporte

Para problemas, solicitudes de características o preguntas:
1. Consulta [GitHub Issues](https://github.com/tunombre/disksense64/issues)
2. Crea un nuevo issue con información detallada
3. Incluye tu plataforma, arquitectura y mensajes de error

---

*DiskSense64 - Tu compañero inteligente de análisis de disco*