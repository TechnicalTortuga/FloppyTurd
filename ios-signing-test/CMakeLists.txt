cmake_minimum_required(VERSION 3.15)
project(SigningTest VERSION 1.0 LANGUAGES C CXX)

# Set iOS deployment target
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)

# Specify iOS platform and architecture
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_ARCHITECTURES arm64)

# Set development team ID
set(DEVELOPMENT_TEAM "86886N72J8")

# Create executable
add_executable(SigningTest MACOSX_BUNDLE main.mm)

# Link iOS frameworks
target_link_libraries(SigningTest "-framework UIKit" "-framework Foundation")

# Set app bundle properties
set_target_properties(SigningTest PROPERTIES
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.floppyturd.game"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.floppyturd.game"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "13.0"
    XCODE_ATTRIBUTE_PRODUCT_NAME "SigningTest"
    XCODE_ATTRIBUTE_BUNDLE_VERSION "1.0.0"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_UI_REQUIRED_DEVICE_CAPABILITIES "arm64"
    XCODE_ATTRIBUTE_UI_SUPPORTED_INTERFACE_ORIENTATIONS "UIInterfaceOrientationPortrait"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
)

# Add post-build cleanup script
add_custom_command(TARGET SigningTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning extended attributes and fixing permissions on app bundle..."
    # Run a check first to determine which build configuration directory exists
    COMMAND bash -c "APP_PATH=''; if [ -d \"${CMAKE_BINARY_DIR}/Debug-iphoneos/${PROJECT_NAME}.app\" ]; then APP_PATH='${CMAKE_BINARY_DIR}/Debug-iphoneos/${PROJECT_NAME}.app'; elif [ -d \"${CMAKE_BINARY_DIR}/Debug-iphonesimulator/${PROJECT_NAME}.app\" ]; then APP_PATH='${CMAKE_BINARY_DIR}/Debug-iphonesimulator/${PROJECT_NAME}.app'; elif [ -d \"${CMAKE_BINARY_DIR}/Release-iphoneos/${PROJECT_NAME}.app\" ]; then APP_PATH='${CMAKE_BINARY_DIR}/Release-iphoneos/${PROJECT_NAME}.app'; elif [ -d \"${CMAKE_BINARY_DIR}/Release-iphonesimulator/${PROJECT_NAME}.app\" ]; then APP_PATH='${CMAKE_BINARY_DIR}/Release-iphonesimulator/${PROJECT_NAME}.app'; fi; if [ -n \"$APP_PATH\" ]; then echo \"Cleaning $APP_PATH\"; xattr -cr \"$APP_PATH\"; chmod -x \"$APP_PATH/embedded.mobileprovision\" 2>/dev/null || echo \"No embedded.mobileprovision found, skipping\"; dot_clean \"$APP_PATH\"; codesign -dvvv \"$APP_PATH\" || echo \"App is not signed or has signing issues\"; else echo \"No app bundle found\"; fi"
    COMMENT "Cleaning extended attributes and fixing permissions to ensure proper code signing"
)
