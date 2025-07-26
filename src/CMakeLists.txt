cmake_minimum_required(VERSION 3.25)
project(FloppyTurdEngine VERSION 1.0.0 LANGUAGES CXX Swift)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection and configuration
if(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(PLATFORM_IOS TRUE)
    set(PLATFORM_MOBILE TRUE)
    
    # Set Swift version and enable C++ interop for iOS
    set(CMAKE_Swift_LANGUAGE_VERSION 6.0)
    enable_language(Swift)
    
    # iOS 17+ deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0")
    set(IPHONEOS_DEPLOYMENT_TARGET "17.0")
    
    message(STATUS "Building for iOS 17+ with Swift 6.0 and C++ interop (NO RAYLIB)")
else()
    # Non-iOS platforms use Raylib
    set(PLATFORM_DESKTOP TRUE)
    message(STATUS "Building for Desktop/Other platforms with Raylib")
endif()

# =============================================================================
# iOS PLATFORM CONFIGURATION (NO RAYLIB - USES METAL/SPRITEKIT)
# =============================================================================
if(PLATFORM_IOS)
    # Swift/C++ interoperability settings
    set(CMAKE_Swift_FLAGS "-cxx-interoperability-mode=default -swift-version 6.0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules -fcxx-modules")
    
    # Module map configuration
    set(MODULE_MAP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../FloppyTurd/module.modulemap")
    if(EXISTS ${MODULE_MAP_PATH})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodule-map-file=${MODULE_MAP_PATH}")
        message(STATUS "Using module map: ${MODULE_MAP_PATH}")
    else()
        message(WARNING "Module map not found at: ${MODULE_MAP_PATH}")
    endif()
endif()

# =============================================================================
# DESKTOP/OTHER PLATFORMS CONFIGURATION (USES RAYLIB)
# =============================================================================
if(PLATFORM_DESKTOP)
    # Find or fetch Raylib
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
    
    message(STATUS "Raylib configured for desktop platform")
endif()

# =============================================================================
# SOURCE FILE COLLECTION
# =============================================================================

# Common Engine Core sources (used by both platforms)
file(GLOB_RECURSE ENGINE_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Events/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Events/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Engine/Platform/*.h"
)

# Common FloppyTurd game sources (used by both platforms)
file(GLOB_RECURSE GAME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Components/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Components/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Entities/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Entities/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Game/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Game/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/States/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/States/*.h"
)

# Platform-specific source collection
if(PLATFORM_IOS)
    # iOS sources (NO RAYLIB - uses Metal/SpriteKit)
    file(GLOB_RECURSE IOS_CPP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Audio/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Input/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Logging/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Rendering/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Threading/*.cpp"
    )
    
    file(GLOB_RECURSE IOS_SWIFT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Audio/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Input/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Logging/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Rendering/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Threading/*.swift"
    )
    
    # Combine sources for iOS (NO RAYLIB)
    set(ALL_SOURCES
        ${ENGINE_CORE_SOURCES}
        ${IOS_CPP_SOURCES}
        ${IOS_SWIFT_SOURCES}
        ${GAME_SOURCES}
    )
    
    message(STATUS "iOS sources collected (excluding Raylib)")
else()
    # Desktop/Other platform sources (WITH RAYLIB)
    file(GLOB_RECURSE RAYLIB_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Input/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Input/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Logging/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Logging/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Rendering/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Rendering/*.h"
    )
    
    # Combine sources for Desktop (WITH RAYLIB)
    set(ALL_SOURCES
        ${ENGINE_CORE_SOURCES}
        ${RAYLIB_SOURCES}
        ${GAME_SOURCES}
    )
    
    message(STATUS "Desktop sources collected (including Raylib)")
endif()

# =============================================================================
# TARGET CREATION AND CONFIGURATION
# =============================================================================

if(PLATFORM_IOS)
    # Create iOS app bundle
    add_executable(FloppyTurdEngine MACOSX_BUNDLE ${ALL_SOURCES})
    
    # iOS-specific include directories
    target_include_directories(FloppyTurdEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Platform
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/Audio
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/Logging
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/Rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/Threading
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Components
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Entities
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Game
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/States
    )
    
    # iOS Framework dependencies (NO RAYLIB)
    target_link_libraries(FloppyTurdEngine PRIVATE
        "-framework Foundation"
        "-framework UIKit"
        "-framework Metal"
        "-framework MetalKit"
        "-framework MetalPerformanceShaders"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AVFoundation"
        "-framework GameController"
        "-framework CoreHaptics"
        "-framework ImageIO"
        "-framework MobileCoreServices"
    )
else()
    # Create desktop executable
    add_executable(FloppyTurdEngine ${ALL_SOURCES})
    
    # Desktop-specific include directories
    target_include_directories(FloppyTurdEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Platform
        ${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Logging
        ${CMAKE_CURRENT_SOURCE_DIR}/Raylib/Rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Components
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Entities
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Game
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/States
    )
    
    # Desktop dependencies (WITH RAYLIB)
    target_link_libraries(FloppyTurdEngine PRIVATE raylib)
endif()

# Compiler definitions
target_compile_definitions(FloppyTurdEngine PRIVATE
    PLATFORM_IOS
    PLATFORM_MOBILE
    __APPLE__
    TARGET_OS_IOS=1
    SWIFT_CPP_INTEROP_ENABLED
)

# Swift/C++ interop compiler options
target_compile_options(FloppyTurdEngine PRIVATE
    $<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>
    $<$<COMPILE_LANGUAGE:Swift>:-swift-version 6.0>
    $<$<COMPILE_LANGUAGE:CXX>:-fmodules>
    $<$<COMPILE_LANGUAGE:CXX>:-fcxx-modules>
)

# iOS Bundle properties
set_target_properties(FloppyTurdEngine PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.floppyturd.engine"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.floppyturd.engine"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "86886N72J8"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "17.0"
    XCODE_ATTRIBUTE_PRODUCT_NAME "FloppyTurdEngine"
    XCODE_ATTRIBUTE_BUNDLE_VERSION "1.0.0"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_UI_REQUIRED_DEVICE_CAPABILITIES "arm64"
    XCODE_ATTRIBUTE_UI_SUPPORTED_INTERFACE_ORIENTATIONS "UIInterfaceOrientationPortrait;UIInterfaceOrientationLandscapeLeft;UIInterfaceOrientationLandscapeRight"
    # Swift header configuration for C++ interop
    XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "FloppyTurdEngine-Swift.h"
    # Enable automatic code signing
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    # Swift 6.0 specific settings
    XCODE_ATTRIBUTE_SWIFT_VERSION "6.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULE_DEBUGGING "YES"
)

# Output directories
set_target_properties(FloppyTurdEngine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug-iphonesimulator"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug-iphonesimulator"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug-iphonesimulator"
)

# Asset catalog and resources
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../FloppyTurd/Assets.xcassets")
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../FloppyTurd/Assets.xcassets" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(FloppyTurdEngine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../FloppyTurd/Assets.xcassets")
endif()

# Metal shaders
file(GLOB METAL_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/../FloppyTurd/*.metal")
foreach(SHADER ${METAL_SHADERS})
    set_source_files_properties(${SHADER} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(FloppyTurdEngine PRIVATE ${SHADER})
endforeach()

# Font resources
file(GLOB FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Assets/fonts/*.ttf")
foreach(FONT_FILE ${FONT_FILES})
    set_source_files_properties(${FONT_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(FloppyTurdEngine PRIVATE ${FONT_FILE})
endforeach()

# Compiler warnings
target_compile_options(FloppyTurdEngine PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
    $<$<COMPILE_LANGUAGE:Swift>:-warnings-as-errors>
)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "FloppyTurd Engine configured for iOS 17+ with Swift 6.0 and native C++ interop")
message(STATUS "Module map: ${MODULE_MAP_PATH}")
message(STATUS "Swift header: FloppyTurdEngine-Swift.h")
message(STATUS "Total source files: ${CMAKE_CURRENT_LIST_LENGTH}")