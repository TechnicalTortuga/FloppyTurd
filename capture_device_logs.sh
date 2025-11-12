#!/bin/bash
# Capture FloppyTurd logs from connected iPhone
# Usage: ./capture_device_logs.sh

echo "📱 Capturing logs from iPhone..."
echo "Make sure FloppyTurd is running on your device!"
echo ""
echo "Press Ctrl+C to stop capturing"
echo "=========================================="
echo ""

# Use xcrun to get device logs
xcrun xctrace record --device "00008140-001E39CC3A47001C" --template 'Logging' --output /tmp/floppyturd_trace.trace --time-limit 60s &
TRACE_PID=$!

echo "Recording for 60 seconds..."
echo "Interact with your app now (tap buttons, view leaderboards, etc.)"
wait $TRACE_PID

echo ""
echo "✅ Logs captured to /tmp/floppyturd_trace.trace"
echo "Open in Instruments to view"
