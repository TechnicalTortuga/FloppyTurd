cmake_minimum_required(VERSION 3.20)
project(PooperTrooper VERSION 1.0.3 LANGUAGES CXX Swift)

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set Swift version and enable C++ interop
set(CMAKE_Swift_LANGUAGE_VERSION 6.0)
enable_language(Swift)

# Platform detection - Enhanced logic that works with iOS toolchain
# Debug output to help diagnose platform detection issues
message(STATUS "Platform Detection Debug:")
message(STATUS "  CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  CMAKE_OSX_SYSROOT: ${CMAKE_OSX_SYSROOT}")
message(STATUS "  CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
message(STATUS "  APPLE: ${APPLE}")
message(STATUS "  CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "  PLATFORM (from toolchain): ${PLATFORM}")

# Reset our platform detection variables
unset(PLATFORM_IOS)
unset(PLATFORM_DESKTOP)
unset(PLATFORM_MOBILE)

# Check if we're using iOS toolchain (explicit toolchain detection)
if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "ios\.toolchain\.cmake")
    set(FLOPPY_PLATFORM "iOS")
    set(PLATFORM_IOS TRUE)
    set(PLATFORM_MOBILE TRUE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0")
    set(IPHONEOS_DEPLOYMENT_TARGET "17.0")
    message(STATUS "Detected iOS build via toolchain - Building for iOS 17+ with Swift 6.0 and C++ interop")
    # Check for Xcode iOS builds (sysroot-based detection)
elseif(APPLE AND CMAKE_OSX_SYSROOT AND (CMAKE_OSX_SYSROOT MATCHES "iphone" OR CMAKE_OSX_SYSROOT MATCHES "ios"))
    set(FLOPPY_PLATFORM "iOS")
    set(PLATFORM_IOS TRUE)
    set(PLATFORM_MOBILE TRUE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0")
    set(IPHONEOS_DEPLOYMENT_TARGET "17.0")
    message(STATUS "Detected iOS build via Xcode sysroot - Building for iOS 17+ with Swift 6.0 and C++ interop")
    # Check for explicit iOS system name
elseif(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(FLOPPY_PLATFORM "iOS")
    set(PLATFORM_IOS TRUE)
    set(PLATFORM_MOBILE TRUE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0")
    set(IPHONEOS_DEPLOYMENT_TARGET "17.0")
    message(STATUS "Detected iOS build via system name - Building for iOS 17+ with Swift 6.0 and C++ interop")
    # Android detection
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(FLOPPY_PLATFORM "Android")
    set(PLATFORM_MOBILE TRUE)
    message(STATUS "Building for Android")
    # macOS detection (only if not iOS)
elseif(APPLE)
    set(FLOPPY_PLATFORM "macOS")
    set(PLATFORM_DESKTOP TRUE)
    message(STATUS "Building for macOS desktop")
    # Generic desktop fallback
else()
    set(FLOPPY_PLATFORM "Desktop")
    set(PLATFORM_DESKTOP TRUE)
    message(STATUS "Building for generic desktop platform")
endif()

# Final platform confirmation
message(STATUS "Final PooperTrooper Platform: ${FLOPPY_PLATFORM}")
message(STATUS "PLATFORM_IOS: ${PLATFORM_IOS}")
message(STATUS "PLATFORM_DESKTOP: ${PLATFORM_DESKTOP}")

# =============================================================================
# iOS PLATFORM CONFIGURATION (NO RAYLIB - USES METAL/SPRITEKIT)
# =============================================================================
if(PLATFORM_IOS)
    # =============================================================================
    # CocoaPods Integration - DISABLED (use Swift Package Manager in Xcode instead)
    # =============================================================================
    # Automatic CocoaPods integration disabled to avoid linker conflicts
    # To use CocoaPods: Run 'pod install' manually, then open PooperTrooper.xcworkspace
    # Recommended: Add Google Mobile Ads SDK via Swift Package Manager in Xcode
    message(STATUS "CocoaPods auto-integration disabled - add Google Mobile Ads SDK via Xcode SPM")

    # Swift/C++ interoperability settings - keep this minimal
    set(CMAKE_Swift_FLAGS "-cxx-interoperability-mode=default -swift-version 6.0")

    # Include paths for iOS
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Assets
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game
    )

    # Expose C++ headers for Swift interop
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Input
    )

    # Collect C++ game sources (will be added directly to main target - no static library)
    file(GLOB_RECURSE FLOPPYTURD_CPP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/States/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/States/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Config/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Config/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Input/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Input/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Configuration/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Configuration/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Utility/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Utility/*.h"
    )

    # Remove any Raylib sources from FLOPPYTURD_CPP_SOURCES for iOS
    list(FILTER FLOPPYTURD_CPP_SOURCES EXCLUDE REGEX ".*Raylib/.*")
    list(FILTER FLOPPYTURD_CPP_SOURCES EXCLUDE REGEX ".*RaylibPlatformImpl.*")
    
    # Note: C++ sources will be added directly to main app target below (no separate static library)
endif()

# =============================================================================
# DESKTOP/OTHER PLATFORMS CONFIGURATION (USES RAYLIB)
# =============================================================================
if(PLATFORM_DESKTOP AND NOT PLATFORM_IOS)
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

# Common Engine core sources (shared between platforms)
file(GLOB_RECURSE ENGINE_CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Events/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Events/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Input/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Input/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Assets/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Assets/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Utility/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Utility/*.h"
)

# Common PooperTrooper game sources (used by both platforms)
file(GLOB_RECURSE GAME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Components/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Components/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Config/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Config/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Entities/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Entities/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/States/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/States/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems/*.h"
)

# Platform-specific source collection
if(PLATFORM_IOS)
    # iOS sources (NO RAYLIB - uses Metal/SpriteKit)
    file(GLOB_RECURSE IOS_CPP_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Audio/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Input/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Logging/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Rendering/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Threading/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Assets/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Audio/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Input/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Logging/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Rendering/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Threading/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Assets/*.h"
    )

    file(GLOB_RECURSE IOS_SWIFT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Assets/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Logging/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Rendering/*.swift"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Threading/*.swift"
    )

    # Remove any Raylib sources from ENGINE_CORE_SOURCES and GAME_SOURCES for iOS
    list(FILTER ENGINE_CORE_SOURCES EXCLUDE REGEX ".*Raylib/.*")
    list(FILTER GAME_SOURCES EXCLUDE REGEX ".*Raylib/.*")

    # Combine ALL sources for iOS into single target (NO RAYLIB, NO STATIC LIBRARY)
    set(ALL_SOURCES
        ${ENGINE_CORE_SOURCES}
        ${IOS_CPP_SOURCES}
        ${IOS_SWIFT_SOURCES}
        ${GAME_SOURCES}
        ${FLOPPYTURD_CPP_SOURCES}
    )

    message(STATUS "iOS sources collected (Raylib sources explicitly excluded)")
else()
    # Desktop/Other platform sources (WITH RAYLIB)
    file(GLOB_RECURSE RAYLIB_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Input/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Input/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Logging/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Logging/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Rendering/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Rendering/*.h"
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
    # Create iOS app bundle with ALL sources (C++, Swift, Engine - no static library)
    add_executable(PooperTrooper MACOSX_BUNDLE ${ALL_SOURCES})

    # iOS-specific include directories (add Engine for module.modulemap)
    target_include_directories(PooperTrooper PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Assets
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Configuration
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Utility
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iOS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Audio
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Logging
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Threading
        ${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Assets
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Components
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Config
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Entities
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/States
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Systems
    )
    # Ensure Swift can find the module map and headers
    set_target_properties(PooperTrooper PROPERTIES
        SWIFT_INCLUDE_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    # Ensure Swift can find the module map and headers
    set_target_properties(PooperTrooper PROPERTIES
        HEADER_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine"
    )

    # iOS Framework dependencies (NO RAYLIB)
    target_link_libraries(PooperTrooper PRIVATE
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

    # Google Mobile Ads SDK - Link for device builds only
    # AdManager.swift uses #if targetEnvironment(simulator) for stub implementation on simulator
    set(GOOGLE_ADS_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GoogleMobileAds/GoogleMobileAdsSdkiOS-12.12.0")
    
    if(PLATFORM STREQUAL "OS64" OR PLATFORM STREQUAL "OS" OR NOT PLATFORM MATCHES "SIMULATOR")
        # Device build - link GoogleMobileAds
        message(STATUS "✅ GoogleMobileAds XCFramework linked for DEVICE build")
        
        # Add framework search path
        set_target_properties(PooperTrooper PROPERTIES
            XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "$(inherited) ${GOOGLE_ADS_SDK_PATH}"
        )
        
        # Link the XCFrameworks
        target_link_libraries(PooperTrooper PRIVATE
            "-framework GoogleMobileAds"
            "-framework UserMessagingPlatform"
        )
        
        # Embed the frameworks with proper signing
        set_target_properties(PooperTrooper PROPERTIES
            XCODE_EMBED_FRAMEWORKS "${GOOGLE_ADS_SDK_PATH}/GoogleMobileAds.xcframework;${GOOGLE_ADS_SDK_PATH}/UserMessagingPlatform.xcframework"
            XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
            XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
            XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "$(inherited) @executable_path/Frameworks"
        )
    else()
        # Simulator build - skip GoogleMobileAds (uses stub in AdManager.swift)
        message(STATUS "⚠️  GoogleMobileAds XCFramework NOT linked (SIMULATOR build - using stubs)")
    endif()
else()
    # Create desktop executable
    add_executable(PooperTrooper ${ALL_SOURCES})

    # Desktop-specific include directories
    target_include_directories(PooperTrooper PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Platform
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Logging
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Raylib/Rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Components
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Entities
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/Game
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FloppyTurd/States
    )

    # Desktop dependencies (WITH RAYLIB)
    target_link_libraries(PooperTrooper PRIVATE raylib)
endif()

# Platform-specific compile definitions
if(PLATFORM_IOS)
    set(COMMON_COMPILE_DEFINITIONS
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Debug>:_DEBUG=1>
        PLATFORM_IOS=1
        PLATFORM_MOBILE=1
        __APPLE__
        TARGET_OS_IOS=1
        SWIFT_CPP_INTEROP_ENABLED
    )
else()
    set(COMMON_COMPILE_DEFINITIONS
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Debug>:_DEBUG=1>
        PLATFORM_DESKTOP=1
    )
endif()

# Apply common compile definitions and properties to all targets
target_compile_definitions(PooperTrooper PRIVATE ${COMMON_COMPILE_DEFINITIONS})

# Global Xcode settings (Minimal to allow Automatic signing to work)
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "17.0")

# Platform-specific target properties
if(PLATFORM_IOS)
    # Check if Info.plist exists, if not generate a minimal one
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
        message(WARNING "Info.plist not found, will be auto-generated by Xcode")
    else()
        message(STATUS "Using Info.plist: ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    endif()

    # Check if entitlements file exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/PooperTrooper.entitlements")
        set(ENTITLEMENTS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/PooperTrooper.entitlements")
        message(STATUS "Using entitlements file: ${ENTITLEMENTS_FILE}")
    else()
        message(WARNING "PooperTrooper.entitlements not found - app may not sign properly")
        set(ENTITLEMENTS_FILE "")
    endif()

    set_target_properties(PooperTrooper PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.floppyturd.game"
        MACOSX_BUNDLE_BUNDLE_NAME "PooperTrooper"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.3"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.3"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.floppyturd.game"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "W77G58P3X9"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "17.0"
        XCODE_ATTRIBUTE_PRODUCT_NAME "PooperTrooper"
        XCODE_ATTRIBUTE_MARKETING_VERSION "1.0.3"
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "1"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # 1=iPhone, 2=iPad (Universal App)
        XCODE_ATTRIBUTE_INFOPLIST_KEY_UISupportedInterfaceOrientations "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight"
        XCODE_ATTRIBUTE_INFOPLIST_KEY_UISupportedInterfaceOrientations_iPad "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight"
        XCODE_ATTRIBUTE_INFOPLIST_KEY_UISupportedInterfaceOrientations_iPhone "UIInterfaceOrientationPortrait"
        XCODE_ATTRIBUTE_INFOPLIST_KEY_UILaunchScreen_Generation "YES"
        XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "PooperTrooper-Swift.h"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[sdk=iphoneos*] "Apple Development"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
        XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS "YES"
        XCODE_ATTRIBUTE_SWIFT_VERSION "6.0"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_INSTALL_PATH "$(LOCAL_APPS_DIR)"
        XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES"
        XCODE_ATTRIBUTE_COPY_PHASE_STRIP "NO"
        XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "NO"
    )

    # Set entitlements file if it exists
    if(ENTITLEMENTS_FILE)
        set_target_properties(PooperTrooper PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${ENTITLEMENTS_FILE}"
        )
    endif()

    # Swift/C++ interop compiler options
    target_compile_options(PooperTrooper PRIVATE
        $<$<COMPILE_LANGUAGE:Swift>:-cxx-interoperability-mode=default>
        $<$<COMPILE_LANGUAGE:Swift>:-swift-version 6.0>
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>
    )

endif()

# No separate static library - all code is in main target now

# ---- Asset Catalog generation for iOS resources ----
if(PLATFORM_IOS)
    # Create asset catalog structure with generate_asset_catalog.sh
    add_custom_command(TARGET PooperTrooper PRE_BUILD
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_asset_catalog.sh
        COMMENT "Generating asset catalog from resources"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Explicitly include the Assets.xcassets directory in the app bundle
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Assets.xcassets
        PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Add the Assets.xcassets directory to the target
    target_sources(PooperTrooper PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Assets.xcassets
    )

    # Set the ASSETCATALOG_COMPILER_APPICON_NAME if not set
    set_target_properties(PooperTrooper PROPERTIES
        ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    )

    # Explicitly add font files to bundle for iOS font registration (from new assets directory)
    # This makes .fnt files accessible via Bundle.main.path() calls
    file(GLOB FONT_FNT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/fonts/bitmap/*.fnt")
    file(GLOB FONT_FNT_FILES2 "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/fonts/truetype/*.fnt")
    list(APPEND FONT_FNT_FILES ${FONT_FNT_FILES2})
    message(STATUS "Found .fnt files: ${FONT_FNT_FILES}")
    foreach(FONT_FILE ${FONT_FNT_FILES})
        message(STATUS "Adding .fnt file to bundle: ${FONT_FILE}")
        set_source_files_properties(${FONT_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        target_sources(PooperTrooper PRIVATE ${FONT_FILE})
    endforeach()

    # Also add .ttf files for completeness
    file(GLOB FONT_TTF_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/fonts/bitmap/*.ttf")
    file(GLOB FONT_TTF_FILES2 "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/fonts/truetype/*.ttf")
    list(APPEND FONT_TTF_FILES ${FONT_TTF_FILES2})
    message(STATUS "Found .ttf files: ${FONT_TTF_FILES}")
    foreach(FONT_FILE ${FONT_TTF_FILES})
        message(STATUS "Adding .ttf file to bundle: ${FONT_FILE}")
        set_source_files_properties(${FONT_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        target_sources(PooperTrooper PRIVATE ${FONT_FILE})
    endforeach()

    # Also try adding fonts to a fonts subdirectory in the bundle
    file(GLOB FONT_ALL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/fonts/*/*")
    foreach(FONT_FILE ${FONT_ALL_FILES})
        if(NOT IS_DIRECTORY ${FONT_FILE})
            message(STATUS "Adding font file to fonts subdirectory: ${FONT_FILE}")
            set_source_files_properties(${FONT_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/fonts)
            target_sources(PooperTrooper PRIVATE ${FONT_FILE})
        endif()
    endforeach()

    # Add hat assets to bundle for iOS
    file(GLOB HAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/hats/*.png")
    message(STATUS "Found hat files: ${HAT_FILES}")
    foreach(HAT_FILE ${HAT_FILES})
        message(STATUS "Adding hat file to bundle: ${HAT_FILE}")
        set_source_files_properties(${HAT_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/hats)
        target_sources(PooperTrooper PRIVATE ${HAT_FILE})
    endforeach()

    # Metal shaders
    file(GLOB METAL_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/iOS/Shaders/*.metal")
    foreach(SHADER ${METAL_SHADERS})
        set_source_files_properties(${SHADER} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        target_sources(PooperTrooper PRIVATE ${SHADER})
    endforeach()
endif()

# Compiler warnings
target_compile_options(PooperTrooper PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>
)

if(PLATFORM_IOS)
    target_compile_options(PooperTrooper PRIVATE
        $<$<COMPILE_LANGUAGE:Swift>:-warnings-as-errors>
    )
endif()

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Status messages
if(PLATFORM_IOS)
    message(STATUS "PooperTrooper Engine configured for iOS 17+ with Swift 6.0 and native C++ interop")
    message(STATUS "Swift header: PooperTrooper-Swift.h")
    message(STATUS "Asset catalog will be generated from src/assets/")
else()
    message(STATUS "PooperTrooper Engine configured for ${PLATFORM} with Raylib")
endif()
