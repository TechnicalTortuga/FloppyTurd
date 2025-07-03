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
    # iOS uses SDL2 instead of Raylib
    include(FetchContent)
    
    # Disable SDL2's main handling before fetching SDL2
    set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "Disable SDL2 main function" FORCE)
    
    # Fetch SDL2
    FetchContent_Declare(
        sdl2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sdl2)
    
    # Fetch SDL2_image
    FetchContent_Declare(
        sdl2_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG release-2.6.3
        GIT_SHALLOW TRUE
    )
    set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(sdl2_image)
    
    # Disable all optional codecs to avoid dependency issues
    set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_OPUS OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_MOD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_FLAC OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_MIDI OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_GME OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_GME OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MP3 OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_MP3 OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MPG123 OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_MPG123 OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MIKMOD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_MIKMOD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_SMPEG OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_SMPEG OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MAD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_MAD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_ID3TAG OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_ID3TAG OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_FLUIDSYNTH OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_FLUIDSYNTH OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_ADLMIDI OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_ADLMIDI OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_OPNMIDI OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_OPNMIDI OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_TIMIDITY OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_TIMIDITY OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MOD_MODPLUG OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VENDORED_MOD_MODPLUG OFF CACHE BOOL "" FORCE)

    # Fetch SDL2_mixer
    FetchContent_Declare(
        sdl2_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG release-2.6.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sdl2_mixer)
    
    # Fetch SDL2_ttf
    FetchContent_Declare(
        sdl2_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-2.20.2
        GIT_SHALLOW TRUE
    )
    
    # Disable SDL2_ttf example/test apps
    set(SDL2TTF_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_INSTALL_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
    
    # Disable SDL2_image example programs
    set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
    
    # Disable SDL2_mixer example programs
    set(SDL2MIXER_SAMPLES OFF CACHE BOOL "" FORCE)
    
    # Force SDL2_ttf to use vendored freetype for iOS
    set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
    set(SDL2TTF_FREETYPE ON CACHE BOOL "" FORCE)
    set(SDL2TTF_VENDORED_FREETYPE ON CACHE BOOL "" FORCE)
    
    # Disable system freetype to prevent linking macOS libraries
    set(FREETYPE_FOUND OFF CACHE BOOL "" FORCE)
    set(FREETYPE_LIBRARY "" CACHE STRING "" FORCE)
    set(FREETYPE_INCLUDE_DIRS "" CACHE STRING "" FORCE)
    
    FetchContent_MakeAvailable(sdl2_ttf)
endif()

# Collect source files
file(GLOB_RECURSE SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.cpp
)

# Remove PlayerOLD.cpp from sources
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/PlayerOLD.cpp)

# Add RaylibCompat for all platforms (conditional compilation inside)
list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/RaylibCompat.cpp)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/PlatformLayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/TouchControls.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/ResourceManager.cpp
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
    # iOS uses SDL2 instead of Raylib
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        SDL2::SDL2
        SDL2_image
        SDL2_mixer
        SDL2_ttf
        "-framework Foundation"
        "-framework UIKit"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AVFoundation"
        "-framework GameController"
        "-framework OpenGLES"
        "-framework ImageIO"
        "-framework MobileCoreServices"
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        PLATFORM_IOS
        __APPLE__
        TARGET_OS_IOS=1
        SDL_MAIN_HANDLED=1
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