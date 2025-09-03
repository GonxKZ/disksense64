# DiskSense64 Operations Guide

## Getting Started

### Installation

1. Download the latest release or build from source
2. Extract to a directory of your choice
3. Run `DiskSense.Gui.exe` for the graphical interface or use `DiskSense.Cli.exe` for command-line operations

### System Requirements

- Windows 11 x64
- Minimum 4GB RAM (8GB recommended)
- NTFS file system
- Administrator privileges for advanced features (MFT scanning, VSS)

## Core Functions

### 1. Disk Scanning

The first step in using DiskSense64 is to scan your disk to build an index of all files.

**CLI Usage:**
```bash
DiskSense.Cli.exe scan C:\path\to\scan
```

**GUI Usage:**
1. Open DiskSense64
2. Click "Scan" tab
3. Select drive or directory
4. Click "Start Scan"

**Options:**
- **Fast Scan**: Uses Win32 APIs (default)
- **Deep Scan**: Uses MFT reader (requires admin)
- **Hash Signatures**: Computes head/tail signatures for deduplication

### 2. Deduplication

Find and eliminate duplicate files to save disk space.

**CLI Usage:**
```bash
# Simulate deduplication (safe)
DiskSense.Cli.exe dedupe C:\path\to\scan

# Actual deduplication (requires confirmation)
DiskSense.Cli.exe dedupe --action=hardlink C:\path\to\scan
```

**GUI Usage:**
1. Open DiskSense64
2. Click "Deduplication" tab
3. Select drive or directory
4. Click "Find Duplicates"
5. Review results and select action:
   - **Simulate**: Show potential savings without making changes
   - **Hardlink**: Create NTFS hardlinks (same volume only)
   - **Move to Recycle Bin**: Move duplicates to recycle bin
   - **Delete**: Permanently delete duplicates

**Safety Features:**
- All operations can be undone
- VSS snapshots for system restore
- Simulation mode to preview changes
- Confirmation prompts for destructive actions

### 3. Treemap Visualization

Visualize disk space usage with interactive treemaps.

**GUI Usage:**
1. Open DiskSense64
2. Click "Treemap" tab
3. Select drive or directory
4. Click "Generate Treemap"

**Navigation:**
- **Zoom**: Mouse wheel or +/- keys
- **Pan**: Click and drag
- **Select**: Click on rectangles to highlight
- **Details**: Hover for file information

**Color Coding:**
- **By Size**: Blue (small) to Red (large)
- **By Type**: Different colors for different file extensions
- **By Age**: Different colors for file modification times

### 4. Residue Detection

Find and clean orphaned files, broken registry entries, and other disk residue.

**CLI Usage:**
```bash
DiskSense.Cli.exe cleanup C:\path\to\scan
```

**GUI Usage:**
1. Open DiskSense64
2. Click "Cleanup" tab
3. Select drive or directory
4. Click "Scan for Residue"
5. Review findings and select items to clean

**Detection Methods:**
- **Orphaned Files**: Files in program directories with no references
- **Broken Registry**: COM entries pointing to missing files
- **Temporary Files**: Old cache and temp files
- **Log Files**: Large log files that can be truncated

**Confidence Levels:**
- **High**: Safe to remove
- **Medium**: Review recommended
- **Low**: Do not remove

### 5. Similarity Detection

Find visually or audibly similar files that may not be exact duplicates.

**CLI Usage:**
```bash
DiskSense.Cli.exe similar C:\path\to\scan
```

**GUI Usage:**
1. Open DiskSense64
2. Click "Similarity" tab
3. Select drive or directory
4. Click "Find Similar Files"
5. Adjust similarity threshold
6. Review and manage groups

**Supported Formats:**
- **Images**: JPEG, PNG, BMP, TIFF
- **Audio**: MP3, WAV, FLAC, OGG

## Performance Optimization

### For Best Results

1. **Close unnecessary applications** before scanning
2. **Disable real-time antivirus scanning** of the target directory during operations
3. **Use SSD storage** for index files
4. **Increase buffer sizes** in settings for large datasets

### Memory Management

- DiskSense64 automatically adjusts memory usage based on system resources
- For systems with limited RAM, reduce concurrent operations in settings
- Index files are memory-mapped for efficient access

## Troubleshooting

### Common Issues

**"Access Denied" errors:**
- Run as Administrator for system directories
- Check file permissions
- Ensure no other processes are locking files

**Slow scanning:**
- Use Fast Scan mode for large directories
- Ensure sufficient free disk space for index files
- Close other disk-intensive applications

**Missing files in results:**
- Verify the selected directory includes the expected files
- Check exclude filters in settings
- Ensure sufficient privileges to access all files

### Support

For issues not covered in this guide, please:
1. Check the [GitHub issues](https://github.com/yourusername/disksense64/issues)
2. Provide detailed information about your system and the problem
3. Include relevant log files from the Logs directory

## Advanced Configuration

### Configuration File

Settings are stored in `DiskSense64.config` in the application directory:

```ini
[General]
BufferSize=1048576
MaxConcurrentOps=8
ExcludePaths=C:\Windows;C:\Program Files

[Deduplication]
MinFileSize=1024
ComputeFullHash=false
DefaultAction=simulate

[Treemap]
ColorMode=size
ShowFileNames=true

[Similarity]
ImageThreshold=5
AudioThreshold=10
```

### Command Line Options

```bash
# Show help
DiskSense.Cli.exe --help

# Set custom configuration file
DiskSense.Cli.exe --config=custom.config scan C:\path

# Verbose output
DiskSense.Cli.exe --verbose scan C:\path

# Specify output directory for index
DiskSense.Cli.exe --index-dir=D:\index scan C:\path
```

## Privacy and Security

### Data Handling

- All scanning and analysis happens locally
- No data is transmitted to external servers
- Index files are stored locally and can be deleted at any time

### Permissions

- Standard operations require no special permissions
- Advanced features (MFT scanning, VSS) require Administrator privileges
- File access respects existing NTFS permissions

### Undo Operations

- All destructive operations create undo information
- VSS snapshots can restore entire system state
- Recycle bin operations can be undone through standard Windows tools