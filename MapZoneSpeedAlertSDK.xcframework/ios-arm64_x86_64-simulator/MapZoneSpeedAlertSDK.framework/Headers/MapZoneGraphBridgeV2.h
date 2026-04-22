#ifndef MapZoneGraphBridgeV2_h
#define MapZoneGraphBridgeV2_h

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// ============================================================
//  GPS processing result (plain value object, no raw data to caller)
// ============================================================

/**
 * Result of a single full GPS processing pass (snap + alerts).
 * All distances in metres; speeds in km/h. 0 = none/not applicable.
 */
@interface MapZoneGpsProcessResultV2 : NSObject
@property (nonatomic, assign) BOOL     matched;
@property (nonatomic, assign) NSInteger currentSpeedLimit;
@property (nonatomic, assign) NSInteger nextSpeedLimit;
@property (nonatomic, assign) NSInteger nextDistMeters;
@property (nonatomic, assign) NSInteger cameraDistMeters;
@property (nonatomic, assign) NSInteger tollDistMeters;
/** Voice trigger: 0=none, 1=speed_changed, 2=approaching_speed, 3=camera, 4=toll */
@property (nonatomic, assign) NSInteger voiceTrigger;
/** Speed value to announce (for trigger 1 and 2). */
@property (nonatomic, assign) NSInteger voiceSpeedValue;
@end

// ============================================================
//  Result types
// ============================================================

/**
 * Result of a single online map-match (v2).
 */
@interface MapZoneSnapResultV2 : NSObject

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
@interface MapZoneNextAlertV2 : NSObject

/** Alert ID. */
@property (nonatomic, assign) NSInteger id;
/**
 * Alert category:
 *   0 = speed sign
 *   1 = toll station
 *   2 = camera
 *  99 = synthetic speed-limit-change event (isSpeedChange=YES)
 */
@property (nonatomic, assign) NSInteger alertType;
/** Sub-type (e.g. 167 = speed-limit sign). */
@property (nonatomic, assign) NSInteger type;
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
//  Bridge interface
// ============================================================

/**
 * MapZoneGraphBridgeV2 — Obj-C++ bridge to the VietMap Graph Engine V2.
 *
 * All public methods are suffixed with "V2".
 * Thread-safe: the underlying C++ engine uses a mutex.
 *
 * Usage:
 * @code
 *   MapZoneGraphBridgeV2 *bridge = [MapZoneGraphBridgeV2 sharedInstance];
 *
 *   // Load graph on a background queue
 *   [bridge addLinkV2WithLinkId:… fromNodeId:… toNodeId:…
 *                     roadClass:… oneway:… posMaxSpeed:… negMaxSpeed:…
 *                encodedGeometry:@"…"];
 *   [bridge addAlertV2WithId:… alertType:… type:… affectLinkId:…
 *             isRightOrient:YES distance:… speed:…];
 *   [bridge buildIndexV2];
 *
 *   // Real-time GPS updates
 *   MapZoneSnapResultV2 *snap = [bridge snapOnlineV2WithLat:… lng:… bearing:…
 *                                                  speed:… accuracy:… timestamp:…];
 *   if (snap.matched) {
 *       NSInteger limit = [bridge getCurrentSpeedV2WithLinkIdx:snap.linkIdx
 *                                                         dir:snap.dir
 *                                                   distAlong:snap.distAlong];
 *       NSArray<MapZoneNextAlertV2*> *alerts =
 *           [bridge findNextAlertsV2WithLinkIdx:snap.linkIdx dir:snap.dir
 *                                    distAlong:snap.distAlong
 *                                       gpsLat:lat gpsLng:lng maxLinks:5];
 *   }
 * @endcode
 */
@interface MapZoneGraphBridgeV2 : NSObject

+ (instancetype)sharedInstance;

// -------------------------------------------------------
//  Graph loading
// -------------------------------------------------------

/**
 * Add a road link with Google-encoded polyline geometry.
 * Must be called before buildIndexV2.
 */
- (void)addLinkV2WithLinkId:(NSInteger)linkId
                 fromNodeId:(NSInteger)fromNodeId
                   toNodeId:(NSInteger)toNodeId
                  roadClass:(NSInteger)roadClass
                     oneway:(NSInteger)oneway
                posMaxSpeed:(NSInteger)posMaxSpeed
                negMaxSpeed:(NSInteger)negMaxSpeed
            encodedGeometry:(NSString *)encodedGeometry;

/**
 * Add an alert associated with a link.
 * Must be called before buildIndexV2.
 *
 * @param alertType    0=speed sign, 1=toll, 2=camera
 * @param isRightOrient YES = POS direction (from→to), NO = NEG
 * @param distance     Meters from fromNode along link geometry
 * @param speed        Speed value in km/h (for speed signs)
 */
- (void)addAlertV2WithId:(NSInteger)alertId
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
- (void)buildIndexV2;

/**
 * Clear all graph data and engine state.
 * After this you must reload links/alerts and call buildIndexV2 again.
 */
- (void)clearV2;

/**
 * Reset the HMM matcher state (begin new GPS sequence).
 * Graph data and indices are preserved.
 */
- (void)resetMatcherV2;

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
 * @return MapZoneSnapResultV2 — check matched before using other fields
 */
- (MapZoneSnapResultV2 *)snapOnlineV2WithLat:(double)lat
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
 * @param linkIdx   linkIdx from MapZoneSnapResultV2
 * @param dir       @"pos" or @"neg"
 * @param distAlong distAlong from MapZoneSnapResultV2
 * @param gpsLat    Current GPS latitude
 * @param gpsLng    Current GPS longitude
 * @param maxLinks  How many links ahead to scan (suggested: 5)
 * @return Array of MapZoneNextAlertV2, sorted by distanceAhead ascending
 */
- (NSArray<MapZoneNextAlertV2 *> *)findNextAlertsV2WithLinkIdx:(NSInteger)linkIdx
                                                           dir:(NSString *)dir
                                                     distAlong:(double)distAlong
                                                        gpsLat:(double)gpsLat
                                                        gpsLng:(double)gpsLng
                                                      maxLinks:(NSInteger)maxLinks;

/**
 * Get current speed limit at matched position.
 * Returns speed in km/h, or 0 if unavailable.
 */
- (NSInteger)getCurrentSpeedV2WithLinkIdx:(NSInteger)linkIdx
                                      dir:(NSString *)dir
                                distAlong:(double)distAlong;

// -------------------------------------------------------
//  Diagnostics
// -------------------------------------------------------

/** Number of links loaded in the graph. */
- (NSInteger)getLinkCountV2;
/** Number of alerts loaded in the graph. */
- (NSInteger)getAlertCountV2;
/** YES if buildIndexV2 has been called and the engine is ready. */
- (BOOL)isReadyV2;

// -------------------------------------------------------
//  Full GPS pipeline (single C++ call, no round-trips)
// -------------------------------------------------------

/**
 * Map-match one GPS point, resolve speed limit, find upcoming alerts.
 * All logic runs in C++; the caller only passes raw GPS.
 *
 * @param speedMs  Speed in m/s
 * @param timestamp  Milliseconds since epoch
 * @return MapZoneGpsProcessResultV2; check .matched before using other fields.
 */
- (MapZoneGpsProcessResultV2*)processGpsV2WithLat:(double)lat
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
- (nullable NSData *)generateSpeedSignBmpV2:(NSInteger)speedKmh;

/** Generate the standard camera-sign BMP. */
- (NSData *)generateCameraBmpV2;

/** Generate the standard toll-sign BMP. */
- (NSData *)generateTollBmpV2;

/**
 * Compute speed status.
 * @return 0=SAFE, 1=NEAR (within 5 km/h of limit), 2=OVER
 */
- (NSInteger)computeSpeedStatusV2:(NSInteger)currentSpeedLimit
                     userSpeedKmh:(double)userSpeedKmh;

// -------------------------------------------------------
//  V2 Voice generation (WAV PCM 16-bit LE, mono, 22050Hz)
// -------------------------------------------------------

/**
 * Generate voice WAV for current speed limit "X km/h" (number only, no prefix).
 * Returns nil for 0 or unknown speeds.
 * Each call produces byte-different WAV (anti-matching noise).
 */
- (nullable NSData *)generateCurrentSpeedLimitVoiceV2:(NSInteger)speedKmh;

/**
 * Generate voice WAV for "tốc độ giới hạn tiếp theo X km/h".
 * Returns nil for 0 or unknown speeds.
 * Each call produces byte-different WAV (anti-matching noise).
 */
- (nullable NSData *)generateNextSpeedLimitVoiceV2:(NSInteger)speedKmh;

/** "phía trước có camera theo dõi tốc độ" voice WAV. */
- (NSData *)generateSpeedCameraVoiceV2;

/** "phía trước có camera phạt nguội" voice WAV. */
- (NSData *)generateCameraVoiceV2;

/** "phía trước có trạm thu phí" voice WAV. */
- (NSData *)generateTollVoiceV2;

/** "bạn đang vượt quá giới hạn tốc độ" voice WAV. */
- (NSData *)generateSpeedingVoiceV2;

// -------------------------------------------------------
//  Zone loading — C++ owns HTTP + decode + parse
// -------------------------------------------------------

/**
 * Store Zone API configuration in the C++ engine.
 * Call once before the first updateLocationV2 call.
 *
 * @param baseUrl     Base URL, e.g. @"https://dev.fastmap.vn/drivingalert"
 * @param vehicleType 1=car, 2=taxi, etc.
 * @param seats       Seat count
 * @param weights     Vehicle weight in kg (integer)
 */
- (void)configureZoneV2WithBaseUrl:(NSString *)baseUrl
                       vehicleType:(NSInteger)vehicleType
                             seats:(NSInteger)seats
                           weights:(NSInteger)weights;

/**
 * Check zone cache; if a reload is needed → synchronous NSURLSession GET →
 * base64/XOR decode → JSON parse → load links/alerts into C++ engine.
 * Must be called from a background queue (blocks until HTTP returns).
 *
 * @return YES if new zone data was loaded.
 */
- (BOOL)updateLocationV2WithLat:(double)lat lng:(double)lng;

@end

NS_ASSUME_NONNULL_END

#endif /* MapZoneGraphBridgeV2_h */
