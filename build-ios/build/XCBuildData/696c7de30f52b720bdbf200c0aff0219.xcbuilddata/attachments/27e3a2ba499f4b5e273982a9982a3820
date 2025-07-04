#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi

