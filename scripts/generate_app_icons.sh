#!/bin/bash
# generate_app_icons.sh
# Generates all required iOS app icon sizes from master icon file
# Usage: ./generate_app_icons.sh <source_icon.png>

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Source icon (should be 1024x1024 or larger)
SOURCE_ICON="$1"

if [ -z "$SOURCE_ICON" ]; then
    echo "Usage: $0 <source_icon.png>"
    echo "Example: $0 src/assets/FTIconIos.png"
    exit 1
fi

if [ ! -f "$SOURCE_ICON" ]; then
    echo "Error: Source icon not found: $SOURCE_ICON"
    exit 1
fi

# Output directory
ICON_SET_DIR="$PROJECT_ROOT/src/Assets.xcassets/AppIcon.appiconset"

echo "📱 Generating iOS app icons from: $SOURCE_ICON"
echo "Output directory: $ICON_SET_DIR"

# Create directory if it doesn't exist
mkdir -p "$ICON_SET_DIR"

# Check if we have sips (macOS image tool)
if ! command -v sips &> /dev/null; then
    echo "Error: 'sips' command not found. This script requires macOS."
    exit 1
fi

# Function to resize icon
resize_icon() {
    local size=$1
    local scale=$2
    local filename=$3
    local pixel_size=$((size * scale))
    
    echo "  Generating ${filename} (${pixel_size}x${pixel_size})..."
    sips -z "$pixel_size" "$pixel_size" "$SOURCE_ICON" --out "$ICON_SET_DIR/$filename" > /dev/null 2>&1
}

# Generate all required sizes
echo "Generating icons..."

# iPhone notification icons (20pt)
resize_icon 20 2 "Icon-App-20x20@2x.png"
resize_icon 20 3 "Icon-App-20x20@3x.png"

# iPhone settings icons (29pt)
resize_icon 29 2 "Icon-App-29x29@2x.png"
resize_icon 29 3 "Icon-App-29x29@3x.png"

# iPhone Spotlight icons (40pt)
resize_icon 40 2 "Icon-App-40x40@2x.png"
resize_icon 40 3 "Icon-App-40x40@3x.png"

# iPhone app icons (60pt)
resize_icon 60 2 "Icon-App-60x60@2x.png"
resize_icon 60 3 "Icon-App-60x60@3x.png"

# App Store icon (1024x1024 @1x)
echo "  Generating Icon-App-1024x1024@1x.png (1024x1024)..."
sips -z 1024 1024 "$SOURCE_ICON" --out "$ICON_SET_DIR/Icon-App-1024x1024@1x.png" > /dev/null 2>&1

# Create Contents.json
echo "Creating Contents.json..."
cat > "$ICON_SET_DIR/Contents.json" << 'EOF'
{
  "images" : [
    {
      "filename" : "Icon-App-20x20@2x.png",
      "idiom" : "iphone",
      "scale" : "2x",
      "size" : "20x20"
    },
    {
      "filename" : "Icon-App-20x20@3x.png",
      "idiom" : "iphone",
      "scale" : "3x",
      "size" : "20x20"
    },
    {
      "filename" : "Icon-App-29x29@2x.png",
      "idiom" : "iphone",
      "scale" : "2x",
      "size" : "29x29"
    },
    {
      "filename" : "Icon-App-29x29@3x.png",
      "idiom" : "iphone",
      "scale" : "3x",
      "size" : "29x29"
    },
    {
      "filename" : "Icon-App-40x40@2x.png",
      "idiom" : "iphone",
      "scale" : "2x",
      "size" : "40x40"
    },
    {
      "filename" : "Icon-App-40x40@3x.png",
      "idiom" : "iphone",
      "scale" : "3x",
      "size" : "40x40"
    },
    {
      "filename" : "Icon-App-60x60@2x.png",
      "idiom" : "iphone",
      "scale" : "2x",
      "size" : "60x60"
    },
    {
      "filename" : "Icon-App-60x60@3x.png",
      "idiom" : "iphone",
      "scale" : "3x",
      "size" : "60x60"
    },
    {
      "filename" : "Icon-App-1024x1024@1x.png",
      "idiom" : "ios-marketing",
      "scale" : "1x",
      "size" : "1024x1024"
    }
  ],
  "info" : {
    "author" : "xcode",
    "version" : 1
  }
}
EOF

echo "✅ App icons generated successfully!"
echo ""
echo "Generated icons in: $ICON_SET_DIR"
echo ""
echo "Next steps:"
echo "1. Clean and rebuild your project"
echo "2. Icons will be automatically included via CMake"
echo ""
