cmake_minimum_required(VERSION 3.20)
project(FloppyTurd VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options for different platforms
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(SUPPORT_HIGHDPI "Support high DPI displays" ON)

# Platform detection
if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(PLATFORM "Android")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(PLATFORM "iOS")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(PLATFORM "Desktop")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(PLATFORM "macOS")
else()
    set(PLATFORM "Desktop")
endif()

message(STATUS "Building for platform: ${PLATFORM}")

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

# Collect source files
file(GLOB_RECURSE SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.cpp
)

# Remove PlayerOLD.cpp from sources
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/PlayerOLD.cpp)

file(GLOB_RECURSE HEADERS 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/*.h
)

# Remove the GameSettings.h from sources (it's header only)
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
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Platform-specific settings
if(${PLATFORM} STREQUAL "Windows")
    # Windows specific settings
    # Removed WIN32_EXECUTABLE to use console entry point (main() instead of WinMain())
    
    # Copy resources to output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
    
    # Set Visual Studio debugger working directory
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    
elseif(${PLATFORM} STREQUAL "Android")
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
    
elseif(${PLATFORM} STREQUAL "iOS")
    # iOS specific settings
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    )
    
    # Bundle resources
    file(GLOB_RECURSE RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources/*")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RESOURCE "${RESOURCES}"
    )
endif()

# Copy resources for desktop builds
if(NOT ${PLATFORM} STREQUAL "Android" AND NOT ${PLATFORM} STREQUAL "iOS")
    # Create resources directory in build folder
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources 
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 