#ifndef MapZoneGraphBridge_h
#define MapZoneGraphBridge_h

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// ============================================================
//  GPS processing result (plain value object, no raw data to caller)
// ============================================================

/**
 * Result of a single full GPS processing pass (snap + alerts).
 * All distances in metres; speeds in km/h. 0 = none/not applicable.
 */
@interface MapZoneGpsProcessResult : NSObject
@property (nonatomic, assign) BOOL     matched;
@property (nonatomic, assign) NSInteger currentSpeedLimit;
@property (nonatomic, assign) NSInteger nextSpeedLimit;
@property (nonatomic, assign) NSInteger nextDistMeters;
@property (nonatomic, assign) NSInteger cameraDistMeters;
@property (nonatomic, assign) NSInteger tollDistMeters;
/** linkId of the matched road segment (-1 when matched=NO). */
@property (nonatomic, assign) NSInteger matchedLinkId;
/** Snapped GPS position from the HMM matcher (valid only when snapValid=YES). */
@property (nonatomic, assign) double    snapLat;
@property (nonatomic, assign) double    snapLng;
@property (nonatomic, assign) BOOL      snapValid;
/**
 * Primary voice trigger this tick (back-compat). See VoiceTrigger enum
 * values 1..18 in graph_engine_v2.hpp. Use `voiceTriggers` for the full
 * list when multiple variants fire on the same tick.
 */
@property (nonatomic, assign) NSInteger voiceTrigger;
/** Speed value to announce (for trigger 1 and 2). */
@property (nonatomic, assign) NSInteger voiceSpeedValue;
/**
 * Full list of voice triggers fired this tick, in fire order. Each
 * element is `NSInteger` wrapped in `NSNumber`. Empty when no voice
 * fires. Includes the primary trigger as the first element.
 */
@property (nonatomic, copy) NSArray<NSNumber *> *voiceTriggers;
/**
 * Priority for each entry in `voiceTriggers` (same index/order). Each element
 * is an `NSInteger` wrapped in `NSNumber`:
 *   0 = "hiện tại"/current (lowest — skipped first when the queue is congested),
 *   1 = normal (approaching/camera/sign),
 *   2 = speeding (most urgent).
 * The engine already ordered the events (priority desc, then nearest first).
 */
@property (nonatomic, copy) NSArray<NSNumber *> *voicePriorities;
@end

// ============================================================
//  Result types
// ============================================================

/**
 * Result of a single online map-match (v2).
 */
@interface MapZoneSnapResult : NSObject

/** YES if the GPS point was successfully matched to a road. */
@property (nonatomic, assign) BOOL    matched;
/** Link array index in the internal graph. -1 when matched=NO. */
@property (nonatomic, assign) NSInteger linkIdx;
/** Original linkId from source data. */
@property (nonatomic, assign) NSInteger linkId;
/** Travel direction: @"pos" (from→to) or @"neg" (to→from). */
@property (nonatomic, copy)   NSString* dir;
/** Distance from fromNode along link geometry (meters). */
@property (nonatomic, assign) double distAlong;
/** Snapped latitude on the road. */
@property (nonatomic, assign) double snapLat;
/** Snapped longitude on the road. */
@property (nonatomic, assign) double snapLng;
/** Road bearing at the snapped point (degrees, clockwise from north). */
@property (nonatomic, assign) double linkBearing;
/** Perpendicular distance from GPS to road (meters). */
@property (nonatomic, assign) double perpDist;
/** Maximum allowed speed in km/h on this link+direction. */
@property (nonatomic, assign) NSInteger matchedSpeed;
/** YES if direction changed versus the previous snap (U-turn detected). */
@property (nonatomic, assign) BOOL uturnDetected;

@end

/**
 * A single upcoming alert ahead on the road (v2).
 */
@interface MapZoneNextAlert : NSObject

/** Alert ID. */
@property (nonatomic, assign) NSInteger id;
/**
 * Alert category top-nibble (`(category >> 12) & 0xF`), per v4 spec:
 *   1 = Sign
 *   2 = Camera
 *   3 = TrafficLight (reserved, no data yet)
 *   4 = TollPlaza
 *  99 = synthetic speed-limit-change event (isSpeedChange=YES)
 */
@property (nonatomic, assign) NSInteger alertType;
/**
 * Variant (low 12 bits of category). Concrete values listed in the
 * v4 spec (e.g. Sign+0x001=SpeedLimit, Camera+0x003=RedLightCamera).
 */
@property (nonatomic, assign) NSInteger variant;
/** Speed value in km/h (for speed signs and speed-change events). */
@property (nonatomic, assign) NSInteger speed;
/** Distance ahead from current position (meters). */
@property (nonatomic, assign) double distanceAhead;
/** Alert position latitude. */
@property (nonatomic, assign) double lat;
/** Alert position longitude. */
@property (nonatomic, assign) double lng;
/** YES for synthetic speed-limit change events. */
@property (nonatomic, assign) BOOL isSpeedChange;
/** Previous speed limit (only meaningful when isSpeedChange=YES). */
@property (nonatomic, assign) NSInteger fromSpeed;
/** New speed limit (only meaningful when isSpeedChange=YES). */
@property (nonatomic, assign) NSInteger toSpeed;

@end

// ============================================================
//  V3 result type
// ============================================================

/**
 * Outcome of a V3 zone-reload attempt.
 *
 * `errorCode` semantics:
 *
 * | Code   | Meaning                                                            |
 * |--------|--------------------------------------------------------------------|
 * | 0      | Success.                                                           |
 * | 2003   | Unauthorized (expired token, bundleId/vehicle out of scope).       |
 * | 1001   | Invalid parameter.                                                 |
 * | 3003   | Vehicle type not supported.                                        |
 * | < 0    | Local SDK failure (network, processing, parse).                    |
 */
@interface ZoneLoadResult : NSObject
/** YES when fresh zone data was loaded into the engine. */
@property (nonatomic, assign) BOOL       success;
/** See ZoneLoadResult docs for the code table. */
@property (nonatomic, assign) NSInteger  errorCode;
/** Human-readable detail, may be empty on success. */
@property (nonatomic, copy)   NSString  *errorMessage;
@end

// ============================================================
//  Bridge interface
// ============================================================

/**
 * Process-wide singleton bridge to the native MapZone graph engine.
 *
 * This is the low-level entry point. Most apps should use
 * `ZoneNetworkManager` (Swift) instead, which wraps this bridge with
 * a background queue, throttling, and main-thread dispatching.
 *
 * Use this class directly when you need:
 *   - Synchronous control of zone reloads (e.g. driving a custom
 *     background pipeline outside the SDK's queue).
 *   - Native file logging for field tests on debug builds where
 *     Xcode console is unavailable (`setLogFilePath:`).
 *
 * Obtain the singleton with `+sharedInstance`. The instance is lazily
 * created on first call and lives for the process lifetime.
 *
 * All public methods are suffixed with `V2`. Thread-safe: the
 * underlying native engine uses a mutex.
 */
@interface MapZoneGraphBridge : NSObject

+ (instancetype)sharedInstance;

// -------------------------------------------------------
//  Graph loading
// -------------------------------------------------------

/**
 * Add a road link with Google-encoded polyline geometry.
 * Must be called before buildIndex.
 */
- (void)addLinkWithLinkId:(NSInteger)linkId
                 fromNodeId:(NSInteger)fromNodeId
                   toNodeId:(NSInteger)toNodeId
                  roadClass:(NSInteger)roadClass
                     oneway:(NSInteger)oneway
                posMaxSpeed:(NSInteger)posMaxSpeed
                negMaxSpeed:(NSInteger)negMaxSpeed
            encodedGeometry:(NSString *)encodedGeometry;

/**
 * Add an alert associated with a link.
 * Must be called before buildIndex.
 *
 * v4 category scheme (category = alertType<<12 | variant):
 * @param alertType    1=Sign, 2=Camera, 3=TrafficLight (reserved), 4=Toll
 * @param type         the variant (category & 0xFFF), e.g. 0x001=SpeedLimit
 *                     for Sign, 0x001=SpeedCamera for Camera. Passed to the
 *                     engine verbatim as the alert variant.
 * @param isRightOrient YES = POS direction (from→to), NO = NEG
 * @param distance     Meters from fromNode along link geometry
 * @param speed        Speed value in km/h (only for Sign + SpeedLimit variant)
 */
- (void)addAlertWithId:(NSInteger)alertId
               alertType:(NSInteger)alertType
                    type:(NSInteger)type
            affectLinkId:(NSInteger)affectLinkId
           isRightOrient:(BOOL)isRightOrient
                distance:(double)distance
                   speed:(NSInteger)speed;

/**
 * Build spatial + adjacency indices.
 * Must be called after all addLink/addAlert calls and before snap/alert calls.
 */
- (void)buildIndex;

/**
 * Clear all graph data and engine state.
 * After this you must reload links/alerts and call buildIndex again.
 */
- (void)clear;

/**
 * Reset the HMM matcher state (begin new GPS sequence).
 * Graph data and indices are preserved.
 */
- (void)resetMatcher;

/**
 * Set which alert triggers are suppressed. `mask` is a bitmask where bit `i`
 * (the native `VoiceTrigger` enum value) set means that category is muted;
 * `0` announces everything (the default).
 *
 * Muting a camera or toll category also hides its on-screen icon together
 * with its voice (both read this same mask). Speed-limit and speeding bits
 * are force-cleared by the engine, so they can never be muted.
 *
 * Persists across zone reloads and `clear` — it is a host preference,
 * not zone data. See `VoiceAlertType` and
 * `ZoneNetworkManager.setMutedAlertTypes(_:)` for the Swift-facing API.
 */
- (void)setMutedVoiceTriggers:(uint64_t)mask;

// -------------------------------------------------------
//  Snap / Map-match
// -------------------------------------------------------

/**
 * Online map-matching: feed one GPS point at a time.
 *
 * @param lat       Latitude (degrees)
 * @param lng       Longitude (degrees)
 * @param bearing   Device heading 0-360°, clockwise from north
 * @param speed     Speed in m/s
 * @param accuracy  Horizontal accuracy in meters
 * @param timestamp Timestamp in milliseconds
 * @return MapZoneSnapResult — check matched before using other fields
 */
- (MapZoneSnapResult *)snapOnlineWithLat:(double)lat
                                         lng:(double)lng
                                     bearing:(double)bearing
                                       speed:(double)speed
                                    accuracy:(double)accuracy
                                   timestamp:(int64_t)timestamp;

// -------------------------------------------------------
//  Alert lookup
// -------------------------------------------------------

/**
 * Find upcoming alerts ahead of the current matched position.
 *
 * @param linkIdx   linkIdx from MapZoneSnapResult
 * @param dir       @"pos" or @"neg"
 * @param distAlong distAlong from MapZoneSnapResult
 * @param gpsLat    Current GPS latitude
 * @param gpsLng    Current GPS longitude
 * @param maxLinks  How many links ahead to scan (suggested: 5)
 * @return Array of MapZoneNextAlert, sorted by distanceAhead ascending
 */
- (NSArray<MapZoneNextAlert *> *)findNextAlertsWithLinkIdx:(NSInteger)linkIdx
                                                           dir:(NSString *)dir
                                                     distAlong:(double)distAlong
                                                        gpsLat:(double)gpsLat
                                                        gpsLng:(double)gpsLng
                                                      maxLinks:(NSInteger)maxLinks;

/**
 * Get current speed limit at matched position.
 * Returns speed in km/h, or 0 if unavailable.
 */
- (NSInteger)getCurrentSpeedWithLinkIdx:(NSInteger)linkIdx
                                      dir:(NSString *)dir
                                distAlong:(double)distAlong;

// -------------------------------------------------------
//  Diagnostics
// -------------------------------------------------------

/** Number of links loaded in the graph. */
- (NSInteger)getLinkCount;
/** Number of alerts loaded in the graph. */
- (NSInteger)getAlertCount;
/** YES if buildIndex has been called and the engine is ready. */
- (BOOL)isReady;

// -------------------------------------------------------
//  Full GPS pipeline (single C++ call, no round-trips)
// -------------------------------------------------------

/**
 * Map-match one GPS point, resolve speed limit, find upcoming alerts.
 * All logic runs in C++; the caller only passes raw GPS.
 *
 * @param speedMs  Speed in m/s
 * @param timestamp  Milliseconds since epoch
 * @return MapZoneGpsProcessResult; check .matched before using other fields.
 */
- (MapZoneGpsProcessResult*)processGpsWithLat:(double)lat
                                               lng:(double)lng
                                           bearing:(double)bearing
                                           speedMs:(double)speedMs
                                          accuracy:(double)accuracy
                                         timestamp:(int64_t)timestamp;

// -------------------------------------------------------
//  V2 Bitmap generation (no V1 involved)
// -------------------------------------------------------

/**
 * Generate a speed-sign BMP for @c speedKmh km/h.
 * Returns nil for 0 or unknown speeds.
 */
- (nullable NSData *)generateSpeedSignBmp:(NSInteger)speedKmh;

/** Generate the standard camera-sign BMP. */
- (NSData *)generateCameraBmp;

/** Generate the standard toll-sign BMP. */
- (NSData *)generateTollBmp;

/**
 * Compute speed status.
 * @return 0=SAFE, 1=NEAR (within 5 km/h of limit), 2=OVER
 */
- (NSInteger)computeSpeedStatus:(NSInteger)currentSpeedLimit
                     userSpeedKmh:(double)userSpeedKmh;

// -------------------------------------------------------
//  V2 Voice generation (WAV PCM 16-bit LE, mono, 22050Hz)
// -------------------------------------------------------

/**
 * Generate voice WAV for current speed limit "X km/h" (number only, no prefix).
 * Returns nil for 0 or unknown speeds.
 * Returns a freshly generated WAV on each call (do not cache the bytes).
 */
- (nullable NSData *)generateCurrentSpeedLimitVoice:(NSInteger)speedKmh;

/**
 * Generate voice WAV for "tốc độ giới hạn tiếp theo X km/h".
 * Returns nil for 0 or unknown speeds.
 * Returns a freshly generated WAV on each call (do not cache the bytes).
 */
- (nullable NSData *)generateNextSpeedLimitVoice:(NSInteger)speedKmh;

/** "phía trước có camera theo dõi tốc độ" voice WAV. */
- (NSData *)generateSpeedCameraVoice;

/** "phía trước có camera phạt nguội" voice WAV. */
- (NSData *)generateCameraVoice;

/** "phía trước có trạm thu phí" voice WAV. */
- (NSData *)generateTollVoice;

/** "bạn đang vượt quá giới hạn tốc độ" voice WAV. */
- (NSData *)generateSpeedingVoice;

// ── v4 variant voice clips ─────────────────────────────────────────────

/** Red-light camera (category 0x2003) — "phía trước có camera giám sát". */
- (NSData *)generateRedLightCameraVoice;
/** AI camera (category 0x2004) — "phía trước có camera AI giám sát". */
- (NSData *)generateAICameraVoice;
/** Rest station (sign 0x100E) — "phía trước có trạm dừng nghỉ". */
- (NSData *)generateRestStationVoice;
/** No-left-turn (sign 0x1006, 0x1008) — "phía trước có biển báo cấm rẽ trái". */
- (NSData *)generateNoLeftTurnVoice;
/** No-right-turn (sign 0x1007, 0x1009) — "phía trước có biển báo cấm rẽ phải". */
- (NSData *)generateNoRightTurnVoice;
/** No U-turn (sign 0x100A, 0x100B) — "phía trước có biển báo cấm quay đầu". */
- (NSData *)generateNoUTurnVoice;
/** No overtaking (sign 0x1004) — "phía trước có biển báo cấm vượt". */
- (NSData *)generateNoOvertakingVoice;
/** End of no-overtaking zone (sign 0x1005) — "kết thúc đoạn cấm vượt". */
- (NSData *)generateNoOvertakingEndVoice;
/** No parking (sign 0x100D) — "phía trước có biển báo cấm dừng đỗ". */
- (NSData *)generateNoParkingVoice;
/** No straight ahead (sign 0x100C) — "phía trước có biển báo cấm đi thẳng". */
- (NSData *)generateNoStraightVoice;
/** Build-up area start (sign 0x1002) — "phía trước có biển báo khu dân cư". */
- (NSData *)generateBuildUpAreaStartVoice;
/** Build-up area end (sign 0x1003) — "kết thúc khu dân cư". */
- (NSData *)generateBuildUpAreaEndVoice;

// -------------------------------------------------------
//  Zone loading — C++ owns HTTP + decode + parse
// -------------------------------------------------------

/**
 * Configure the V2 (legacy unauthenticated) zone profile. Call once
 * before the first `updateLocationLegacyWithLat:lng:` call.
 *
 * @param baseUrl     Bare host of the zone API.
 * @param vehicleType Vehicle category code (1=car, 2=taxi, …; see SDK guide).
 * @param seats       Number of seats.
 * @param weights     Vehicle gross weight in kg.
 */
- (void)configureZoneLegacyWithBaseUrl:(NSString *)baseUrl
                       vehicleType:(NSInteger)vehicleType
                             seats:(NSInteger)seats
                           weights:(NSInteger)weights;

/**
 * Reload zone data if the position has moved outside the cached zone.
 * Safe to call on every GPS tick — the underlying check is cheap when
 * no fetch is needed.
 *
 * Must be called from a background queue; this call blocks until any
 * necessary HTTP request completes.
 *
 * @return YES when fresh zone data was loaded into the engine.
 */
- (BOOL)updateLocationLegacyWithLat:(double)lat lng:(double)lng;

// -------------------------------------------------------
//  V3 Zone loading
// -------------------------------------------------------

/**
 * Configure the V3 (authenticated) zone profile. Call once before the
 * first `updateLocationWithLat:lng:` call.
 *
 * @param baseUrl    Bare host, e.g. `@"https://driving.map.zone"`.
 *                   Must start with `https://`.
 * @param apiKeyId   Public API Key ID issued for this app.
 * @param apiKey     Authentication secret paired with `apiKeyId`. Treat
 *                   as a credential — do not log.
 * @param bundleId   Application bundle ID whitelisted for this key.
 * @param vehicleId  Vehicle identifier associated with `apiKeyId`.
 * @param vehicleType Vehicle category code.
 * @param seats      Number of seats.
 * @param weights    Vehicle gross weight in kg.
 *
 * Throws an NSException with name `"InvalidArgument"` if `baseUrl`
 * does not start with `https://`.
 */
- (void)configureZoneWithBaseUrl:(NSString *)baseUrl
                          apiKeyId:(NSString *)apiKeyId
                            apiKey:(NSString *)apiKey
                          bundleId:(NSString *)bundleId
                         vehicleId:(NSString *)vehicleId
                       vehicleType:(NSInteger)vehicleType
                             seats:(NSInteger)seats
                           weights:(NSInteger)weights;

/**
 * Reload zone data using the V3 profile, without motion context.
 * Equivalent to calling the four-argument variant with `speedKmh = 0`
 * and `bearingDeg = NAN`.
 *
 * Must be called from a background queue; this call blocks until any
 * necessary HTTP request completes.
 */
- (ZoneLoadResult *)updateLocationWithLat:(double)lat lng:(double)lng;

/**
 * Reload zone data using the V3 profile, with motion context for
 * smarter prefetching.
 *
 * Must be called from a background queue; this call blocks until any
 * necessary HTTP request completes. The engine internally checks
 * whether a reload is actually needed before issuing network requests.
 *
 * @param speedKmh   Current GPS speed in km/h. Faster driving widens
 *                   the prefetch margin so the next zone is fetched
 *                   further ahead of time. Pass `0` if speed is unknown.
 * @param bearingDeg Heading in degrees (0 = N, 90 = E). The request
 *                   sent to the server is projected ~400m forward
 *                   along this bearing so an edge-of-zone request
 *                   asks about the road ahead, not the road just
 *                   left. Pass `NAN` when bearing is unknown.
 */
- (ZoneLoadResult *)updateLocationWithLat:(double)lat
                                            lng:(double)lng
                                       speedKmh:(double)speedKmh
                                     bearingDeg:(double)bearingDeg;

// -------------------------------------------------------
//  Native log file sink — for field reports on debug builds.
// -------------------------------------------------------

/**
 * Begin (or stop) writing native logs to a file, in addition to the
 * Xcode console. Pass `nil` or `@""` to stop file logging. The file is
 * opened in append mode; subsequent sessions add to it.
 *
 * @return `YES` when the file is open after the call.
 */
- (BOOL)setLogFilePath:(nullable NSString *)path;

/** Current native log file path, or empty string when file logging is off. */
- (NSString *)logFilePath;

/** Flush pending bytes to disk before sharing the log file. */
- (void)flushLog;

// -------------------------------------------------------
//  Deprecated aliases — kept for source-compatibility with the previous
//  V2-suffixed bridge surface. New code should call the renamed methods
//  directly; these stubs forward and will be removed in a future release.
// -------------------------------------------------------

- (void)addLinkV2WithLinkId:(NSInteger)linkId
                 fromNodeId:(NSInteger)fromNodeId
                   toNodeId:(NSInteger)toNodeId
                  roadClass:(NSInteger)roadClass
                     oneway:(NSInteger)oneway
                posMaxSpeed:(NSInteger)posMaxSpeed
                negMaxSpeed:(NSInteger)negMaxSpeed
            encodedGeometry:(NSString *)encodedGeometry
    __attribute__((deprecated("Renamed: use -addLinkWithLinkId:fromNodeId:toNodeId:roadClass:oneway:posMaxSpeed:negMaxSpeed:encodedGeometry:")));

- (void)addAlertV2WithId:(NSInteger)alertId
               alertType:(NSInteger)alertType
                    type:(NSInteger)type
            affectLinkId:(NSInteger)affectLinkId
           isRightOrient:(BOOL)isRightOrient
                distance:(double)distance
                   speed:(NSInteger)speed
    __attribute__((deprecated("Renamed: use -addAlertWithId:alertType:type:affectLinkId:isRightOrient:distance:speed:")));

- (void)buildIndexV2
    __attribute__((deprecated("Renamed: use -buildIndex")));

- (void)resetMatcherV2
    __attribute__((deprecated("Renamed: use -resetMatcher")));

- (MapZoneSnapResult *)snapOnlineV2WithLat:(double)lat
                                       lng:(double)lng
                                   bearing:(double)bearing
                                     speed:(double)speed
                                  accuracy:(double)accuracy
                                 timestamp:(int64_t)timestamp
    __attribute__((deprecated("Renamed: use -snapOnlineWithLat:lng:bearing:speed:accuracy:timestamp:")));

- (NSArray<MapZoneNextAlert *> *)findNextAlertsV2WithLinkIdx:(NSInteger)linkIdx
                                                         dir:(NSString *)dir
                                                   distAlong:(double)distAlong
                                                      gpsLat:(double)gpsLat
                                                      gpsLng:(double)gpsLng
                                                    maxLinks:(NSInteger)maxLinks
    __attribute__((deprecated("Renamed: use -findNextAlertsWithLinkIdx:dir:distAlong:gpsLat:gpsLng:maxLinks:")));

- (NSInteger)getCurrentSpeedV2WithLinkIdx:(NSInteger)linkIdx
                                      dir:(NSString *)dir
                                distAlong:(double)distAlong
    __attribute__((deprecated("Renamed: use -getCurrentSpeedWithLinkIdx:dir:distAlong:")));

- (NSData *)generateCameraVoiceV2
    __attribute__((deprecated("Renamed: use -generateCameraVoice")));

@end

NS_ASSUME_NONNULL_END

#endif /* MapZoneGraphBridge_h */
