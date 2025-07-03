# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-src")
  file(MAKE_DIRECTORY "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-src")
endif()
file(MAKE_DIRECTORY
  "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-build"
  "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-subbuild/sdl2-populate-prefix"
  "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-subbuild/sdl2-populate-prefix/tmp"
  "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-subbuild/sdl2-populate-prefix/src/sdl2-populate-stamp"
  "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-subbuild/sdl2-populate-prefix/src"
  "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-subbuild/sdl2-populate-prefix/src/sdl2-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-subbuild/sdl2-populate-prefix/src/sdl2-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/_deps/sdl2-subbuild/sdl2-populate-prefix/src/sdl2-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
