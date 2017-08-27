/** @file tracking.h
 * @brief Robot Position Tracking
 *
 * Uses sensors (mainly encoders) to estimate the robot's location on the field
 * relative to its start point.
 */

#ifndef tracking_h
#define tracking_h

#define WHEEL_RADIUS 4      // inches
#define AXLE_LENGTH 10.25   // inches
#define TICKS_TO_RADIANS (M_TWOPI/360)
#define VELOCITY_SAMPLES 4

#define RADIANS_TO_DEGREES (360/M_TWOPI)
#define MILLIS_PER_SECOND 1000

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
    double w;   // rate of turn (counter-clockwise radians/second)
    TickDelta deltaHistory[VELOCITY_SAMPLES];  // recent history of tick count measurements (ring buffer)
    int deltaPos;                              // index of 'deltaHistory' containing the most recent
} Position;

void positionUpdate(unsigned long now);

extern Position position;

#endif //tracking_h
