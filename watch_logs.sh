#!/bin/bash

# Script to watch FloppyTurd iOS Simulator logs

echo "=== FloppyTurd Log Watcher ==="
echo ""

# Get the bundle identifier
BUNDLE_ID="com.floppyturd.game"

# Find the simulator device ID for iPhone 16
DEVICE_ID=$(xcrun simctl list devices | grep "iPhone 16" | grep "Booted" | sed -E 's/.*\(([0-9A-F-]+)\).*/\1/')

if [ -z "$DEVICE_ID" ]; then
    echo "⚠️  No booted iPhone 16 simulator found."
    echo "Please boot the iPhone 16 simulator first."
    echo ""
    echo "Available simulators:"
    xcrun simctl list devices | grep "iPhone 16"
    exit 1
fi

echo "📱 Found booted iPhone 16: $DEVICE_ID"
echo ""

# Find the app container
APP_CONTAINER=$(xcrun simctl get_app_container "$DEVICE_ID" "$BUNDLE_ID" 2>/dev/null)

if [ -z "$APP_CONTAINER" ]; then
    echo "⚠️  FloppyTurd app not installed on this simulator."
    echo "Please build and install the app first using Xcode."
    exit 1
fi

echo "📂 App Container: $APP_CONTAINER"
echo ""
echo "=== Watching logs (press Ctrl+C to stop) ==="
echo "Looking for: [ToiletPaper] and [COLLISION] messages..."
echo ""

# Use xcrun simctl spawn to stream logs
# Filter for FloppyTurd process and show ToiletPaper/COLLISION logs
xcrun simctl spawn "$DEVICE_ID" log stream --predicate 'processImagePath contains "FloppyTurd"' --style compact 2>/dev/null | grep --line-buffered -E "\[ToiletPaper\]|\[COLLISION\]|Player.*GetCircle"

# Alternative: Use system log
# log stream --predicate 'processImagePath contains "FloppyTurd"' --style compact | grep --line-buffered -E "\[ToiletPaper\]|\[COLLISION\]"
