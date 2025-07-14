cmake_minimum_required(VERSION 3.5)
project(FloppyTurd VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for different platforms
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(SUPPORT_HIGHDPI "Support high DPI displays" ON)

# Platform detection
if(NOT DEFINED PLATFORM)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
        set(PLATFORM "Android")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
        # Let the toolchain handle iOS specifics, default to OS64 for consistency
        set(PLATFORM "OS64")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(PLATFORM "Desktop")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(PLATFORM "Desktop")  # macOS uses Desktop platform for raylib
        set(IS_MACOS TRUE)
    else()
        set(PLATFORM "Desktop")
    endif()
endif()

message(STATUS "Building for platform: ${PLATFORM}")
message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
if(IS_MACOS)
    message(STATUS "macOS detected - using Desktop platform with macOS-specific settings")
endif()

# Temporary: Override PLATFORM for raylib configure step only
set(_FTURD_SAVED_PLATFORM "${PLATFORM}")
set(PLATFORM Desktop CACHE STRING "Raylib platform override" FORCE)

# Find or fetch Raylib (only for non-iOS platforms)
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "iOS")
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
else()
    # iOS uses Metal/SpriteKit instead of Raylib
    message(STATUS "iOS platform detected - using Metal/SpriteKit for rendering")
    
    # No external dependencies needed for Metal/SpriteKit - they're part of iOS SDK
    # The frameworks will be linked directly in the target_link_libraries section
endif()

set(PLATFORM "${_FTURD_SAVED_PLATFORM}" CACHE STRING "" FORCE)

# Collect source files
file(GLOB_RECURSE CPP_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.cpp
)
file(GLOB_RECURSE OBJC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.mm
)
set(SOURCES ${CPP_SOURCES} ${OBJC_SOURCES})


# Remove files that no longer exist (were deleted during refactor)
# Note: These files were already removed, so these commands are safe to keep
# but they won't do anything if the files don't exist
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/PlayerOLD.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/GameLog.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/LogManager.cpp)

# Add new files that were created during refactor
list(APPEND SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/GlobalStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/FloppyTurdInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/UIManager.cpp
)

    # Add platform-specific compatibility layer
    if(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
        # iOS uses PlatformAPI and PlatformIOS for Metal implementation
        list(APPEND SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/AudioStateManager.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/GameLog.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/LogManager.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/main_ios.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/WindowIOS.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS/AppDelegate.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS/GameViewController.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS/GameView.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS/HapticsManager.mm
            # Metal rendering implementation files
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalRenderer.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalFrameResources.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalTextureCache.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalTexture.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/FontAtlasGenerator.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/FontCache.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalShader.mm
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalTextRenderer.mm
            # iOS-specific coordinate system implementation
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/UICoordinateSystem.mm
        )
    
    # Remove main.cpp from iOS builds to avoid duplicate main functions
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/main.cpp)
else()
    # Other platforms use PlatformAPI with direct Raylib
    # Explicitly add main.cpp for non-iOS platforms
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/main.cpp)
endif()

# Note: main.cpp vs main_ios.mm is handled above in platform-specific sections

file(GLOB_RECURSE HEADERS 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.h
)
# Remove the GameSettings.h and Resources.h from sources (header only)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/GameSettings.h)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Resources.h)

# For iOS, remove files that are replaced by .mm versions
if(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    # Remove AudioStateManager.cpp since we use AudioStateManager.mm for iOS
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/AudioStateManager.cpp)
    # Remove UICoordinateSystem.cpp since we use Metal/UIKit implementation for iOS
    list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/UICoordinateSystem.cpp)
endif()

# TouchControls, ResourceManager, and AIGUI.cpp are already included via GLOB_RECURSE

# Create executable
if(${PLATFORM} STREQUAL "Android")
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
elif(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    # iOS: Create as app bundle with automatic signing
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug-iphoneos"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug-iphoneos"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug-iphoneos"
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd
)

# Add iOS-specific include directories
if(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Shaders
    )
endif()

# Link libraries - conditional based on platform
if(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    # iOS uses Metal/SpriteKit native frameworks
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "-framework Foundation"
        "-framework UIKit"
        "-framework Metal"
        "-framework MetalKit"
        "-framework MetalPerformanceShaders"
        "-framework SpriteKit"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AVFoundation"
        "-framework GameController"
        "-framework ImageIO"
        "-framework MobileCoreServices"
        "-framework CoreHaptics"
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        PLATFORM_IOS
        PLATFORM_MOBILE
        __APPLE__
        TARGET_OS_IOS=1
    )
    
    # Bundle resources - use asset catalogs for proper iOS packaging
    file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources/*")
    foreach(RESOURCE ${RESOURCES})
        set_source_files_properties(${RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    endforeach()
    
    # Explicitly add font files to bundle for iOS font registration (only from resources, not asset catalog)
    file(GLOB FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources/fonts/*.ttf")
    foreach(FONT_FILE ${FONT_FILES})
        set_source_files_properties(${FONT_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        target_sources(${PROJECT_NAME} PRIVATE ${FONT_FILE})
    endforeach()
    
    # Define Metal shader files
    set(METAL_SHADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Shaders2D.metal
    )
    
    # Only add the .metal file as a resource for Xcode to pick up
    if(DEFINED METAL_SHADERS)
        foreach(SHADER ${METAL_SHADERS})
            set_source_files_properties(${SHADER} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            target_sources(${PROJECT_NAME} PRIVATE ${SHADER})
        endforeach()
    endif()
    
    # Asset catalogs handle resources properly, no need for cleanup scripts
    
    # ---- Asset Catalog generation for iOS resources ----
    
    # Create asset catalog structure with generate_asset_catalog.sh script
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_asset_catalog.sh
        COMMENT "Generating asset catalog from resources"
        VERBATIM
    )
    
    # Include the asset catalog as a resource bundle
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Assets.xcassets PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Assets.xcassets)
    
    # Configure basic iOS target properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.floppyturd.game"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.floppyturd.game"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "13.0"
        XCODE_ATTRIBUTE_PRODUCT_NAME "FloppyTurd"
        XCODE_ATTRIBUTE_BUNDLE_VERSION "1.0.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_UI_REQUIRED_DEVICE_CAPABILITIES "arm64"
        XCODE_ATTRIBUTE_UI_SUPPORTED_INTERFACE_ORIENTATIONS "UIInterfaceOrientationPortrait;UIInterfaceOrientationLandscapeLeft;UIInterfaceOrientationLandscapeRight"
        # Enable code signing
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "86886N72J8"
    )
else()
    # All other platforms use Raylib
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
endif()

# Platform-specific settings
if(${PLATFORM} STREQUAL "Windows")
    # Windows specific settings
    # Use console entry point (main() instead of WinMain())
    
    # Copy resources to output directory (skip for iOS - we use asset catalogs instead)
    if(NOT APPLE OR NOT IOS)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
        )
    endif()
    
    # Set Visual Studio debugger working directory
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    
elif(IS_MACOS)
    # macOS specific settings
    message(STATUS "Applying macOS-specific build settings")
    
    # Copy resources to output directory for macOS (skip for iOS - we use asset catalogs instead)
    # For macOS, we can use the standard resource copy approach
    if(NOT IOS)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
        )
    endif()
    
    # macOS-specific compiler flags for better compatibility
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-deprecated-declarations  # Silence deprecation warnings on macOS
    )
    
elif(${PLATFORM} STREQUAL "Android")
    # Android specific settings
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_ANDROID)
    
    # Android requires specific flags
    target_link_libraries(${PROJECT_NAME} PRIVATE
        android
        EGL
        GLESv2
        OpenSLES
        log
        m
    )
    
else()
    # Default/Linux settings
    # Copy resources to output directory for other platforms (skip for iOS - we use asset catalogs instead)
    if(NOT IOS)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
        )
    endif()
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
