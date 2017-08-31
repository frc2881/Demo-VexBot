/** @file opcontrol.c
 * @brief File for operator control code
 *
 * This file should contain the user operatorControl() function and any functions related to it.
 *
 * Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 *
 * PROS contains FreeRTOS (http://www.freertos.org) whose source code may be
 * obtained from http://sourceforge.net/projects/freertos/files/ or on request.
 */

#include <math.h>
#include <string.h>
#include "main.h"
#include "calibrate.h"
#include "hid.h"
#include "motor.h"
#include "ports.h"
#include "tracking.h"

#define SLEEP_MILLIS 20

typedef enum {
    DRIVE_TANK,    // Left joystick controls left wheels, right joystick controls right wheels
    DRIVE_ARCADE,  // Left joystick turns, right joystick for forward & back
    DRIVE_CONSTANT_RADIUS,  // Left joystick sets turn radius, right joystick speed
    DRIVE_ACCELEROMETER,    // Controller accelerometer (tilt the controller forward & back, left & right)
} DriveMode;

DriveMode chooseDriveMode();

void tankDrive(int left, int right, bool squared);
void arcadeDrive(int power, int turn);
void constantRadiusDrive(int power, int turn);

void debugUpdate();
void debugPrintState();

void ledUpdate(unsigned long now);

void lcdUpdate(const LcdInput *lcdInput, const Controller *joystick, DriveMode driveMode,
               unsigned long sleptAt, unsigned long now);

static short _debugCounter = 0;
static short _debugInterval = 25;

static char _spinnerChars[] = "-\\|/";

/*
 * Runs the user operator control code. This function will be started in its own task with the
 * default priority and stack size whenever the robot is enabled via the Field Management System
 * or the VEX Competition Switch in the operator control mode. If the robot is disabled or
 * communications is lost, the operator control task will be stopped by the kernel. Re-enabling
 * the robot will restart the task, not resume it from where it left off.
 *
 * If no VEX Competition Switch or Field Management system is plugged in, the VEX Cortex will
 * run the operator control task. Be warned that this will also occur if the VEX Cortex is
 * tethered directly to a computer via the USB A to A cable without any VEX Joystick attached.
 *
 * Code running in this task can take almost any action, as the VEX Joystick is available and
 * the scheduler is operational. However, proper use of delay() or taskDelayUntil() is highly
 * recommended to give other tasks (including system tasks such as updating LCDs) time to run.
 *
 * This task should never exit; it should end with some kind of infinite loop, even if empty.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void operatorControl() {
    // Initialize input from Joystick 1 (and optionally LCD 1 buttons)
    hidInit(1, LCD_PORT);
    LcdInput *lcdInput = hidLcdInput();
    Controller *joystick = hidController(1);
    joystick->accel.vertScale = -1;  // Tilt down moves forward, tilt up moves back

    calibrationInit();

    bool underAutopilotControl = false;

    unsigned long previousWakeTime = millis();
    unsigned long sleptAt = millis();
    while (true) {
        unsigned long now = millis();

        // Update sensor readings
        trackingUpdate(now);

        // Update controller inputs
        hidUpdate(now);

        if (joystick->leftButtons.down.changed == -1) {
            // Turn around 180 degrees and reverse course
            trackingSetDriveWaypoint(position.x, position.y, position.a + M_PI, position.v, 0);
            underAutopilotControl = true;

        } else if (joystick->leftButtons.left.changed == -1) {
            // Turn left 90 degrees (and go to what location?)
            trackingSetDriveWaypoint(position.x, position.y, position.a + M_PI_2, position.v, 0);
            underAutopilotControl = true;

        } else if (joystick->leftButtons.right.changed == -1) {
            // Turn right 90 degrees (and go to what location?)
            trackingSetDriveWaypoint(position.x, position.y, position.a - M_PI_2, position.v, 0);
            underAutopilotControl = true;

        } else if (joystick->leftButtons.up.changed == -1) {
            // Turn around 180 degrees while continuing in the same direction
            trackingSetDriveWaypoint(position.x, position.y, position.a + M_PI, -position.v, 0);
            underAutopilotControl = true;

        } else if (joystick->rightButtons.up.changed == -1) {
            calibrationStart(CALIBRATE_MOTOR_RPM);
            underAutopilotControl = false;

        } else if (joystick->lastChangedTime == now) {
            // Any joystick input cancels semi-autonomous tasks
            underAutopilotControl = false;
            calibrationEnd();
        }

        // What chassis control algorithm has the user selected?
        DriveMode driveMode = chooseDriveMode();

        // Is the robot running a semi-autonomous task or under user control?
        if (underAutopilotControl) {
            trackingDriveToTarget();

        } else if (calibration.mode != CALIBRATE_NONE) {
            // Debugging/calibration
            calibrationUpdate(now);

        } else {
            // Use joysticks to control drive train
            bool squared = joystick->rightButtons.right.pressed;
            if (driveMode == DRIVE_TANK) {
                tankDrive(joystick->left.vert, joystick->right.vert, squared);

            } else if (driveMode == DRIVE_ARCADE) {
                arcadeDrive(joystick->right.vert, joystick->left.horz);

            } else if (driveMode == DRIVE_CONSTANT_RADIUS) {
                constantRadiusDrive(joystick->right.vert, joystick->left.horz);

            } else if (driveMode == DRIVE_ACCELEROMETER) {
                arcadeDrive(joystick->accel.vert, joystick->accel.horz);
            }

            // Experimental pneumatics
//            digitalWrite(1, joystick->rightButtons.left.pressed);
        }

        bool flashlightOn = joystick->leftTrigger.up.pressed || joystick->rightTrigger.up.pressed;
        smartMotorSet(MOTOR_FLASHLIGHT, flashlightOn ? 127 : 0);

        // Apply desired drive settings to all motors
        smartMotorUpdate();

        // Show status
        ledUpdate(now);
        lcdUpdate(lcdInput, joystick, driveMode, sleptAt, now);
        debugUpdate();

        // Sleep for a while, give other tasks a chance to run
        sleptAt = millis();
        taskDelayUntil(&previousWakeTime, SLEEP_MILLIS);
    }
}
#pragma clang diagnostic pop

/*
 * Drives the chassis.  A positive 'turn' argument turns clockwise.
 */
void arcadeDrive(int power, int turn) {
    smartMotorSet(MOTOR_LEFT_F, power + turn);  // set left-front wheel(s)
    smartMotorSet(MOTOR_LEFT_R, power + turn);  // set left-rear wheel(s)
    smartMotorSet(MOTOR_RIGHT_F, power - turn);  // set right wheel(s)
    smartMotorSet(MOTOR_RIGHT_R, power - turn);  // set right wheel(s)
}

void tankDrive(int left, int right, bool squared) {
    if (squared) {
        left = left * abs(left) / 127;
        right = right * abs(right) / 127;
    }
    smartMotorSet(MOTOR_LEFT_F, left);  // set left-front wheel(s)
    smartMotorSet(MOTOR_LEFT_R, left);  // set left-rear wheel(s)
    smartMotorSet(MOTOR_RIGHT_F, right);  // set right wheel(s)
    smartMotorSet(MOTOR_RIGHT_R, right);  // set right wheel(s)
}

void constantRadiusDrive(int power, int turn) {
    // Map 'turn' to a value between -1 and 1 corresponding to the ratio of inner/outer wheel in the turn
    float scaled = turn * turn / (float) (127 * 127);  // 0 <= scaled <= 1.0, squaring flattens initial input response
    float ratio = 1 - (2 * scaled);  // -1.0 <= ratio <= 1.0; turn==0 => ratio=1; turn==127 => ratio=-1
    int inner = lroundf(power * ratio);
    int left = (turn >= 0) ? power : inner;
    int right = (turn >= 0) ? inner : power;
    smartMotorSet(MOTOR_LEFT_F, left);  // set left-front wheel(s)
    smartMotorSet(MOTOR_LEFT_R, left);  // set left-rear wheel(s)
    smartMotorSet(MOTOR_RIGHT_F, right);  // set right wheel(s)
    smartMotorSet(MOTOR_RIGHT_R, right);  // set right wheel(s)
}

DriveMode chooseDriveMode() {
    static DriveMode _driveMode = DRIVE_TANK;
    // button '8 down' cycles through control modes on button up
    if (hidController(1)->rightButtons.down.changed == 1) {
        _driveMode = (_driveMode + 1) % 4;
    }
    return _driveMode;
}

void debugUpdate() {
    Controller *master = hidController(1);
    if (master->leftButtons.up.changed == 1 && _debugInterval > 1) {
        _debugInterval--;
    }
    if (master->leftButtons.down.changed == 1) {
        _debugInterval++;
    }
    if (++_debugCounter >= _debugInterval && master->leftButtons.left.pressed) {
        debugPrintState();
        _debugCounter = 0;
    }
}

void debugPrintState() {
    Controller *master = hidController(1);

    // printf("debug: %d  clock: %x\n", _debugInterval, (int) millis());
    // printf("turn-cc: %d\n", master->right.horz);

    // int battery = powerLevelMain();

    // Look at the Joystick.  Each control is numbered on the Joystick itself.
    // int joystick = JOYSTICK_MASTER;
    // int rVert = joystickGetAnalog(joystick, 2);
    // int rHorz = joystickGetAnalog(joystick, 1);

    // int lVert = joystickGetAnalog(joystick, 3);
    // int lHorz = joystickGetAnalog(joystick, 4);

    // int accelX = joystickGetAnalog(joystick, ACCEL_X);
    // int accelY = joystickGetAnalog(joystick, ACCEL_Y);

    // int ldDn = joystickGetDigital(joystick, 5, JOY_DOWN);
    // int ldUp = joystickGetDigital(joystick, 5, JOY_UP);
    //
    // int rdDn = joystickGetDigital(joystick, 6, JOY_DOWN);
    // int rdUp = joystickGetDigital(joystick, 6, JOY_UP);
    //
    // int luDn = joystickGetDigital(joystick, 7, JOY_DOWN);
    // int luUp = joystickGetDigital(joystick, 7, JOY_UP);
    // int luLt = joystickGetDigital(joystick, 7, JOY_LEFT);
    // int luRt = joystickGetDigital(joystick, 7, JOY_RIGHT);
    //
    // int ruDn = joystickGetDigital(joystick, 8, JOY_DOWN);
    // int ruUp = joystickGetDigital(joystick, 8, JOY_UP);
    // int ruLt = joystickGetDigital(joystick, 8, JOY_LEFT);
    // int ruRt = joystickGetDigital(joystick, 8, JOY_RIGHT);

    // printf("accel=%d,%d\n", accelX, accelY);
    // return;

    // printf("battery=%d, joyLeft=%d,%d joyRight=%d,%d, accel=%d,%d\n",
    // battery,
    // lVert, lHorz, rVert, rHorz, accelX, accelY);
    //  printf("battery=%d, joyLeft=%d,%d joyRight=%d,%d, accel=%d,%d, b5=%d%d b6=%d%d, b7=%d%d%d%d, b8=%d%d%d%d\n",
    // ldDn, ldUp, rdDn, rdUp,
    // luDn, luUp, luLt, luRt, ruDn, ruUp, ruLt, ruRt);
}

// Toggle the LED indicator to prove that we haven't crashed
void ledUpdate(unsigned long now) {
    static unsigned long _toggleIndicatorAt;
    if (_toggleIndicatorAt == 0 || now >= _toggleIndicatorAt) {
        digitalWrite(LED_GREEN, digitalRead(LED_GREEN) ? LOW : HIGH);
        _toggleIndicatorAt = now + 500;
    }
}

// Update the LCD (2 lines x 16 characters)
void lcdUpdate(const LcdInput *lcdInput, const Controller *joystick, DriveMode driveMode,
               unsigned long sleptAt, unsigned long now) {
    static int _displayMode = 0;
    static unsigned long _tempExpiresAt;

    unsigned int secSinceChange = (unsigned int) ((now - MAX(joystick->lastChangedTime, lcdInput->lastChangedTime)) / 1000);

    // The LCD shows two lines of 16 characters.  To help make sure that status messages fit w/in the 16
    // characters width of the display, there are comments below with 16 characters "0123456789abcdef" that
    // line up with a sample of status text.  By comparing the two, it's easy to see messages that are too long.

    // Left LCD button cycles through the display modes
    if (lcdInput->left.changed == 1) {
        _displayMode = (_displayMode + 1) % 6;
        _tempExpiresAt = now + 1000;
    }
    char line1[17] = "", line2[17] = "";
    int n = -1;
    if (calibration.mode == CALIBRATE_MOTOR_RPM) {
        // 0123456789abcdef
        // IN= -123   V=7.2
        // OUT=-123.234
        snprintf(line1, 17, "IN= %-4d   V=%.1f", calibration.input, powerLevelMain() / 1000.0);
        snprintf(line2, 17, "OUT=%-.3f", calibration.lastSpeed);

    } else if (_displayMode == ++n) {
        // 0123456789abcdef
        // ARCADE DRIVE
        // CONSTANT RADIUS
        // CPU=20 IDLE=99 /
        char* modeString;
        if (driveMode == DRIVE_TANK) {
            modeString = "TANK DRIVE";
        } else if (driveMode == DRIVE_ARCADE) {
            modeString = "ARCADE DRIVE";
        } else if (driveMode == DRIVE_CONSTANT_RADIUS) {
            modeString = "CONSTANT RADIUS";
        } else {
            modeString = "ACCELEROMETER";
        }
        unsigned int millisSinceSlept = (unsigned int) (now - sleptAt);  // how much idle time did the cpu have?
        int cpuUsage = (SLEEP_MILLIS - millisSinceSlept) * 100 / SLEEP_MILLIS;
        snprintf(line1, 17, "%s", modeString);
        snprintf(line2, 17, "CPU=%-2d IDLE=%d", MIN(cpuUsage, 99), MIN(secSinceChange, 99));

    } else if (_displayMode == ++n) {
        // Show estimated position (inches), heading (degrees) and linear velocity (inches/sec)
        if (now < _tempExpiresAt) {
            //                   0123456789abcdef
            snprintf(line1, 17, "Position");
            snprintf(line2, 17, "Heading, Speed");
        } else {
            // 0123456789abcdef
            // X+123.4 Y-123.4
            // A+123.4 V-123.4
            snprintf(line1, 17, "X%+4.1f Y%+4.1f", position.x, position.y);
            snprintf(line2, 17, "A%+4.1f V%+4.1f", position.a * DEGREES_PER_RADIAN, position.v);
        }

    } else if (_displayMode == ++n) {
        // Show estimated heading (degrees), angular velocity (degrees/sec), gyro heading (degrees)
        if (now < _tempExpiresAt) {
            //                   0123456789abcdef
            snprintf(line1, 17, "Heading, Turning");
            snprintf(line2, 17, "Gyro, RPM");
        } else {
            // 0123456789abcdef
            // A+123.4 W-123.4
            // G+123 RPM+123.4
            double speed = abs(position.vLeft) > abs(position.vRight) ? position.vLeft : position.vRight;  // faster side
            snprintf(line1, 17, "A%+4.1f W%+4.1f", position.a * DEGREES_PER_RADIAN, position.w * DEGREES_PER_RADIAN);
            snprintf(line2, 17, "G%+4d RPM%+4.1f", gyroGet(gyro), speed * (60 / (WHEEL_RADIUS * M_TWOPI)));  // convert to RPM
        }

    } else if (_displayMode == ++n) {
        if (now < _tempExpiresAt) {
            //                   0123456789abcdef
            snprintf(line1, 17, "Encoders");
        } else {
            // 0123456789abcdef
            // Left=  -123456
            // Right= +123456
            snprintf(line1, 17, "Left=  %-+6d", encoderGet(encoderLeft));
            snprintf(line2, 17, "Right= %-+6d", encoderGet(encoderRight));
        }

    } else if (_displayMode == ++n) {
        bool bumperLeft = digitalRead(BUMPER_LEFT);
        bool bumperRight = digitalRead(BUMPER_RIGHT);
        snprintf(line1, 17, "Bumpers");
        snprintf(line2, 17, "L=%d R=%d", bumperLeft, bumperRight);

    } else if (_displayMode == ++n) {
        if (now < _tempExpiresAt) {
            snprintf(line1, 17, "Battery Levels");
        } else {
            // 0123456789abcdef
            // Battery= 7.123
            // Backup=  9.123
            snprintf(line1, 17, "Battery= %.3f", powerLevelMain() / 1000.0);  // millivolts -> volts
            snprintf(line2, 17, "Backup=  %.3f", powerLevelBackup() / 1000.0);
        }
    }

    // Append the spinner to the end of the 2nd line to show that the LCD screen is alive
    for (int i = strlen(line2); i < 15; i++) {
        line2[i] = ' ';
    }
    line2[15] = _spinnerChars[(now / 250) % 4];  // rotate once per second
    line2[16] = 0;

    lcdSetText(LCD_PORT, 1, line1);
    lcdSetText(LCD_PORT, 2, line2);
    lcdSetBacklight(LCD_PORT, secSinceChange < 60);  // save battery: turn off the backlight after 60 seconds
}
