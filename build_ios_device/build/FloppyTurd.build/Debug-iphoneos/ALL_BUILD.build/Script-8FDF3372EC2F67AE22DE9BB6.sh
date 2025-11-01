#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/aimac/Development/FloppyTurd/build_ios_device
  echo Build\ all\ projects
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/aimac/Development/FloppyTurd/build_ios_device
  echo Build\ all\ projects
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/aimac/Development/FloppyTurd/build_ios_device
  echo Build\ all\ projects
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/aimac/Development/FloppyTurd/build_ios_device
  echo Build\ all\ projects
fi

