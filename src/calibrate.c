/** @file calibrate.c
 * @brief Utilities for measuring things on the robot
 */

#include <math.h>
#include "calibrate.h"
#include "motor.h"
#include "ports.h"
#include "tracking.h"

Calibration calibration;

static void calibrateMotorRpm(unsigned long now);

void calibrationInit() {
    calibration.mode = CALIBRATE_NONE;
}

void calibrationStart(CalibrationMode mode) {
    printf("Beginning MotorRpm calibration...\n");
    calibration.mode = mode;
    calibration.nextAt = 0;
    // The first call to calibrationUpdate() will get things rolling
}

void calibrationUpdate(unsigned long now) {
    if (calibration.mode == CALIBRATE_MOTOR_RPM) {
        calibrateMotorRpm(now);
    }
}

void calibrationEnd() {
    if (calibration.mode != CALIBRATE_NONE) {
        printf("Ending MotorRpm calibration...\n");
        // Restore settings
        calibration.mode = CALIBRATE_NONE;
        smartMotorEnabled(true);
        smartMotorSlew(MOTOR_LEFT_R, 0.75, 100);
        smartMotorSlew(MOTOR_RIGHT_R, 0.75, 100);
        smartMotorStopAll();
    }
}

void calibrateMotorRpm(unsigned long now) {
    if (calibration.nextAt == 0) {
        // Initial setup
        smartMotorEnabled(false);
        smartMotorSlew(MOTOR_LEFT_R, 100, 100);
        smartMotorSlew(MOTOR_RIGHT_R, 100, 100);
        smartMotorStopAll();
        calibration.channel = MOTOR_LEFT_R;
        calibration.input = -127;
        calibration.direction = 1;
        calibration.lastSpeed = 9999;

    } else if (now < calibration.nextAt) {
        // Wait for the test to stabilize
        return;

    } else {
        bool leftSide = (calibration.channel == MOTOR_LEFT_R);

        // We've given the wheel time to respond.  Is the velocity measurement stable?
        double speed = leftSide ? position.vLeft : position.vRight;
        double speedChange = speed - calibration.lastSpeed;
        calibration.lastSpeed = speed;
        if (abs(speedChange) > 0.1) {
            return;  // Keep waiting for it to stabilize
        }

        // Stabilized.  Measure and report the RPM.
        double rpm = speed * (60 / (WHEEL_RADIUS * M_TWOPI));  // converts inches/second to RPM
        printf("{\"test\":\"MotorRpm%s%s\", \"voltage\":%d, \"input\":%d, \"output\":%.3f, \"raw\":%d}\n",
               (leftSide ? "Left" : "Right"), (calibration.direction > 0 ? "Up" : "Down"),
               powerLevelMain(), calibration.input, rpm, motorGet(calibration.channel));

        // Advance to the next stage of this test
        if (calibration.direction > 0) {
            if (calibration.input < 127) {
                calibration.input++;
            } else {
                calibration.direction = -1;
            }
        } else {
            if (calibration.input > -127) {
                calibration.input--;
            } else if (calibration.channel == MOTOR_LEFT_R) {
                smartMotorSet(MOTOR_LEFT_R, 0);
                calibration.channel = MOTOR_RIGHT_R;
                calibration.input = -127;
                calibration.direction = 1;
                calibration.lastSpeed = 9999;
            } else {
                calibrationEnd();
                return;
            }
        }
    }
    smartMotorSet(calibration.channel, calibration.input);
    calibration.nextAt = now + 100;
}
