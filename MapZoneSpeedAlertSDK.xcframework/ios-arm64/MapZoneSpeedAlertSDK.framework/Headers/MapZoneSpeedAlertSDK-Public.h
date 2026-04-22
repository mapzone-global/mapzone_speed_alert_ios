//
//  MapZoneSpeedAlertSDK-Public.h
//  MapZoneSpeedAlertSDK
//
//  Public C interface for MapZoneSpeedAlertSDK
//

#ifndef MAPZONE_SPEED_ALERT_SDK_PUBLIC_H
#define MAPZONE_SPEED_ALERT_SDK_PUBLIC_H

#import <Foundation/Foundation.h>

//! Project version number for MapZoneSpeedAlertSDK.
FOUNDATION_EXPORT double MapZoneSpeedAlertSDKVersionNumber;

//! Project version string for MapZoneSpeedAlertSDK.
FOUNDATION_EXPORT const unsigned char MapZoneSpeedAlertSDKVersionString[];

// C-compatible data structures
typedef struct {
    double latitude;
    double longitude;
    double altitude;
    double accuracy;
    double speed;
    double heading;
    long long timestamp;
} VTLocation;

typedef struct {
    int enableTracking;
    int uploadInterval;
    int locationUpdateInterval;
    int enablePowerSaving;
    int maxBatchSize;
} VTTrackingConfig;

typedef struct {
    int enableSpeedAlerts;
    double speedLimitThreshold;
    int enableGeofenceAlerts;
    int enableRouteDeviationAlerts;
    double deviationThreshold;
} VTAlertConfig;

typedef struct {
    double* latitudes;
    double* longitudes;
    int pointCount;
    double totalDistance;
    long long estimatedDuration;
} VTRouteData;

// Callback function types
typedef void (*VTLocationUpdateCallback)(VTLocation location);
typedef void (*VTTrackingStateCallback)(int isTracking);
typedef void (*VTErrorCallback)(const char* errorMessage);
typedef void (*VTAlertCallback)(const char* alertType, const char* message);

// C functions for the SDK
#ifdef __cplusplus
extern "C" {
#endif

// SDK lifecycle
int vt_initialize_sdk(const char* apiKey);
void vt_cleanup_sdk(void);

// Configuration
int vt_set_tracking_config(VTTrackingConfig config);
VTTrackingConfig vt_get_tracking_config(void);
int vt_set_alert_config(VTAlertConfig config);
VTAlertConfig vt_get_alert_config(void);

// Tracking control
int vt_start_tracking(void);
int vt_stop_tracking(void);
int vt_is_tracking(void);

// Location management
int vt_add_location(VTLocation location);
VTLocation* vt_get_locations(int* count);
void vt_clear_locations(void);

// Route management
int vt_set_route(VTRouteData route);
VTRouteData vt_get_current_route(void);
void vt_clear_route(void);

// Callbacks
void vt_set_location_callback(VTLocationUpdateCallback callback);
void vt_set_tracking_state_callback(VTTrackingStateCallback callback);
void vt_set_error_callback(VTErrorCallback callback);
void vt_set_alert_callback(VTAlertCallback callback);

// Data upload
int vt_upload_data(void);
int vt_is_uploading(void);

// Statistics
int vt_get_tracked_points_count(void);
double vt_get_total_distance(void);
long long vt_get_tracking_duration(void);

// Device info
const char* vt_get_device_id(void);
const char* vt_get_sdk_version(void);

#ifdef __cplusplus
}
#endif

#endif // MAPZONE_SPEED_ALERT_SDK_PUBLIC_H
