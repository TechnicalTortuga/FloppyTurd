# Building Floppy Turd

This document explains how to build Floppy Turd for different platforms using CMake.

## Prerequisites

- CMake 3.20 or higher
- C++20 compatible compiler
- Git

### Platform-specific requirements:

**Windows:**
- Visual Studio 2022 or MinGW-w64
- Windows SDK

**Android:**
- Android NDK r23 or higher
- Android SDK
- Java JDK 8 or higher

**iOS:**
- Xcode 14 or higher
- macOS 12 or higher

## Building for Desktop (Windows/Linux/macOS)

```bash
# Clone the repository
git clone <your-repo-url>
cd FloppyTurd

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run
./FloppyTurd  # or FloppyTurd.exe on Windows
```

### Visual Studio (Windows)

```bash
# Generate Visual Studio solution
cmake -G "Visual Studio 17 2022" -A x64 ..

# Open the generated .sln file in Visual Studio
```

## Building for Android

```bash
# Set up environment variables
export ANDROID_NDK_HOME=/path/to/android-ndk
export ANDROID_SDK_HOME=/path/to/android-sdk

# Create build directory
mkdir build-android
cd build-android

# Configure for Android
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-21 \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .
```

## Building for iOS

```bash
# Create build directory
mkdir build-ios
cd build-ios

# Configure for iOS
cmake .. \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .

# Open the generated Xcode project
open FloppyTurd.xcodeproj
```

## Build Options

- `CMAKE_BUILD_TYPE`: Debug, Release, RelWithDebInfo, MinSizeRel
- `BUILD_SHARED_LIBS`: Build as shared library (for Android)
- `SUPPORT_HIGHDPI`: Enable high DPI support

## Troubleshooting

### Missing Resources
Ensure the `resources` folder is copied to the build directory. CMake should handle this automatically.

### Android Build Issues
- Make sure NDK path is correct
- Try different Android ABI (armeabi-v7a for older devices)
- Check minimum Android API level

### iOS Build Issues
- Ensure you have valid Apple Developer certificates
- Check iOS deployment target compatibility 