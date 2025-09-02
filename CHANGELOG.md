# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project structure and core modules
- Cross-platform I/O optimization framework
- BLAKE3 and SHA-256 hashing implementations
- Content-defined chunking for large files
- Perceptual hashing for images and audio
- LSM-based index with memory mapping
- Deduplication engine with hardlink support
- Residue detection and cleanup utilities
- Treemap visualization with Direct2D
- CLI and GUI applications
- Comprehensive documentation and build instructions
- Performance monitoring with ETW tracing
- Configuration system with INI files
- Cross-platform build system with CMake
- Unit tests and performance benchmarks

### Changed
- Improved I/O scheduling with IOCP/epoll
- Optimized memory usage patterns
- Enhanced error handling and recovery
- Better support for large file processing
- Improved UI responsiveness and animations
- Enhanced security with proper privilege handling
- Better cross-platform compatibility
- Updated documentation and user guides

### Fixed
- Memory leaks in file processing
- Race conditions in multithreaded operations
- Performance bottlenecks in hash computation
- UI rendering issues on high DPI displays
- File permission handling on restricted files
- Unicode path handling issues
- Resource cleanup on application exit

## [0.1.0] - 2023-09-02

### Added
- Initial release with core functionality
- File scanning and indexing capabilities
- Basic deduplication with simulation mode
- Simple CLI interface
- Foundation for GUI development
- Cross-platform build system
- Documentation and contribution guidelines

### Changed
- None

### Deprecated
- None

### Removed
- None

### Fixed
- None

### Security
- None

---

## Release Cycle

DiskSense64 follows a time-based release cycle:

- **Major releases**: Every 6 months (March and September)
- **Minor releases**: Every month
- **Patch releases**: As needed for critical bugs

Major releases may include breaking changes, while minor and patch releases maintain backward compatibility.

## Versioning Scheme

DiskSense64 uses Semantic Versioning 2.0.0:

- **MAJOR** version: Breaking changes to the API or file formats
- **MINOR** version: Backward-compatible new features
- **PATCH** version: Backward-compatible bug fixes

Example:
- 1.0.0 → 2.0.0: Major version with breaking changes
- 1.0.0 → 1.1.0: Minor version with new features
- 1.0.0 → 1.0.1: Patch version with bug fixes

## Upgrade Notes

### Upgrading to 1.0.0

This is the first stable release of DiskSense64. There are no previous versions to upgrade from.

### Compatibility

DiskSense64 maintains compatibility between:
- **PATCH versions**: Full API and file format compatibility
- **MINOR versions**: Backward compatibility guaranteed
- **MAJOR versions**: May include breaking changes

Always backup your index files before upgrading to a new MAJOR version.

## Reporting Issues

To report an issue with a specific version:

1. Check the [GitHub Issues](https://github.com/yourusername/disksense64/issues) to see if it's already reported
2. Include the version number in your issue report
3. Provide steps to reproduce the issue
4. Include your operating system and hardware specifications

Example issue title:
```
[v1.2.3] Application crashes when scanning large directories on Windows 11
```

## Security Advisories

Security vulnerabilities are disclosed in this changelog and on the [GitHub Security Advisory page](https://github.com/yourusername/disksense64/security/advisories).

### Reporting Security Issues

To report a security vulnerability, please contact security@disksense64.org with:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Your contact information (optional)

We aim to respond to security reports within 72 hours.

## Credits

Thanks to all the contributors who have helped with this release:

- Gonzalo (Lead Developer)
- [Full list in CONTRIBUTORS.md]

Special thanks to the open source community and the beta testers who provided valuable feedback.