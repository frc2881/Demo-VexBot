/** @file calibrate.c
 * @brief Utilities for measuring things on the robot
 */

#include "calibrate.h"
#include "main.h"
#include "motor.h"
#include "ports.h"
#include "tracking.h"

Calibration calibration;

static void calibrateMotorRpm(unsigned long now);

void calibrationInit() {
    calibration.mode = CALIBRATE_NONE;
}

void calibrationStart(CalibrationMode mode) {
    printf("Beginning MotorRpm calibration...");
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
        printf("Ending MotorRpm calibration...");
        calibration.mode = CALIBRATE_NONE;
        smartMotorSlewEnabled(true);  // restore settings
        smartMotorStopAll();
    }
}

void calibrateMotorRpm(unsigned long now) {
    if (calibration.nextAt == 0) {
        // Initial setup
        smartMotorSlewEnabled(false);
        smartMotorStopAll();
        calibration.channel = MOTOR_LEFT_R;
        calibration.input = 0;
        calibration.lastRpm = 0;

    } else if (now < calibration.nextAt) {
        // Wait for the test to stabilize
        return;

    } else {
        bool leftSide = (calibration.channel == MOTOR_LEFT_R);

        // We've given the wheel time to respond.  Is the velocity measurement stable?
        double rpm = leftSide ? position.leftRpm : position.rightRpm;
        double rpmChange = rpm - calibration.lastRpm;
        calibration.lastRpm = rpm;
        if (abs(rpmChange) > 0.1) {
            return;  // Keep waiting for it to stabilize
        }

        // Stabilized.  Measure and report the RPM.
        printf("{\"test\":\"MotorRpm%s\", \"voltage\":%d, \"input\":%d, \"output\":%.3f}\n",
               leftSide ? "Left" : "Right", powerLevelMain(), calibration.input, rpm);

        // Advance to the next stage of this test
        if (calibration.input == -127) {
            if (calibration.channel == MOTOR_LEFT_R) {
                smartMotorSet(MOTOR_LEFT_R, 0);
                calibration.channel = MOTOR_RIGHT_R;
                calibration.input = 0;
                calibration.lastRpm = 0;
            } else {
                calibrationEnd();
                return;
            }
        } else if (calibration.input == 127) {
            calibration.input = -2;
        } else if (calibration.input >= 0) {
            calibration.input = MIN(calibration.input + 2, 127);
        } else {
            calibration.input = MAX(calibration.input - 2, -127);
        }
    }

    smartMotorSet(calibration.channel, calibration.input);
    calibration.nextAt = now + 80;
}
