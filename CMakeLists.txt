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

# Collect source files
file(GLOB_RECURSE SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.cpp
)

# Remove PlayerOLD.cpp from sources
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/PlayerOLD.cpp)

# Add platform-specific compatibility layer
if(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    # iOS uses Metal compatibility layer
    list(APPEND SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalRaylibCompat.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalRenderer.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalTexture.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalShader.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/MetalTextRenderer.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS/AppDelegate.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS/GameViewController.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/iOS/HapticsManager.mm
    )
    # Add Metal shader files as resources
    set(METAL_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Shaders2D.metal)
else()
    # Other platforms use SDL or direct Raylib
    list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/RaylibCompat.cpp)
endif()

# Explicitly add main.cpp to ensure it's included
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/main.cpp)

file(GLOB_RECURSE HEADERS 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.h
)

# Remove the GameSettings.h and Resources.h from sources (header only)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/GameSettings.h)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/Resources.h)

# Add new platform abstraction files
list(APPEND SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/PlatformLayer.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/TouchControls.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/ResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/AIGUI.cpp
)

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
        __APPLE__
        TARGET_OS_IOS=1
        USE_METAL_RENDERER=1
    )
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.floppyturd.game"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "13.0"
        XCODE_ATTRIBUTE_PRODUCT_NAME "FloppyTurd"
        XCODE_ATTRIBUTE_BUNDLE_VERSION "1.0.0"
        XCODE_ATTRIBUTE_SHORT_VERSION_STRING "1.0.0"
        XCODE_ATTRIBUTE_UI_REQUIRED_DEVICE_CAPABILITIES "arm64"
        XCODE_ATTRIBUTE_UI_SUPPORTED_INTERFACE_ORIENTATIONS "UIInterfaceOrientationPortrait;UIInterfaceOrientationLandscapeLeft;UIInterfaceOrientationLandscapeRight"
    )
    
    # Bundle resources
    file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources/*")
    foreach(RESOURCE ${RESOURCES})
        set_source_files_properties(${RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    endforeach()
    
    # Bundle Metal shader files
    if(DEFINED METAL_SHADERS)
        foreach(SHADER ${METAL_SHADERS})
            set_source_files_properties(${SHADER} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endforeach()
        target_sources(${PROJECT_NAME} PRIVATE ${METAL_SHADERS})
    endif()
else()
    # All other platforms use Raylib
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
endif()

# Platform-specific settings
if(${PLATFORM} STREQUAL "Windows")
    # Windows specific settings
    # Use console entry point (main() instead of WinMain())
    
    # Copy resources to output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
    
    # Set Visual Studio debugger working directory
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    
elif(IS_MACOS)
    # macOS specific settings
    message(STATUS "Applying macOS-specific build settings")
    
    # Copy resources to output directory for macOS
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
    
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
    # Copy resources to output directory for other platforms
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 