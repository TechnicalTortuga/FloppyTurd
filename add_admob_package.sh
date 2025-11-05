#!/bin/bash

# Add Google Mobile Ads SDK via Swift Package Manager to Xcode project
# This script modifies the Xcode project file to add the package dependency

PROJECT_FILE="build_ios/FloppyTurd.xcodeproj/project.pbxproj"

echo "📦 Adding Google Mobile Ads SDK package dependency to Xcode project..."

# Check if project file exists
if [ ! -f "$PROJECT_FILE" ]; then
    echo "❌ Error: Project file not found at $PROJECT_FILE"
    echo "Run cmake first to generate the Xcode project."
    exit 1
fi

# The easiest way is to use Xcode's command-line tools
# However, xcodebuild doesn't support adding packages directly

echo ""
echo "⚠️  Automated package addition requires Xcode GUI or manual project file editing."
echo ""
echo "📋 Please follow these steps in Xcode:"
echo ""
echo "1. Open the project:"
echo "   open build_ios/FloppyTurd.xcodeproj"
echo ""
echo "2. Select 'FloppyTurd' project in the navigator"
echo ""
echo "3. Go to File → Add Package Dependencies..."
echo ""
echo "4. Enter this URL:"
echo "   https://github.com/googleads/swift-package-manager-google-mobile-ads.git"
echo ""
echo "5. Select 'Up to Next Major Version' with version 12.0.0"
echo ""
echo "6. Click 'Add Package'"
echo ""
echo "7. Ensure 'GoogleMobileAds' is added to the 'FloppyTurd' target"
echo ""
echo "8. Click 'Add Package' to confirm"
echo ""
echo "✅ After adding the package, build with:"
echo "   xcodebuild -project build_ios/FloppyTurd.xcodeproj -scheme FloppyTurd -destination 'platform=iOS Simulator,name=iPhone 16,OS=18.3.1' build"
echo ""

