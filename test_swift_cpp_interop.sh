#!/bin/bash

# Test Swift/C++ Interoperability Build
# This script helps diagnose and test the module configuration

echo "=== Swift/C++ Interop Test Script ==="
echo

# Clean build artifacts
echo "1. Cleaning build artifacts..."
rm -rf build_ios
rm -rf ~/Library/Developer/Xcode/DerivedData/FloppyTurd-*
rm -rf /var/folders/*/C/org.llvm.clang.*/ModuleCache

# Create build directory
mkdir -p build_ios
cd build_ios

echo
echo "2. Running CMake configuration..."
cmake -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DCMAKE_OSX_ARCHITECTURES="arm64" \
    -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
    -DCMAKE_IOS_INSTALL_COMBINED=YES \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
    -DCMAKE_CXX_FLAGS="-fmodules -fcxx-modules -std=c++17" \
    -DCMAKE_SWIFT_FLAGS="-cxx-interoperability-mode=default" \
    ..

echo
echo "3. Building for iOS Simulator..."
xcodebuild -project FloppyTurd.xcodeproj \
    -scheme FloppyTurd \
    -sdk iphonesimulator \
    -destination "platform=iOS Simulator,name=iPhone 16" \
    -configuration Debug \
    OTHER_CPLUSPLUSFLAGS="-fmodules -fcxx-modules" \
    OTHER_SWIFT_FLAGS="-cxx-interoperability-mode=default" \
    CLANG_ENABLE_MODULES=YES \
    build 2>&1 | tee ../build_output_test.txt

echo
echo "4. Checking for ThreadingProxy errors..."
if grep -i "redefinition.*ThreadingProxy" ../build_output_test.txt; then
    echo "ERROR: ThreadingProxy redefinition errors found!"
    echo
    echo "Suggested fixes:"
    echo "1. Ensure all C++ files use conditional includes for ThreadingProxy.h"
    echo "2. Clear module cache and rebuild"
    echo "3. Check that module.modulemap is properly configured"
else
    echo "No ThreadingProxy redefinition errors found."
fi

echo
echo "5. Checking if Swift can access ThreadingProxy functions..."
if grep -i "has no member.*getAndClearRenderCommandsFromProxy" ../build_output_test.txt; then
    echo "ERROR: Swift cannot access ThreadingProxy functions!"
    echo
    echo "Suggested fixes:"
    echo "1. Ensure functions are declared in the FloppyTurd namespace"
    echo "2. Check that ThreadingProxy.h is included in module.modulemap"
    echo "3. Verify Swift interop mode is enabled"
else
    echo "Swift appears to have access to ThreadingProxy functions."
fi

echo
echo "=== Test Complete ==="
echo "Check build_output_test.txt for full build log." 