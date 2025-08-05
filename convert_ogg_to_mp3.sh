#!/bin/bash

# Script to convert all .ogg files in src/assets to .mp3 format
# Using high quality settings for game audio

cd "/Users/aimac/Development/FloppyTurd/src/assets"

echo "Converting .ogg files to .mp3 in src/assets directory..."
echo "Found $(find . -name "*.ogg" -type f | wc -l) files to convert"

# Counter for progress
count=0
total=$(find . -name "*.ogg" -type f | wc -l)

# Find all .ogg files and convert them
find . -name "*.ogg" -type f | while read -r file; do
    count=$((count + 1))
    
    # Get the directory and filename without extension
    dir=$(dirname "$file")
    filename=$(basename "$file" .ogg)
    
    # Output path with .mp3 extension
    output_file="${dir}/${filename}.mp3"
    
    echo "[$count/$total] Converting: $file -> $output_file"
    
    # Convert with high quality settings
    # -acodec libmp3lame: Use LAME MP3 encoder
    # -ab 192k: 192 kbps bitrate (good quality for game audio)
    # -ar 44100: 44.1 kHz sample rate (standard)
    # -ac 2: Stereo output
    ffmpeg -i "$file" -acodec libmp3lame -ab 192k -ar 44100 -ac 2 "$output_file" -y -loglevel error
    
    if [ $? -eq 0 ]; then
        echo "  ✓ Successfully converted"
        # Remove the original .ogg file after successful conversion
        rm "$file"
        echo "  ✓ Removed original .ogg file"
    else
        echo "  ✗ Error converting $file"
    fi
    
    echo ""
done

echo "Conversion complete!"
echo "Verifying results..."

# Count remaining .ogg files (should be 0)
remaining_ogg=$(find . -name "*.ogg" -type f | wc -l)
echo "Remaining .ogg files: $remaining_ogg"

# Count new .mp3 files
mp3_count=$(find . -name "*.mp3" -type f | wc -l)
echo "Total .mp3 files: $mp3_count"

if [ $remaining_ogg -eq 0 ]; then
    echo "✓ All .ogg files successfully converted to .mp3!"
else
    echo "⚠ Warning: $remaining_ogg .ogg files remain unconverted"
fi
