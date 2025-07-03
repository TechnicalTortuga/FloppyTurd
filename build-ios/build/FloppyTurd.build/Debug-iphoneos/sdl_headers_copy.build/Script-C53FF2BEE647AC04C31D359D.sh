#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build
  /opt/homebrew/bin/cmake -E copy_if_different /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-src/include/SDL_error.h /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build/include/SDL2/SDL_error.h
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build
  /opt/homebrew/bin/cmake -E copy_if_different /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-src/include/SDL_error.h /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build/include/SDL2/SDL_error.h
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build
  /opt/homebrew/bin/cmake -E copy_if_different /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-src/include/SDL_error.h /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build/include/SDL2/SDL_error.h
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build
  /opt/homebrew/bin/cmake -E copy_if_different /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-src/include/SDL_error.h /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/_deps/sdl2-build/include/SDL2/SDL_error.h
fi

