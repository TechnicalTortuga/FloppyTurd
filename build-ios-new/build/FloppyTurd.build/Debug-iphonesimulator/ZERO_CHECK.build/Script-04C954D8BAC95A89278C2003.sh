#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  make -f /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  make -f /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  make -f /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  make -f /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/CMakeScripts/ReRunCMake.make
fi

