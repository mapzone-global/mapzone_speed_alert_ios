# MapZoneSpeedAlertSDK – iOS Integration

This SDK is provided only for Vietmap MAPs API enterprise customers. Contact your Vietmap account manager for access or [Vietmap Solutions](https://zalo.me/3189066936017422854) Zalo OA if you are interested in becoming a customer.

---

## Installation

### CocoaPods (recommended)

Add to your `Podfile`:

```ruby
platform :ios, '12.0'
use_frameworks!

target 'YourApp' do
  pod 'MapZoneSpeedAlertSDK', '~> <lasted-version>'
end
```

Then install:

```bash
pod install
```

> **Note:** The pod vendors a prebuilt XCFramework — no source compilation, no NDK / Swift toolchain pinning. Open the generated `.xcworkspace` from now on.

---

## Permissions — `Info.plist`

Add the following keys. Strings are shown to the user in the system permission prompt — keep them short and explain *why* you need each.

```xml
<key>NSLocationWhenInUseUsageDescription</key>
<string>Used for real-time speed limit and camera alerts while you drive.</string>

<key>NSLocationAlwaysAndWhenInUseUsageDescription</key>
<string>Used for speed alerts when the app is in the background.</string>

<key>UIBackgroundModes</key>
<array>
    <string>location</string>
    <string>audio</string>
</array>
```

> **Note:** `audio` background mode is only required if you need voice cues to keep playing when the screen is locked. Drop it if your app foregrounds for navigation.

Request authorization before instantiating the engine:

```swift
import CoreLocation

let locationManager = CLLocationManager()
locationManager.requestWhenInUseAuthorization()
// Switch to requestAlwaysAuthorization() if you need background updates.
```

---

## Quick Integration — `ZoneNetworkManager`

`ZoneNetworkManager` is the only public entry point. It owns the engine and serialises all GPS / network work on an internal background queue — the public API is safe to call from the main thread.

### 1. Create and configure

```swift
import MapZoneSpeedAlertSDK

let zoneManager = ZoneNetworkManager(
    baseUrl:     "https://driving.map.zone",         // endpoint
    apiKeyId:    "<your-api-key-id>",                // issued by Vietmap
    apiKey:      "<your-api-key>",                   // issued by Vietmap
    bundleId:    Bundle.main.bundleIdentifier ?? "",
    vehicleId:   "<your-vehicle-id>",                // issued by Vietmap
    vehicleType: 1,        // see Vehicle Types table below
    seats:       4,        // used by the server to filter applicable alerts
    weights:     1500      // gross weight in kg
)
```

> **Note:** Treat `apiKey` as a credential. Do not log it, embed it in a public repo, or expose it in user-facing UI. The constructor throws an Objective-C exception with name `"InvalidArgument"` if `baseUrl` does not start with `https://`.

### 2. Register callbacks

All four callbacks fire on the main thread. Wire only the ones you need.

```swift
// (a) Engine ready — fires after each successful zone load.
zoneManager.onReady = { linkCount, alertCount in
    print("ready: \(linkCount) links, \(alertCount) alerts")
}

// (b) Network result — surfaces server errors. errorCode == 0 means success.
zoneManager.onResult = { success, errorCode, errorMessage in
    if !success && errorCode != 0 {
        print("error code=\(errorCode): \(errorMessage)")
    }
}

// (c) Per-tick bitmaps + first voice clip — the core driver of your UI.
zoneManager.onBitmap = { currentImage, speedStatus,
                        nextImage,   nextDistMeters,
                        cameraImage, cameraDistMeters,
                        tollImage,   tollDistMeters,
                        voiceWavData in

    // Current speed-limit sign (nil = matcher has no link under GPS).
    self.currentSpeedSign.image = currentImage

    // Overspeed indicator: 0 = safe, 1 = approaching limit, 2 = over limit.
    switch speedStatus {
    case 1:  self.statusBar.tintColor = .systemOrange
    case 2:  self.statusBar.tintColor = .systemRed
    default: self.statusBar.tintColor = .systemGreen
    }

    // Upcoming sign / camera / toll. Containers should be hidden when distance == 0.
    self.nextSignView.image  = nextImage
    self.nextSignLabel.text  = nextImage  != nil ? "\(nextDistMeters)m"   : ""
    self.cameraView.image    = cameraImage
    self.cameraLabel.text    = cameraImage != nil ? "\(cameraDistMeters)m" : ""
    self.tollView.image      = tollImage
    self.tollLabel.text      = tollImage   != nil ? "\(tollDistMeters)m"   : ""

    // First voice clip of the tick (WAV PCM 16-bit LE, mono, 22050 Hz).
    // NOTE: voiceWavData is the FIRST clip only, and is non-nil ONLY when
    // `onVoice` is not set. Once `onVoice` is wired, every clip (including the
    // first) is delivered through `onVoice` and voiceWavData is always nil —
    // so there is no double playback.
    if let wav = voiceWavData {
        self.enqueueVoice(wav, priority: 1)
    }
}

// (d) Voice — receives EVERY clip of the tick, each tagged with its trigger
// and priority. The engine already orders them (priority desc, then nearest
// first), so arrival order is the order you should speak them in.
//   trigger : alert type — see the Voice Cues & Priority table below
//   priority: 0 = "current" (lowest, drop first when congested)
//             1 = normal (approaching / camera / sign)
//             2 = speeding (most urgent)
zoneManager.onVoice = { wav, trigger, priority in
    self.enqueueVoice(wav, priority: priority)
}
```

> **Important — iOS has no built-in voice player.** Unlike the Android SDK, the iOS engine never plays audio for you. Every WAV delivered via `onBitmap` (`voiceWavData`) or `onVoice` must be handled by the host app, or the cue is dropped. A minimal priority-aware sequential player — it inserts by priority (highest first) and keeps arrival order (the engine's travel order) within the same priority, so urgent cues like "speeding" are spoken before stale "current speed limit" ones:
>
> ```swift
> import AVFoundation
>
> private var voiceQueue: [(data: Data, priority: Int)] = []
> private var audioPlayer: AVAudioPlayer?
> private var playing = false
>
> func enqueueVoice(_ data: Data, priority: Int) {
>     // Insert before the first clip of strictly lower priority → stable within a tier.
>     let idx = voiceQueue.firstIndex { $0.priority < priority } ?? voiceQueue.count
>     voiceQueue.insert((data, priority), at: idx)
>     if !playing { playNext() }
> }
>
> private func playNext() {
>     guard !voiceQueue.isEmpty else { playing = false; return }
>     let url = URL(fileURLWithPath: NSTemporaryDirectory())
>         .appendingPathComponent("mzv_\(Int.random(in: 1000...9999)).wav")
>     try? voiceQueue.removeFirst().data.write(to: url)
>     audioPlayer = try? AVAudioPlayer(contentsOf: url)
>     audioPlayer?.delegate = self          // implement audioPlayerDidFinishPlaying -> playNext()
>     audioPlayer?.play()
>     playing = true
> }
> ```
>
> The single-clip `onBitmap` path (legacy hosts that never wire `onVoice`) maps cleanly onto the same queue with `priority: 1`.

### 3. Feed GPS updates

Call both APIs on every GPS frame. The engine throttles HTTP fetches internally — calling at 1 Hz when the vehicle hasn't moved costs nothing.

```swift
func locationManager(_ manager: CLLocationManager,
                     didUpdateLocations locations: [CLLocation]) {
    guard let loc = locations.last else { return }

    let lat      = loc.coordinate.latitude
    let lng      = loc.coordinate.longitude
    let bearing  = loc.course >= 0 ? loc.course : 0        // -1 when stationary
    let speedKmh = max(loc.speed, 0) * 3.6                 // CoreLocation gives m/s
    let accuracy = loc.horizontalAccuracy

    // (a) Refresh zone cache. Returns immediately; HTTP happens on a background queue.
    zoneManager.updateLocation(lat: lat, lng: lng,
                               speedKmh: speedKmh, bearingDeg: bearing)

    // (b) Drive the per-tick UI: bitmaps, speed status, voice cues.
    zoneManager.processGps(lat: lat, lng: lng, bearing: bearing,
                           speedKmh: speedKmh, accuracy: accuracy)
}
```

> **Note:** Pass *snapped-to-route* coordinates here if your app already runs map-matched navigation — raw CoreLocation samples can be 10-20 m off the road centre even with good `horizontalAccuracy`, which causes the matcher to pick the wrong link.

### 4. Reset

```swift
zoneManager.reset()    // clears loaded zone data; next updateLocation() refetches
```

Call this when the driver swaps `vehicleType` / `seats` / `weights`, or at the end of a navigation session if you want to free engine memory immediately.

---

## Vehicle Types

Pass the integer `value` column to the `vehicleType:` initialiser argument.

| Value | Type | Notes |
|---|---|---|
| 1 | Car | |
| 2 | Motorcycle | |
| 3 | Truck | Supported for speed alerts |
| 4 | Coach | |
| 5 | Bus | |
| 6 | Taxi | |
| 7 | Bicycle | |
| 8 | Pedestrian | |
| 9 | Emergency | |

---

## Error Codes (`onResult`)

| Code | Meaning |
|---|---|
| `0` | Success (zone loaded) |
| `1001` | Invalid parameter (check `vehicleId`, `seats`, `weights`) |
| `2003` | Unauthorized — `apiKeyId` / `apiKey` / `bundleId` mismatch |
| `3003` | Vehicle type not supported for this account |
| negative values | Local SDK failure (network unreachable, parse error) |

---

## Threading Model

```
 Main thread ──► updateLocation / processGps
                          │
                          ▼
              Internal serial DispatchQueue
              (HTTP + matching + bitmap render)
                          │
                          ▼
              Main thread ◄── onReady / onResult / onBitmap / onVoice
```

All public methods are safe to call from the main thread. The callbacks always arrive back on the main thread, so you can update UIKit directly without dispatching.

---

## Troubleshooting

- **No bitmaps and `onReady` never fires:** check `onResult` — a non-zero `errorCode` tells you whether it is an auth issue (2003), a parameter issue (1001), or a network failure (negative code).
- **Voice never plays:** verify your `AVAudioPlayer` queue logic — the SDK never plays audio on iOS (see the *Important* note in step 2).
- **Bitmaps appear with a black background instead of transparency:** you are decoding the BMP via `UIImage(data:)`. The SDK already returns proper `UIImage` instances via `onBitmap` — don't re-decode the underlying bytes.
- **Wrong sign shown on the wrong road:** pass map-matched coordinates instead of raw GPS (see step 3 note).

---

## Demo
Check demo app on [Github](https://github.com/mapzone-global/mapzone-speed-alert-app-ios).
