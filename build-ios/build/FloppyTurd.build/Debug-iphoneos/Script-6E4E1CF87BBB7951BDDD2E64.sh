#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug-iphoneos/Debug/resources
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug-iphoneos/Release/resources
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug-iphoneos/MinSizeRel/resources
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug-iphoneos/RelWithDebInfo/resources
fi

