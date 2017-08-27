/** @file tracking.c
 * @brief Robot Position Tracking
 *
 * Uses sensors (mainly encoders) to estimate the robot's location on the field
 * relative to its start point.
 */

#include <API.h>
#include <math.h>
#include "ports.h"
#include "tracking.h"

#define MILLIS_PER_SECOND 1000

Position position;

void positionUpdate(unsigned long now) {
    int left = encoderGet(encoderLeft);
    int right = encoderGet(encoderRight);

    // Calculate change vs. the last time
    TickDelta* delta = &position.deltaHistory[position.deltaPos];
    position.deltaPos = (position.deltaPos + 1) % VELOCITY_SAMPLES;
    if (position.time != 0) {
        delta->millis = now - position.time;
        delta->left = left - position.left;
        delta->right = right - position.right;
    }
    position.time = now;
    position.left = left;
    position.right = right;

    if (abs(delta->left) > 360 || abs(delta->right) > 360) {
        return;  // ignore bad data due to encoder resets etc.
    }

    // Update our estimate of our position and heading
    double radiansLeft = delta->left * RADIANS_PER_TICK;
    double radiansRight = delta->right * RADIANS_PER_TICK;
    double deltaPosition = (radiansLeft + radiansRight) * (WHEEL_RADIUS / 2);  // this averages left and right
    double deltaHeading = (radiansRight - radiansLeft) * (WHEEL_RADIUS / AXLE_LENGTH);
    double midHeading = position.a + deltaHeading / 2;  // estimate of heading 1/2 way through the sample period
    position.x += deltaPosition * cos(midHeading);
    position.y += deltaPosition * sin(midHeading);
    position.a = fmod(position.a + deltaHeading + M_TWOPI, M_TWOPI);

    // Update our estimate of our velocity by averaging across the last few updates
    unsigned long sumMillis = 0;
    int sumLeft = 0, sumRight = 0;
    for (int i = 0; i < VELOCITY_SAMPLES; i++) {
        TickDelta* sample = &position.deltaHistory[i];
        if (abs(sample->left) > 360 || abs(sample->right) > 360) {
            continue;  // ignore bad data due to encoder resets etc.
        }
        sumMillis += sample->millis;
        sumLeft += sample->left;
        sumRight += sample->right;
    }
    if (sumMillis > 0) {
        double speedLeft = sumLeft * (RADIANS_PER_TICK * MILLIS_PER_SECOND / (double) sumMillis);  // radians/second
        double speedRight = sumRight * (RADIANS_PER_TICK * MILLIS_PER_SECOND / (double) sumMillis);
        position.v = (speedLeft + speedRight) * (WHEEL_RADIUS / 2);
        position.w = (speedRight - speedLeft) * (WHEEL_RADIUS / AXLE_LENGTH);
        position.leftRpm = speedLeft * (60 / M_TWOPI);
        position.rightRpm = speedLeft * (60 / M_TWOPI);
    }
}
