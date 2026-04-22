# MapZoneSpeedAlertSDK – iOS Integration (Graph Engine V2)

This SDK is provided only for Vietmap MAPs API enterprise customers. Contact your Vietmap account manager for access or [Vietmap Solutions](https://zalo.me/3189066936017422854) Zalo OA if you are interested in becoming a customer.


**This version is under development and will deprecate soon. For the stable version, please switch to the latest version branch or contact Vietmap support.**

## Requirements

| | Minimum |
|---|---|
| iOS | 12.0+ |
| Xcode | 14.0+ |
| Swift | 5.0+ |

---

## Installation

```ruby
# Podfile
pod 'MapZoneSpeedAlertSDK'
```

```bash
pod install
```

---

## Setup – Info.plist

```xml
<key>NSLocationWhenInUseUsageDescription</key>
<string>Used for real-time speed alerts.</string>

<key>NSLocationAlwaysAndWhenInUseUsageDescription</key>
<string>Used for speed alerts in the background.</string>

<key>UIBackgroundModes</key>
<array>
    <string>location</string>
</array>
```

---

## Quick Integration — `ZoneNetworkManagerV2`

`ZoneNetworkManagerV2` is the recommended high-level entry point.
It handles zone data fetching, graph loading, GPS pipeline, bitmaps, and voice in one object.

### 1. Create and configure

```swift
import MapZoneSpeedAlertSDK

let zoneManager = ZoneNetworkManagerV2(
    baseUrl:     "https://dev.fastmap.vn/drivingalert",
    vehicleType: 1,      // see Vehicle Types table below
    seats:       4,
    weights:     1500
)
```

### 2. Register callbacks

```swift
// Fires once after zone data loads (linkCount, alertCount)
zoneManager.onReady = { linkCount, alertCount in
    print("Graph ready: \(linkCount) links, \(alertCount) alerts")
}

// Fires on every GPS frame that produces a sign update
zoneManager.onBitmap = {
    currentSignImage, speedStatus,
    nextSignImage,    nextDistMeters,
    cameraImage,      cameraDistMeters,
    tollImage,        tollDistMeters,
    voiceWavData in

    // --- Current speed limit sign ---
    self.speedSignView.image = currentSignImage

    // --- Overspeed indicator ---
    // speedStatus: 0 = safe, 1 = near limit, 2 = over limit
    switch speedStatus {
    case 1:  self.statusBar.backgroundColor = .systemYellow
    case 2:  self.statusBar.backgroundColor = .systemRed
    default: self.statusBar.backgroundColor = .clear
    }

    // --- Upcoming sign ---
    self.nextSignView.image = nextSignImage
    self.nextDistLabel.text = nextSignImage != nil ? "\(nextDistMeters)m" : ""

    // --- Speed camera ---
    self.cameraView.image = cameraImage
    self.cameraLabel.text = cameraImage != nil ? "\(cameraDistMeters)m" : ""

    // --- Toll gate ---
    self.tollView.image = tollImage
    self.tollLabel.text = tollImage != nil ? "\(tollDistMeters)m" : ""

    // --- Voice (WAV PCM 16-bit, 22 050 Hz mono) ---
    if let wav = voiceWavData {
        self.playWav(wav)
    }
}

// Additional queued voice clips (e.g. when two events fire close together)
zoneManager.onVoice = { wav in
    self.playWav(wav)
}
```

### 3. Feed GPS updates

```swift
// Step A — update zone cache (call on a background queue; blocks on HTTP if zone changed)
DispatchQueue.global(qos: .utility).async {
    zoneManager.updateLocation(lat: lat, lng: lng)
}

// Step B — process GPS frame (call on any queue; delivers callbacks on main thread)
zoneManager.processGps(
    lat:      lat,
    lng:      lng,
    bearing:  heading,   // degrees, 0–360, clockwise from north
    speedKmh: speed,     // km/h
    accuracy: accuracy   // meters
)
```

Typical call site inside a `CLLocationManagerDelegate`:

```swift
func locationManager(_ manager: CLLocationManager,
                     didUpdateLocations locations: [CLLocation]) {
    guard let loc = locations.last else { return }

    let lat      = loc.coordinate.latitude
    let lng      = loc.coordinate.longitude
    let bearing  = loc.course >= 0 ? loc.course : 0
    let speedKmh = max(loc.speed, 0) * 3.6
    let accuracy = loc.horizontalAccuracy

    // Zone cache check (background)
    DispatchQueue.global(qos: .utility).async {
        self.zoneManager.updateLocation(lat: lat, lng: lng)
    }

    // GPS pipeline (immediate)
    zoneManager.processGps(lat: lat, lng: lng, bearing: bearing,
                           speedKmh: speedKmh, accuracy: accuracy)
}
```

### 4. Reset (e.g. on new route or app resume)

```swift
zoneManager.reset()
```

---

## Vehicle Types

| Value | Type |
|---|---|
| 1 | Car |
| 2 | Taxi |
| 3 | Bus |
| 4 | Coach |
| 5 | Truck |
| 6 | Trailer |
| 7 | Bicycle |
| 8 | Motorbike |
| 9 | Pedestrian |
| 10 | Semi-trailer |


Currently, we only support `vehicleType` 3, 5, 6, (truck and trailer) for speed alerts. If you want to use more types, consider using our [Vietmap Live](https://vietmap.vn/vietmap-live) application, which has a built-in speed alert feature.

## BMP Decoding Helper

iOS `UIImage(data:)` does not support BMP alpha. Use this helper:

```swift
func imageFromBmpData(_ data: Data) -> UIImage? {
    guard data.count > 54,
          data[0] == 0x42, data[1] == 0x4D else { return nil }   // "BM"

    let pixelOffset = Int(data[10]) | (Int(data[11]) << 8)
    let width       = Int(data[18]) | (Int(data[19]) << 8)
    let height      = Int(data[22]) | (Int(data[23]) << 8)
    guard width > 0, height > 0 else { return nil }

    let rowBytes = width * 4
    var rgba = [UInt8](repeating: 0, count: width * height * 4)

    for row in 0..<height {
        let srcRow  = height - 1 - row        // BMP is stored bottom-up
        let srcBase = pixelOffset + srcRow * rowBytes
        let dstBase = row * rowBytes
        guard srcBase + rowBytes <= data.count else { continue }
        for col in 0..<width {
            let s = srcBase + col * 4
            let d = dstBase + col * 4
            rgba[d]   = data[s + 2]   // R  (BMP BGRA → RGBA)
            rgba[d+1] = data[s + 1]   // G
            rgba[d+2] = data[s]       // B
            rgba[d+3] = data[s + 3]   // A
        }
    }

    guard let provider = CGDataProvider(data: Data(rgba) as CFData),
          let cgImage  = CGImage(
              width:            width,
              height:           height,
              bitsPerComponent: 8,
              bitsPerPixel:     32,
              bytesPerRow:      rowBytes,
              space:            CGColorSpaceCreateDeviceRGB(),
              bitmapInfo:       CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue),
              provider:         provider,
              decode:           nil,
              shouldInterpolate: true,
              intent:           .defaultIntent
          ) else { return nil }

    return UIImage(cgImage: cgImage)
}
```
