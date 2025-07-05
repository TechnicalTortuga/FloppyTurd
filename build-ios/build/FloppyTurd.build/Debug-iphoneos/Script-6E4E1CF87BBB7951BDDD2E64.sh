#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Cleaning\ extended\ attributes\ to\ fix\ iOS\ code\ signing...
  xattr -cr /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  echo ✅\ Extended\ attributes\ cleaned\ from\ app\ bundle
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Final\ xattr\ cleanup\ before\ CodeSign...
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Debug${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Cleaning\ extended\ attributes\ to\ fix\ iOS\ code\ signing...
  xattr -cr /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  echo ✅\ Extended\ attributes\ cleaned\ from\ app\ bundle
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Final\ xattr\ cleanup\ before\ CodeSign...
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/Release${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Cleaning\ extended\ attributes\ to\ fix\ iOS\ code\ signing...
  xattr -cr /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  echo ✅\ Extended\ attributes\ cleaned\ from\ app\ bundle
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Final\ xattr\ cleanup\ before\ CodeSign...
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/MinSizeRel${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Cleaning\ extended\ attributes\ to\ fix\ iOS\ code\ signing...
  xattr -cr /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  echo ✅\ Extended\ attributes\ cleaned\ from\ app\ bundle
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  echo 🧹\ Final\ xattr\ cleanup\ before\ CodeSign...
  dot_clean -m /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app 2>/dev/null || true
  find /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app -exec xattr -c {} + 2>/dev/null || true
  cd /Users/aimac/Documents/GitHub/FloppyTurd/build-ios
  /opt/homebrew/bin/cmake -E copy_directory /Users/aimac/Documents/GitHub/FloppyTurd/FloppyTurd/resources /Users/aimac/Documents/GitHub/FloppyTurd/build-ios/RelWithDebInfo${EFFECTIVE_PLATFORM_NAME}/FloppyTurd.app/resources
fi

