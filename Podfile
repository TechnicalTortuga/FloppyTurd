# Podfile for FloppyTurd
# Google Mobile Ads SDK integration via CocoaPods

platform :ios, '13.0'

# Disable module headers to avoid conflicts with CMake
use_modular_headers!

# Specify the Xcode project
project 'build_ios/FloppyTurd.xcodeproj'

target 'FloppyTurd' do
  # Google Mobile Ads SDK
  pod 'Google-Mobile-Ads-SDK'

  # User Messaging Platform (required for GDPR/privacy compliance)
  pod 'GoogleUserMessagingPlatform'
end

# Post-install hook to configure build settings
post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      # Set iOS deployment target
      config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '13.0'

      # Enable C++17 standard
      config.build_settings['CLANG_CXX_LANGUAGE_STANDARD'] = 'c++17'
      config.build_settings['CLANG_CXX_LIBRARY'] = 'libc++'

      # Disable bitcode (not needed for iOS 14+)
      config.build_settings['ENABLE_BITCODE'] = 'NO'

      # Enable ARC
      config.build_settings['CLANG_ENABLE_OBJC_ARC'] = 'YES'

      # Swift version
      config.build_settings['SWIFT_VERSION'] = '6.0'
    end
  end
end
