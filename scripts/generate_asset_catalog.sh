#!/bin/bash
# Script to generate an asset catalog from FloppyTurd resources
# This script will process all resources and create appropriate asset catalog entries

set -e  # Exit on error

# Define paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."
RESOURCES_DIR="$PROJECT_ROOT/FloppyTurd/resources"
ASSET_CATALOG_DIR="$PROJECT_ROOT/FloppyTurd/Assets.xcassets"

# Create asset catalog base structure if it doesn't exist
mkdir -p "$ASSET_CATALOG_DIR"

# Create Contents.json file for root asset catalog if it doesn't exist
if [ ! -f "$ASSET_CATALOG_DIR/Contents.json" ]; then
  cat > "$ASSET_CATALOG_DIR/Contents.json" << EOF
{
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF
fi

# Function to process image resources
process_images() {
  local source_dir="$RESOURCES_DIR/$1"
  local category="$1"
  
  # Skip if source directory doesn't exist
  if [ ! -d "$source_dir" ]; then
    return
  fi
  
  # Create category directory in asset catalog
  mkdir -p "$ASSET_CATALOG_DIR/$category"
  
  # Create category Contents.json
  cat > "$ASSET_CATALOG_DIR/$category/Contents.json" << EOF
{
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF

  # Process each image file
  find "$source_dir" -type f \( -name "*.png" -o -name "*.jpg" -o -name "*.jpeg" \) | while read img_file; do
    filename=$(basename "$img_file")
    name="${filename%.*}"
    
    # Create imageset directory
    mkdir -p "$ASSET_CATALOG_DIR/$category/$name.imageset"
    
    # Create Contents.json for image
    cat > "$ASSET_CATALOG_DIR/$category/$name.imageset/Contents.json" << EOF
{
  "images" : [
    {
      "filename" : "$filename",
      "idiom" : "universal",
      "scale" : "1x"
    }
  ],
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF
    
    # Copy image file
    cp "$img_file" "$ASSET_CATALOG_DIR/$category/$name.imageset/"
    echo "Processed image: $category/$name"
  done
}

# Function to process audio resources
process_audio() {
  local source_dir="$RESOURCES_DIR/$1"
  local category="$1"
  
  # Skip if source directory doesn't exist
  if [ ! -d "$source_dir" ]; then
    return
  fi
  
  # Create category directory in asset catalog
  mkdir -p "$ASSET_CATALOG_DIR/$category"
  
  # Create category Contents.json
  cat > "$ASSET_CATALOG_DIR/$category/Contents.json" << EOF
{
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF

  # Process each audio file
  find "$source_dir" -type f \( -name "*.mp3" -o -name "*.ogg" -o -name "*.wav" \) | while read audio_file; do
    filename=$(basename "$audio_file")
    name="${filename%.*}"
    
    # Create dataset directory
    mkdir -p "$ASSET_CATALOG_DIR/$category/$name.dataset"
    
    # Create Contents.json for audio
    cat > "$ASSET_CATALOG_DIR/$category/$name.dataset/Contents.json" << EOF
{
  "data" : [
    {
      "filename" : "$filename",
      "idiom" : "universal"
    }
  ],
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF
    
    # Copy audio file
    cp "$audio_file" "$ASSET_CATALOG_DIR/$category/$name.dataset/"
    echo "Processed audio: $category/$name"
  done
}

# Process all resource directories
echo "Generating asset catalog from resources..."

# Process image categories
process_images "enemies"
process_images "environment"
process_images "hats"
process_images "mainmenu"
process_images "objects"
process_images "turd"
process_images "ui"
process_images "vfx"

# Process audio categories
process_audio "music"
process_audio "sounds"

# Process font resources (copy as-is)
process_fonts() {
  local source_dir="$RESOURCES_DIR/fonts"
  
  # Skip if source directory doesn't exist
  if [ ! -d "$source_dir" ]; then
    return
  fi
  
  # Create fonts directory in asset catalog
  mkdir -p "$ASSET_CATALOG_DIR/fonts"
  
  # Create category Contents.json
  cat > "$ASSET_CATALOG_DIR/fonts/Contents.json" << EOF
{
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF

  # Copy font files directly
  cp "$source_dir"/* "$ASSET_CATALOG_DIR/fonts/" 2>/dev/null || true
  echo "Processed fonts"
}

process_fonts

echo "Asset catalog generation complete at: $ASSET_CATALOG_DIR"
