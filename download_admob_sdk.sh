#!/bin/bash

# Download Google Mobile Ads SDK manually and add to project
# This avoids CocoaPods and Swift Package Manager complexities

SDK_VERSION="12.12.0"
SDK_URL="https://dl.google.com/googleadmobadssdk/googlemobileadssdkios.zip"
SDK_DIR="ThirdParty/GoogleMobileAds"

echo "📦 Downloading Google Mobile Ads SDK..."
echo "   Version: ${SDK_VERSION}"
echo "   This will take a minute..."

# Create directory
mkdir -p "${SDK_DIR}"

# Download SDK
curl -L "${SDK_URL}" -o "${SDK_DIR}/sdk.zip"

if [ $? -eq 0 ]; then
    echo "✅ Downloaded successfully"
    
    # Unzip
    echo "📂 Extracting..."
    cd "${SDK_DIR}"
    unzip -q sdk.zip
    rm sdk.zip
    
    echo "✅ Google Mobile Ads SDK installed to ${SDK_DIR}"
    echo ""
    echo "📋 Next: Update CMakeLists.txt to link the frameworks"
    ls -la
else
    echo "❌ Download failed"
    exit 1
fi

