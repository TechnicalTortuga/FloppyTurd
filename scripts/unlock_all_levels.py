#!/usr/bin/env python3
"""
Unlock All Levels Tool for FloppyTurd
Modifies the save file to unlock all levels for testing
"""

import plistlib
import sys
from pathlib import Path

def unlock_all_levels(plist_path):
    """Unlock all levels in the save file"""
    
    # Read the plist file
    with open(plist_path, 'rb') as f:
        save_data = plistlib.load(f)
    
    print("📂 Loaded save file:")
    print(f"   Version: {save_data.get('version', 'unknown')}")
    print(f"   Legacy High Score: {save_data.get('legacyHighScore', 0)}")
    
    # Get the progress section
    progress = save_data.get('progress', {})
    levels = progress.get('levels', [])
    
    print(f"\n📊 Found {len(levels)} level(s)")
    
    # Unlock all levels
    unlocked_count = 0
    for level in levels:
        level_id = level.get('levelId', 0)
        was_locked = not level.get('unlocked', False)
        
        # Unlock the level
        level['unlocked'] = True
        
        if was_locked:
            unlocked_count += 1
            print(f"   🔓 Unlocked Level {level_id}")
        else:
            print(f"   ✅ Level {level_id} already unlocked")
    
    # Also give some coins for testing
    stats = save_data.get('statistics', {})
    original_coins = stats.get('storedCoins', 0)
    stats['storedCoins'] = 99999
    save_data['statistics'] = stats
    
    print(f"\n💰 Coins: {original_coins} → 99,999")
    
    # Unlock all hats
    customization = save_data.get('customization', {})
    original_hats = customization.get('unlockedHats', [])
    # Unlock hats 0-4 (assuming there are 5 hats)
    customization['unlockedHats'] = [0, 1, 2, 3, 4]
    save_data['customization'] = customization
    
    print(f"\n🎩 Hats unlocked: {len(original_hats)} → {len(customization['unlockedHats'])}")
    
    # Write back the modified plist
    with open(plist_path, 'wb') as f:
        plistlib.dump(save_data, f)
    
    print(f"\n✅ Save file updated!")
    print(f"   Total levels unlocked: {unlocked_count}")
    print(f"   Coins: 99,999")
    print(f"   All hats unlocked")
    print(f"\n💾 Restart the game to see changes")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 unlock_all_levels.py <path_to_save_file.plist>")
        print("\nExample:")
        print("  python3 unlock_all_levels.py /path/to/floppyturd_save_v3.plist")
        sys.exit(1)
    
    plist_path = Path(sys.argv[1])
    
    if not plist_path.exists():
        print(f"❌ Error: File not found: {plist_path}")
        sys.exit(1)
    
    # Create backup
    backup_path = plist_path.with_suffix('.plist.backup')
    import shutil
    shutil.copy(plist_path, backup_path)
    print(f"💾 Backup created: {backup_path}")
    
    try:
        unlock_all_levels(plist_path)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        print(f"   Restoring from backup...")
        shutil.copy(backup_path, plist_path)
        print(f"   Backup restored")
        sys.exit(1)
