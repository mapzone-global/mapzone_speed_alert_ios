Pod::Spec.new do |spec|
  spec.name         = "MapZoneSpeedAlertSDK"
  spec.version      = "0.0.6"
  spec.summary      = "MapZone Speed Alert SDK – GPS tracking and real-time speed alert for iOS."
  spec.description  = <<-DESC
    MapZoneSpeedAlertSDK provides GPS tracking and speed alert functionality with a
    shared C++ core. Features include real-time speed limit detection, camera alerts,
    toll booth detection, route progress tracking, and TTS announcements.
  DESC

  spec.homepage     = "https://github.com/mapzone-global/mapzone_speed_alert_ios"
  spec.license      = { :type => "MIT", :file => "LICENSE" }
  spec.author       = { "Vietmap" => "support@vietmap.vn" }

  spec.platform              = :ios, "12.0"
  spec.ios.deployment_target = "12.0"
  spec.swift_version         = "5.0"

  # Tag format: ios-1.0.0 (separate from Android tags)
  spec.source = {
    :git => "https://github.com/mapzone-global/mapzone_speed_alert_ios.git",
    :tag => "ios-#{spec.version}"
  }

  # Prebuilt XCFramework – path relative to this file (ios/ directory)
  spec.vendored_frameworks = "MapZoneSpeedAlertSDK.xcframework"

  spec.frameworks   = ["Foundation", "CoreLocation", "UIKit"]
  spec.requires_arc = true

  spec.pod_target_xcconfig = {
    "CLANG_CXX_LANGUAGE_STANDARD"                           => "c++17",
    "CLANG_CXX_LIBRARY"                                     => "libc++",
    "DEFINES_MODULE"                                        => "YES",
    "CLANG_ALLOW_NON_MODULAR_INCLUDES_IN_FRAMEWORK_MODULES" => "YES"
  }

  spec.user_target_xcconfig = {
    "CLANG_CXX_LANGUAGE_STANDARD" => "c++17",
    "CLANG_CXX_LIBRARY"           => "libc++"
  }
end
