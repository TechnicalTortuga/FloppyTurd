#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios-new/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi

