#!/usr/bin/env python3
"""
Resize iOS Simulator screenshots to App Store Connect required dimensions.
Handles multiple device sizes and maintains aspect ratio.
"""

import os
import sys
from PIL import Image
from pathlib import Path

# App Store Connect required screenshot sizes (portrait orientation)
REQUIRED_SIZES = {
    "6.7inch": (1290, 2796),  # iPhone 14 Pro Max, 15 Pro Max, 16 Pro Max
    "6.5inch": (1242, 2688),  # iPhone 11 Pro Max, XS Max
    "5.5inch": (1242, 2208),  # iPhone 8 Plus (optional but recommended)
}

def resize_screenshot(input_path, output_dir, target_sizes=None):
    """
    Resize a screenshot to App Store Connect required dimensions.
    
    Args:
        input_path: Path to input image
        output_dir: Directory to save resized images
        target_sizes: Dict of size names to (width, height) tuples. If None, uses all sizes.
    """
    if target_sizes is None:
        target_sizes = REQUIRED_SIZES
    
    try:
        # Open the image
        img = Image.open(input_path)
        print(f"📸 Processing: {os.path.basename(input_path)}")
        print(f"   Original size: {img.size[0]}x{img.size[1]}")
        
        # Get base filename without extension
        base_name = Path(input_path).stem
        
        # Create output directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)
        
        # Resize to each required size
        for size_name, (target_width, target_height) in target_sizes.items():
            # Calculate scaling to fit within target dimensions while maintaining aspect ratio
            img_aspect = img.size[0] / img.size[1]
            target_aspect = target_width / target_height
            
            if img_aspect > target_aspect:
                # Image is wider than target - fit to width
                new_width = target_width
                new_height = int(target_width / img_aspect)
            else:
                # Image is taller than target - fit to height
                new_height = target_height
                new_width = int(target_height * img_aspect)
            
            # Resize image
            resized = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
            
            # Create canvas with target size (centered, black bars if needed)
            canvas = Image.new('RGB', (target_width, target_height), (0, 0, 0))
            
            # Calculate position to center the image
            x_offset = (target_width - new_width) // 2
            y_offset = (target_height - new_height) // 2
            
            # Paste resized image onto canvas
            canvas.paste(resized, (x_offset, y_offset))
            
            # Save with descriptive filename
            output_filename = f"{base_name}_{size_name}_{target_width}x{target_height}.png"
            output_path = os.path.join(output_dir, output_filename)
            canvas.save(output_path, 'PNG', optimize=True)
            
            print(f"   ✅ Saved {size_name}: {output_filename}")
        
        print(f"   🎉 Done!\n")
        return True
        
    except Exception as e:
        print(f"   ❌ Error processing {input_path}: {str(e)}\n")
        return False

def main():
    """Main function to process screenshots from Desktop."""
    
    print("=" * 60)
    print("🎮 Floppy Turd - App Store Screenshot Resizer")
    print("=" * 60)
    print()
    
    # Get Desktop path
    desktop = os.path.expanduser("~/Desktop")
    
    # Create output directory on Desktop
    output_dir = os.path.join(desktop, "FloppyTurd_AppStore_Screenshots")
    
    # If command line arguments provided, use those files
    if len(sys.argv) > 1:
        input_files = sys.argv[1:]
    else:
        # Look for recent screenshots on Desktop
        print("🔍 Looking for screenshots on Desktop...")
        screenshots = []
        for file in os.listdir(desktop):
            if file.endswith('.png') and ('Screenshot' in file or 'Simulator' in file):
                file_path = os.path.join(desktop, file)
                screenshots.append(file_path)
        
        # Sort by modification time (newest first)
        screenshots.sort(key=lambda x: os.path.getmtime(x), reverse=True)
        
        if not screenshots:
            print("❌ No screenshots found on Desktop!")
            print("   Take screenshots in Simulator (Cmd+S) and try again.")
            return
        
        # Show available screenshots
        print(f"\n📋 Found {len(screenshots)} screenshot(s):\n")
        for i, path in enumerate(screenshots[:10], 1):  # Show max 10
            filename = os.path.basename(path)
            size = os.path.getsize(path) / 1024 / 1024  # MB
            print(f"   {i}. {filename} ({size:.1f} MB)")
        
        print("\n" + "=" * 60)
        print("Choose an option:")
        print("  1) Process ALL screenshots")
        print("  2) Process specific screenshot(s) by number (e.g., '1,3,5')")
        print("  3) Exit")
        print("=" * 60)
        
        choice = input("\nYour choice: ").strip()
        
        if choice == '3':
            print("👋 Goodbye!")
            return
        elif choice == '1':
            input_files = screenshots[:10]  # Limit to 10 for safety
        elif choice == '2':
            selected = input("Enter screenshot numbers (e.g., '1,3,5'): ").strip()
            try:
                indices = [int(x.strip()) - 1 for x in selected.split(',')]
                input_files = [screenshots[i] for i in indices if 0 <= i < len(screenshots)]
            except (ValueError, IndexError):
                print("❌ Invalid selection!")
                return
        else:
            print("❌ Invalid choice!")
            return
    
    # Process selected files
    print(f"\n🚀 Processing {len(input_files)} screenshot(s)...\n")
    
    success_count = 0
    for file_path in input_files:
        if resize_screenshot(file_path, output_dir):
            success_count += 1
    
    # Summary
    print("=" * 60)
    print(f"✅ Successfully processed {success_count}/{len(input_files)} screenshot(s)")
    print(f"📁 Output directory: {output_dir}")
    print("=" * 60)
    print("\n📱 Required sizes for App Store Connect:")
    print("   • 6.7\" Display (iPhone 16 Pro Max): 1290 × 2796px")
    print("   • 6.5\" Display (iPhone 11 Pro Max): 1242 × 2688px")
    print("   • 5.5\" Display (iPhone 8 Plus):     1242 × 2208px")
    print("\n💡 Upload these to App Store Connect → App Information → Screenshots")
    print()

if __name__ == "__main__":
    main()
