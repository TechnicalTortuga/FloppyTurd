cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_MODULE_PATH "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-src/cmake")
project(CheckOBJC OBJC)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/result.cmake"
  "set(CMAKE_OBJC_COMPILER \"${CMAKE_OBJC_COMPILER}\")\n"
  
  )