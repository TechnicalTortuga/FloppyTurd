cmake_minimum_required(VERSION 3.20)
project(FloppyTurd VERSION 0.9.2 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(FLOPPY_BUILD_TESTS "Build save-format and viewport regression checks" ON)
if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
else()
    set(PLATFORM Desktop CACHE STRING "raylib platform" FORCE)
endif()
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 02566453880e06bec97f5b8160fa6fe1e98397ca)
FetchContent_MakeAvailable(raylib)
# Link raylib into the game without installing its SDK into player packages.
set_property(DIRECTORY "${raylib_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)
if(EMSCRIPTEN)
    # raylib 6.2 exports only ccall by default; the application owns the complete
    # export list so transitive link flags cannot overwrite the browser bridge.
    get_target_property(RAYLIB_WEB_OPTIONS raylib INTERFACE_LINK_OPTIONS)
    list(FILTER RAYLIB_WEB_OPTIONS EXCLUDE REGEX "^-sEXPORTED_RUNTIME_METHODS=")
    set_property(TARGET raylib PROPERTY INTERFACE_LINK_OPTIONS ${RAYLIB_WEB_OPTIONS})
endif()
file(GLOB SOURCES CONFIGURE_DEPENDS "FloppyTurd/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "PlayerOLD[.]cpp$")
add_executable(FloppyTurd ${SOURCES})
target_include_directories(FloppyTurd PRIVATE FloppyTurd)
target_link_libraries(FloppyTurd PRIVATE raylib)
if(MSVC)
    target_compile_options(FloppyTurd PRIVATE /W3 /utf-8)
else()
    target_compile_options(FloppyTurd PRIVATE -Wall -Wextra)
endif()
if(MINGW)
    target_link_options(FloppyTurd PRIVATE -static)
endif()
# Package only runtime assets, preserving case and relative paths.
file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/FloppyTurd/resources"
    "FloppyTurd/resources/*.png" "FloppyTurd/resources/*.fnt"
    "FloppyTurd/resources/*.ttf" "FloppyTurd/resources/*.ogg"
    "FloppyTurd/resources/*.mp3" "FloppyTurd/resources/*.wav" "FloppyTurd/resources/*.ico")
set(RUNTIME_ASSETS "${CMAKE_CURRENT_BINARY_DIR}/runtime/resources")
foreach(ASSET IN LISTS ASSETS)
    get_filename_component(DIR "${ASSET}" DIRECTORY)
    file(MAKE_DIRECTORY "${RUNTIME_ASSETS}/${DIR}")
    configure_file("FloppyTurd/resources/${ASSET}" "${RUNTIME_ASSETS}/${ASSET}" COPYONLY)
endforeach()
if(EMSCRIPTEN)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS tools/prepare-web-assets.py web/asset-loader.js web/bridge.js)
    execute_process(COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare-web-assets.py"
        --source "${RUNTIME_ASSETS}" --output "${CMAKE_CURRENT_BINARY_DIR}"
        COMMAND_ERROR_IS_FATAL ANY)
    file(READ web/asset-loader.js ASSET_LOADER_JS)
    file(READ web/bridge.js BRIDGE_JS)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/bridge.js" "${ASSET_LOADER_JS}\n${BRIDGE_JS}")
    set_target_properties(FloppyTurd PROPERTIES OUTPUT_NAME index SUFFIX ".html")
    set_property(TARGET FloppyTurd APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/asset-catalog.js" "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
    target_compile_options(FloppyTurd PRIVATE -fexceptions)
    target_link_options(FloppyTurd PRIVATE
        -fexceptions -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=134217728
        -sSTACK_SIZE=2097152 -sFORCE_FILESYSTEM=1
        "-sEXPORTED_FUNCTIONS=['_main','_floppy_set_paused','_floppy_resize','_floppy_safe_area','_floppy_orientation','_floppy_touch','_floppy_key','_floppy_pointer','_floppy_reset_input','_floppy_set_presentation','_floppy_activate_audio']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','FS','callMain','HEAPF32']"
        "--pre-js=${CMAKE_CURRENT_BINARY_DIR}/asset-catalog.js"
        "--preload-file=${CMAKE_CURRENT_BINARY_DIR}/bootstrap/resources@/resources"
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
else()
    add_custom_command(TARGET FloppyTurd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${RUNTIME_ASSETS}" "$<TARGET_FILE_DIR:FloppyTurd>/resources")
    set_property(TARGET FloppyTurd PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:FloppyTurd>")
    install(TARGETS FloppyTurd RUNTIME DESTINATION .)
    install(DIRECTORY "${RUNTIME_ASSETS}" DESTINATION .)
    set(CPACK_PACKAGE_NAME FloppyTurd)
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_GENERATOR ZIP)
    include(CPack)
endif()
if(FLOPPY_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()
    add_executable(FloppyTurdChecks tests/regression.cpp FloppyTurd/GameStats.cpp FloppyTurd/PlatformLayer.cpp)
    target_include_directories(FloppyTurdChecks PRIVATE FloppyTurd)
    target_link_libraries(FloppyTurdChecks PRIVATE raylib)
    add_test(NAME regression COMMAND FloppyTurdChecks)
    add_executable(FloppyTurdLayoutChecks tests/layout_regression.cpp)
    target_include_directories(FloppyTurdLayoutChecks PRIVATE FloppyTurd)
    target_link_libraries(FloppyTurdLayoutChecks PRIVATE raylib)
    add_test(NAME layout_regression COMMAND FloppyTurdLayoutChecks)
    add_executable(FloppyTurdIosPortChecks tests/ios_port_regression.cpp FloppyTurd/GameStats.cpp FloppyTurd/PlatformLayer.cpp)
    target_include_directories(FloppyTurdIosPortChecks PRIVATE FloppyTurd)
    target_link_libraries(FloppyTurdIosPortChecks PRIVATE raylib)
    add_test(NAME ios_port_regression COMMAND FloppyTurdIosPortChecks)
    set_tests_properties(ios_port_regression PROPERTIES ENVIRONMENT "FLOPPY_TURD_SAVE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-ios-port-saves")
    set_tests_properties(regression PROPERTIES ENVIRONMENT "FLOPPY_TURD_SAVE_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-saves")
endif()
