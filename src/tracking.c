/** @file tracking.c
 * @brief Robot Position Tracking
 *
 * Uses sensors (mainly encoders) to estimate the robot's location on the field
 * relative to its start point.
 */

#include <API.h>
#include <math.h>
#include "main.h"
#include "motor.h"
#include "ports.h"
#include "tracking.h"

#define MILLIS_PER_SECOND 1000

#define MAX_ROTATIONS_PER_MIN 170.0
#define MAX_RADIANS_PER_SEC (MAX_ROTATIONS_PER_MIN * M_TWOPI / 60)
#define MAX_LINEAR_SPEED (MAX_RADIANS_PER_SEC * WHEEL_RADIUS)                      // inches/sec
#define MAX_TURN_SPEED (MAX_RADIANS_PER_SEC * 2 * WHEEL_RADIUS / AXLE_LENGTH)      // radians/sec

typedef struct {
    double x;  // inches
    double y;  // inches
    double a;  // radians
    double v;  // inches/sec
    double w;  // radians/sec
    double k_1;
    double k_2_v;
    double k_3;
} PositionTarget;

static void setChassisSpeed(double linearSpeed, double turnSpeed);
static void setWheelSpeed(unsigned char front, unsigned char rear, double desired, double actual);
static double normalizeAngle(double rads, double min, double max);

Position position;
PositionTarget positionTarget;

void trackingUpdate(unsigned long now) {
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
    position.a = normalizeAngle(position.a + deltaHeading, 0, M_TWOPI);

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
        // Convert ticks/millisecond to inches/second
        double speedLeft = sumLeft * (RADIANS_PER_TICK * WHEEL_RADIUS * MILLIS_PER_SECOND) / (double) sumMillis;
        double speedRight = sumRight * (RADIANS_PER_TICK * WHEEL_RADIUS * MILLIS_PER_SECOND) / (double) sumMillis;
        position.v = (speedLeft + speedRight) / 2;
        position.w = (speedRight - speedLeft) / AXLE_LENGTH;   // radians/second
        position.vLeft = speedLeft;
        position.vRight = speedRight;
    }
}

void setChassisSpeed(double linearSpeed, double turnSpeed) {
    // Convert desired linear & turn values into tank chassis left & right
    double left = linearSpeed + turnSpeed * AXLE_LENGTH / 2;
    double right = linearSpeed - turnSpeed * AXLE_LENGTH / 2;

    // Scale down inputs to maintain curvature radius if requested speeds exceeds physical limits
    double saturation = MAX(abs(left) / MAX_LINEAR_SPEED, abs(right) / MAX_LINEAR_SPEED);
    if (saturation > 1) {
        left /= saturation;
        right /= saturation;
    }

    setWheelSpeed(MOTOR_LEFT_F, MOTOR_LEFT_R, left, position.vLeft);
    setWheelSpeed(MOTOR_RIGHT_F, MOTOR_RIGHT_R, right, position.vRight);
}

void setWheelSpeed(unsigned char front, unsigned char rear, double desired, double actual) {
    // Simple P controller w/both feedforward and feedback components
    double levelFeedforward = desired;
    double levelFeedback = 0.1 * (desired - actual);
    int value = lround(127 * (levelFeedforward + levelFeedback) / MAX_LINEAR_SPEED);
    smartMotorSet(front, value);
    smartMotorSet(rear, value);
}

void trackingSetDriveSpeed(short forward, short turn) {
    setChassisSpeed(forward * (MAX_LINEAR_SPEED / 127), turn * (MAX_TURN_SPEED / 127));
}

void trackingSetDriveTarget(double x, double y, double a) {
    trackingSetDriveWaypoint(x, y, a, 0, 0);
}

void trackingSetDriveWaypoint(double x, double y, double a, double v, double w) {
    // Don't come to a complete stop at the end, avoid singularities in the math of the feedback design
    v = (abs(v) < 0.01) ? 0.01 : 0;
    w = (abs(w) < 0.01) ? 0.01 : 0;

    // Set destination location, heading and velocities
    positionTarget.x = x;
    positionTarget.y = y;
    positionTarget.a = normalizeAngle(a, -M_PI, M_PI);
    positionTarget.v = v;
    positionTarget.w = w;

    // Compute the gain matrix for this target
    double zeta = 0.7;  // damping factor, adjust 0 < zeta < 1 to get good results
    double g = 60;      // gain constant, adjust 0 < gain to get good results
    positionTarget.k_1 = 2 * zeta * sqrt(w * w + g * v * v);
    positionTarget.k_2_v = g * v;
    positionTarget.k_3 = positionTarget.k_1;
}

void trackingDriveToTarget() {
    // Algorithm based on the Nonlinear Controller described at:
    // https://www.researchgate.net/publication/224115822_Experimental_comparison_of_trajectory_tracking_algorithms_for_nonholonomic_mobile_robot

    // Compute deviation from desired position and heading
    double d_x = positionTarget.x - position.x;
    double d_y = positionTarget.y - position.y;
    double d_a = normalizeAngle(positionTarget.a - position.a, -M_PI, M_PI);

    // Transform error values to the perspective of the robot
    double cos_a = cos(position.a);
    double sin_a = sin(position.a);
    double e_1 = d_x * cos_a + d_y * sin_a;   // Distance to go straight ahead (may be negative)
    double e_2 = d_y * cos_a - d_x * sin_a;   // Distance to go side-to-side (obviously can't move directly in that direction)
    double e_3 = d_a;                         // Deviation from desired heading

    // Compute feedforward linear and angular velocity
    double uf_v = positionTarget.v * cos(e_3);
    double uf_w = positionTarget.w;

    // Compute feedback linear and angular velocity
    double ub_v = e_1 * positionTarget.k_1;
    double ub_w = e_2 * positionTarget.k_2_v * (abs(e_3) >= 0.001 ? sin(e_3) / e_3 : 1) + e_3 * positionTarget.k_3;

    double linearSpeed = uf_v + ub_v;  // inches/sec
    double turnSpeed = uf_w + ub_w;   // radians/sec

    setChassisSpeed(linearSpeed, turnSpeed);
}

double normalizeAngle(double rads, double min, double max) {
    if (rads < min) {
        rads += M_TWOPI;
    } else if (rads >= max) {
        rads -= M_TWOPI;
    }
    return rads;
}
