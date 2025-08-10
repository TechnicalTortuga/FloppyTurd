#!/bin/bash
# Script to generate an asset catalog from FloppyTurd resources
# This script will process all resources and create appropriate asset catalog entries

set -e  # Exit on error

# Define paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."
RESOURCES_DIR="$PROJECT_ROOT/src/assets"
ASSET_CATALOG_DIR="$PROJECT_ROOT/src/Assets.xcassets"

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

  # Process each audio file (.mp3 and .wav only, skip .ogg for iOS compatibility)
  find "$source_dir" -type f \( -name "*.mp3" -o -name "*.wav" \) | while read audio_file; do
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

# Process all image files recursively, preserving directory structure
process_all_images() {
  local base_dir="$RESOURCES_DIR/graphics"
  if [ ! -d "$base_dir" ]; then
    return
  fi
  
  echo "Processing all graphics directories and subdirectories..."
  find "$base_dir" -type f \( -name "*.png" -o -name "*.jpg" -o -name "*.jpeg" \) | while read img_file; do
    # Get relative path from graphics directory
    rel_path="${img_file#$base_dir/}"
    # Get directory path and filename
    dir_path=$(dirname "$rel_path")
    filename=$(basename "$img_file")
    name="${filename%.*}"
    
    # Create full category path preserving directory structure
    if [ "$dir_path" = "." ]; then
      category_path="graphics"
    else
      category_path="graphics/$dir_path"
    fi
    
    # Create asset catalog directory structure
    mkdir -p "$ASSET_CATALOG_DIR/$category_path/$name.imageset"
    
    # Create Contents.json
    cat > "$ASSET_CATALOG_DIR/$category_path/$name.imageset/Contents.json" << EOF
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
    cp "$img_file" "$ASSET_CATALOG_DIR/$category_path/$name.imageset/"
    echo "Processed graphics: $category_path/$name"
  done
}

# Process all audio files recursively, preserving directory structure
process_all_audio() {
  local base_dir="$RESOURCES_DIR/audio"
  if [ ! -d "$base_dir" ]; then
    return
  fi
  
  echo "Processing all audio directories and subdirectories..."
  # Process .mp3 files first, then .wav, skip .ogg for iOS compatibility
  find "$base_dir" -type f \( -name "*.mp3" -o -name "*.wav" \) | while read audio_file; do
    # Get relative path from audio directory
    rel_path="${audio_file#$base_dir/}"
    # Get directory path and filename
    dir_path=$(dirname "$rel_path")
    filename=$(basename "$audio_file")
    name="${filename%.*}"
    
    # Create full category path preserving directory structure
    if [ "$dir_path" = "." ]; then
      category_path="audio"
    else
      category_path="audio/$dir_path"
    fi
    
    # Create asset catalog directory structure
    mkdir -p "$ASSET_CATALOG_DIR/$category_path/$name.dataset"
    
    # Create Contents.json
    cat > "$ASSET_CATALOG_DIR/$category_path/$name.dataset/Contents.json" << EOF
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
    cp "$audio_file" "$ASSET_CATALOG_DIR/$category_path/$name.dataset/"
    echo "Processed audio: $category_path/$name"
  done
}

# Process all font files recursively, preserving directory structure
process_all_fonts() {
  local base_dir="$RESOURCES_DIR/fonts"
  if [ ! -d "$base_dir" ]; then
    return
  fi
  
  echo "Processing all font directories and subdirectories..."
  find "$base_dir" -type f \( -name "*.fnt" -o -name "*.ttf" -o -name "*.otf" \) | while read font_file; do
    # Get relative path from fonts directory
    rel_path="${font_file#$base_dir/}"
    # Get directory path and filename
    dir_path=$(dirname "$rel_path")
    filename=$(basename "$font_file")
    
    # Create full category path preserving directory structure
    if [ "$dir_path" = "." ]; then
      category_path="fonts"
    else
      category_path="fonts/$dir_path"
    fi
    
    # Create asset catalog directory structure
    mkdir -p "$ASSET_CATALOG_DIR/$category_path"
    
    # Copy font file preserving directory structure
    cp "$font_file" "$ASSET_CATALOG_DIR/$category_path/"
    echo "Processed font: $category_path/$filename"
  done
}

###############################################################################
# MSDF font assets (PNG atlas + CSV metrics) → add to asset catalog
# - PNG goes into an imageset so UIImage(named:) can load it
# - CSV goes into a dataset so NSDataAsset(name:) can load it
###############################################################################
process_msdf_assets() {
  local base_dir="$RESOURCES_DIR/fonts/msdf"
  if [ ! -d "$base_dir" ]; then
    return
  fi

  echo "Processing MSDF font assets (PNG + CSV) ..."

  # Iterate all PNG atlases
  find "$base_dir" -type f -name "*.png" | while read png_file; do
    filename=$(basename "$png_file")               # e.g., Whacky_Joe_msdf.png
    name_no_ext="${filename%.*}"                   # e.g., Whacky_Joe_msdf

    # We expose the atlas via a distinct imageset name to avoid name collisions
    imageset_name="${name_no_ext}_atlas"

    # Preserve relative directory (for organization only)
    rel_path="${png_file#$base_dir/}"
    rel_dir=$(dirname "$rel_path")                 # e.g., Whacky_Joe
    if [ "$rel_dir" = "." ]; then
      category_path="fonts/msdf"
    else
      category_path="fonts/msdf/$rel_dir"
    fi

    mkdir -p "$ASSET_CATALOG_DIR/$category_path/$imageset_name.imageset"
    cat > "$ASSET_CATALOG_DIR/$category_path/$imageset_name.imageset/Contents.json" << EOF
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
    cp "$png_file" "$ASSET_CATALOG_DIR/$category_path/$imageset_name.imageset/"
    echo "Processed MSDF atlas: $category_path/$imageset_name"
  done

  # Iterate all CSV metric files
  find "$base_dir" -type f -name "*.csv" | while read csv_file; do
    filename=$(basename "$csv_file")               # e.g., Whacky_Joe_msdf.csv
    name_no_ext="${filename%.*}"                   # e.g., Whacky_Joe_msdf

    # Use a distinct dataset name for metrics
    dataset_name="${name_no_ext}_metrics"

    # Preserve relative directory (for organization only)
    rel_path="${csv_file#$base_dir/}"
    rel_dir=$(dirname "$rel_path")
    if [ "$rel_dir" = "." ]; then
      category_path="fonts/msdf"
    else
      category_path="fonts/msdf/$rel_dir"
    fi

    mkdir -p "$ASSET_CATALOG_DIR/$category_path/$dataset_name.dataset"
    cat > "$ASSET_CATALOG_DIR/$category_path/$dataset_name.dataset/Contents.json" << EOF
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
    cp "$csv_file" "$ASSET_CATALOG_DIR/$category_path/$dataset_name.dataset/"
    echo "Processed MSDF metrics: $category_path/$dataset_name"
  done
}

process_all_images
process_all_audio  
process_all_fonts
process_msdf_assets

echo "Asset catalog generation complete at: $ASSET_CATALOG_DIR"
