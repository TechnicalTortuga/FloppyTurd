#!/bin/bash

# FloppyTurd Extended Attributes Cleanup Script
# This script removes extended attributes that prevent iOS code signing
# Based on Apple Technical Q&A QA1940

echo "🧹 Cleaning extended attributes from FloppyTurd project..."
echo "This fixes the 'resource fork, Finder information, or similar detritus not allowed' error"
echo ""

# Clean specific directories that matter for building
echo "Removing extended attributes from source files..."
xattr -cr "/Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd" 2>/dev/null || true

echo "Removing extended attributes from build directory..."
xattr -cr "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios" 2>/dev/null || true

echo "Removing extended attributes from resources..."
xattr -cr "/Users/aimac/Documents/GitHub/FloppyTurd/resources" 2>/dev/null || true

# Clean any existing app bundles specifically
echo "Cleaning app bundles..."
find "/Users/aimac/Documents/GitHub/FloppyTurd" -name "*.app" -exec xattr -cr {} \; 2>/dev/null || true

echo "✅ Metadata cleanup complete! Ready to build."
echo ""
echo "🚀 FloppyTurd prep done—time to build in Xcode!"
echo ""
echo "💡 Pro tip: Avoid using Finder's 'Show Package Contents' on .app bundles"
echo "   to prevent extended attributes from being re-added."
echo "📋 Run this script whenever you see the 'detritus not allowed' error."
