#!/bin/bash
# Reset Save Data - Deletes all save files from iOS Simulator
# Use this for testing a fresh game start

echo "🗑️  Resetting Floppy Turd save data..."

# Find the app's container in the booted Simulator using simctl
APP_CONTAINER=$(xcrun simctl get_app_container booted com.floppyturd.game data 2>/dev/null)

if [ -z "$APP_CONTAINER" ]; then
    echo "⚠️  App container not found."
    echo "💡 Make sure:"
    echo "   1. iOS Simulator is running"
    echo "   2. You've built and launched the app at least once"
    echo "   3. The app's bundle ID is com.floppyturd.game"
    exit 1
fi

DOCS_DIR="$APP_CONTAINER/Documents"

if [ ! -d "$DOCS_DIR" ]; then
    echo "⚠️  Documents directory not found at: $DOCS_DIR"
    exit 1
fi

echo "📁 Found app container: $APP_CONTAINER"
echo "📁 Documents directory: $DOCS_DIR"
echo ""
echo "🔍 Current save files:"
ls -lh "$DOCS_DIR"/*.plist "$DOCS_DIR"/*.json "$DOCS_DIR"/*.dat "$DOCS_DIR"/*.cfg "$DOCS_DIR"/*.txt 2>/dev/null || echo "  (none found)"
echo ""

# Delete all save files
echo "🗑️  Deleting save files..."
rm -f "$DOCS_DIR"/floppyturd_save*.plist
rm -f "$DOCS_DIR"/floppyturd_save*.json
rm -f "$DOCS_DIR"/floppyturd_save*.dat
rm -f "$DOCS_DIR"/floppyturd_settings.cfg
rm -f "$DOCS_DIR"/hat_status.txt

echo ""
echo "✅ Save data deleted!"
echo "📝 Debug flags are set to FALSE in FloppyTurdGame.cpp"
echo ""
echo "🎮 Next steps:"
echo "  1. Quit the app in Simulator (if running)"
echo "  2. Launch again - you'll see a fresh game!"
echo "  3. All levels locked (except Level 1), all hats locked, no coins, no hat equipped"
echo ""
