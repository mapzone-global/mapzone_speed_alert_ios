#ifndef MapZoneAlertBridge_h
#define MapZoneAlertBridge_h

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MapZoneAlertBridge : NSObject

+ (instancetype _Nonnull)sharedInstance;

- (void)configureWithApiUrl:(NSString *)apiUrl apiKey:(NSString *)apiKey apiID:(NSString *)apiID;

// Alert lifecycle methods
- (void)startAlert;
- (void)stopAlert;

// Main method - returns speed limit text for TTS
- (void)processLocation:(CLLocation *)location
          forSpeedAlert:(double)heading 
                 speed:(double)speed
             vehicleId:(NSString *)vehicleId
           vehicleType:(int)vehicleType
                 seats:(int)seats
               weights:(float)weights
          maxProvision:(int)maxProvision
            completion:(void (^)(BOOL hasSpeedLimit, NSString * _Nullable speedLimitText))completion;

// Speed status constants
typedef NS_ENUM(NSInteger, MapZoneSpeedStatus) {
    MapZoneSpeedStatusSafe = 0,
    MapZoneSpeedStatusNearLimit = 1,
    MapZoneSpeedStatusOverLimit = 2
};

// Speed sign bitmap callback - delivers bitmaps (priority: current speed → next speed → camera → toll):
// currentBmpData/speedStatus: current link speed limit sign
// nextBmpData/nextDistanceMeters: next upcoming speed sign
// cameraBmpData/cameraDistanceMeters: next camera ahead
// tollBmpData/tollDistanceMeters: next toll booth ahead
typedef void (^SpeedSignBitmapBlock)(NSData * _Nullable currentBmpData, MapZoneSpeedStatus speedStatus, NSData * _Nullable nextBmpData, int nextDistanceMeters, NSData * _Nullable cameraBmpData, int cameraDistanceMeters, NSData * _Nullable tollBmpData, int tollDistanceMeters);

- (void)setSpeedSignBitmapCallback:(SpeedSignBitmapBlock _Nullable)callback;

/**
 * Fire the speed-sign bitmap callback directly from v2 graph data.
 * Mirrors the v1 C++ path but generates bitmaps from supplied integer values.
 *
 * @param currentSpeedLimit  km/h on the matched link (0 = none)
 * @param userSpeedKmh       current GPS speed in km/h (used for speedStatus)
 * @param nextSpeedLimit     km/h of the next different sign (0 = none)
 * @param nextDistMeters     distance to that next sign
 * @param cameraDistMeters   distance to nearest camera ahead (0 = none)
 * @param tollDistMeters     distance to nearest toll ahead (0 = none)
 */
- (void)fireSpeedSignBitmapV2:(int)currentSpeedLimit
                 userSpeedKmh:(double)userSpeedKmh
               nextSpeedLimit:(int)nextSpeedLimit
               nextDistMeters:(int)nextDistMeters
             cameraDistMeters:(int)cameraDistMeters
               tollDistMeters:(int)tollDistMeters;

@end

NS_ASSUME_NONNULL_END

#endif /* MapZoneAlertBridge_h */
