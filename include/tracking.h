/** @file tracking.h
 * @brief Robot Position Tracking
 *
 * Uses sensors (mainly encoders) to estimate the robot's location on the field
 * relative to its start point.
 */

#ifndef tracking_h
#define tracking_h

#define AXLE_LENGTH 10.25   // inches
#define WHEEL_RADIUS 4      // inches
#define RADIANS_PER_TICK (M_TWOPI/360)
#define VELOCITY_SAMPLES 4

#define DEGREES_PER_RADIAN (360/M_TWOPI)

// Difference in encoder tick count over a time period
typedef struct {
    unsigned long millis;
    int left;
    int right;
} TickDelta;

typedef struct {
    unsigned long time;  // millis since robot start
    int left;   // encoder tick count
    int right;  // encoder tick count
    double x;   // relative to robot start location (inches)
    double y;   // relative to robot start location (inches)
    double a;   // current heading (radians)
    double v;   // forward velocity (inches/second)
    double w;   // rate of turning left (positive) or right (negative) (radians/second)
    double leftRpm;
    double rightRpm;
    TickDelta deltaHistory[VELOCITY_SAMPLES];  // recent history of tick count measurements (ring buffer)
    int deltaPos;                              // index of 'deltaHistory' containing the most recent
} Position;

// Lifecycle
void trackingUpdate(unsigned long now);
void trackingDriveToTarget();

// Control
void trackingSetSpeed(double linearSpeed, double rotationSpeed);
void trackingSetDriveTarget(double x, double y, double a);
void trackingSetDriveWaypoint(double x, double y, double a, double v, double w);

extern Position position;

#endif //tracking_h
