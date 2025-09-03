#!/bin/bash
# Script to generate and update translation files for DiskSense64

# Create translations directory
mkdir -p translations

# Generate translation files for supported languages
echo "Generating translation files..."

# English (default)
lupdate -no-obsolete -locations none -ts translations/disksense_en.ts apps/DiskSense.Gui/*.cpp apps/DiskSense.Gui/ui/*.cpp apps/DiskSense.Gui/components/*.cpp core/*/*.cpp platform/*.cpp

# Spanish
lupdate -no-obsolete -locations none -ts translations/disksense_es.ts apps/DiskSense.Gui/*.cpp apps/DiskSense.Gui/ui/*.cpp apps/DiskSense.Gui/components/*.cpp core/*/*.cpp platform/*.cpp

# French
lupdate -no-obsolete -locations none -ts translations/disksense_fr.ts apps/DiskSense.Gui/*.cpp apps/DiskSense.Gui/ui/*.cpp apps/DiskSense.Gui/components/*.cpp core/*/*.cpp platform/*.cpp

# German
lupdate -no-obsolete -locations none -ts translations/disksense_de.ts apps/DiskSense.Gui/*.cpp apps/DiskSense.Gui/ui/*.cpp apps/DiskSense.Gui/components/*.cpp core/*/*.cpp platform/*.cpp

# Compile translation files
echo "Compiling translation files..."

# English
lrelease translations/disksense_en.ts -qm translations/disksense_en.qm

# Spanish
lrelease translations/disksense_es.ts -qm translations/disksense_es.qm

# French
lrelease translations/disksense_fr.ts -qm translations/disksense_fr.qm

# German
lrelease translations/disksense_de.ts -qm translations/disksense_de.qm

echo "Translation files generated and compiled successfully!"