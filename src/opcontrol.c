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
#include "hid.h"
#include "motor.h"
#include "ports.h"
#include "tracking.h"

#define CONTROL_TANK 0
#define CONTROL_ARCADE 1
#define CONTROL_ACCEL 2

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define SLEEP_MILLIS 20

int chooseControlMode();

void arcadeDrive(int power, int turnCC);

void tankDrive(int left, int right, bool squared);

void debugUpdate();

void debugPrintState();

void ledUpdate(unsigned long now);

void lcdUpdate(const LcdInput *lcdInput, const Controller *joystick, int controlMode,
               unsigned long sleptAt, unsigned long now);

static short s_debugCounter = 0;
static short s_debugInterval = 25;

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
    LcdInput *lcdInput = hidLcd();
    Controller *joystick = hidController(1);
    joystick->accel.vertScale = -1;  // Tilt down moves forward, tilt up moves back

    unsigned long previousWakeTime = millis();
    unsigned long sleptAt = millis();
    while (true) {
        unsigned long now = millis();

        positionUpdate(now);
        hidUpdate();

        // What chassis control algorithm has the user selected?
        int controlMode = chooseControlMode();

        // Use joysticks to control drive train
        bool squared = joystick->rightButtons.right.pressed;
        if (controlMode == CONTROL_TANK) {
            tankDrive(joystick->left.vert, joystick->right.vert, squared);
        } else if (controlMode == CONTROL_ARCADE) {
            arcadeDrive(joystick->right.vert, joystick->left.horz);
        } else {
            arcadeDrive(joystick->accel.vert, joystick->accel.horz);
        }

        // Experimental pneumatics
//        digitalWrite(1, joystick->rightButtons.left.pressed);

        // Disable smart motor slew control with a button press
        smartMotorSlewEnabled(!joystick->rightButtons.up.pressed);

        // Apply desired drive settings to all motors
        smartMotorUpdate();

        // Show status
        ledUpdate(now);
        lcdUpdate(lcdInput, joystick, controlMode, sleptAt, now);
        debugUpdate();

        // Sleep for a while, give other tasks a chance to run
        sleptAt = millis();
        taskDelayUntil(&previousWakeTime, SLEEP_MILLIS);
    }
}

#pragma clang diagnostic pop

/*
 * Drives the chassis.  A positive 'turnCC' argument turns clockwise.
 */
void arcadeDrive(int power, int turnCC) {
    smartMotorSet(MOTOR_LEFT_F, power + turnCC);  // set left-front wheel(s)
    smartMotorSet(MOTOR_LEFT_R, power + turnCC);  // set left-rear wheel(s)
    smartMotorSet(MOTOR_RIGHT_F, power - turnCC);  // set right wheel(s)
    smartMotorSet(MOTOR_RIGHT_R, power - turnCC);  // set right wheel(s)
}

void tankDrive(int left, int right, bool squared) {
    if (squared) {
        left = left * left / 255;
        right = right * right / 255;
    }
    smartMotorSet(MOTOR_LEFT_F, left);  // set left-front wheel(s)
    smartMotorSet(MOTOR_LEFT_R, left);  // set left-rear wheel(s)
    smartMotorSet(MOTOR_RIGHT_F, right);  // set right wheel(s)
    smartMotorSet(MOTOR_RIGHT_R, right);  // set right wheel(s)
}

int chooseControlMode() {
    static int s_controlMode = CONTROL_TANK;
    // button '8 down' cycles through control modes on button up
    if (hidController(1)->rightButtons.down.changed == 1) {
        s_controlMode = (s_controlMode + 1) % 3;
    }
    return s_controlMode;
}

void debugUpdate() {
    Controller *master = hidController(1);
    if (master->leftButtons.up.changed == 1 && s_debugInterval > 1) {
        s_debugInterval--;
    }
    if (master->leftButtons.down.changed == 1) {
        s_debugInterval++;
    }
    if (++s_debugCounter >= s_debugInterval && master->leftButtons.left.pressed) {
        debugPrintState();
        s_debugCounter = 0;
    }
}

void debugPrintState() {
    Controller *master = hidController(1);

    // printf("debug: %d  clock: %x\n", s_debugInterval, (int) millis());
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
    static unsigned long toggleIndicatorAt;
    if (toggleIndicatorAt == 0 || now >= toggleIndicatorAt) {
        digitalWrite(LED_GREEN, digitalRead(LED_GREEN) ? LOW : HIGH);
        toggleIndicatorAt = now + 500;
    }
}

// Update the LCD (2 lines x 16 characters)
void lcdUpdate(const LcdInput *lcdInput, const Controller *joystick, int controlMode,
               unsigned long sleptAt, unsigned long now) {
    static int displayMode = 1;  // TODO
    static unsigned long tempExpiresAt;

    unsigned int secSinceChange = (unsigned int) ((now - MAX(joystick->lastChangedTime, lcdInput->lastChangedTime)) / 1000);

    // Left LCD button cycles through the display modes
    if (lcdInput->left.changed == 1) {
        displayMode = (displayMode + 1) % 6;
        tempExpiresAt = now + 1000;
    }
    char line1[17] = "", line2[17] = "";
    int n = -1;
    if (displayMode == ++n) {
        // 0123456789abcdef
        // ARCADE DRIVE
        // CPU=20 IDLE=99 *
        char* modeString = (controlMode == CONTROL_TANK) ? "TANK" : (controlMode == CONTROL_ARCADE) ? "ARCADE" : "ACCEL";
        unsigned int millisSinceSlept = (unsigned int) (now - sleptAt);  // how much idle time did the cpu have?
        int cpuUsage = (SLEEP_MILLIS - millisSinceSlept) * 100 / SLEEP_MILLIS;
        snprintf(line1, 17, "%s DRIVE", modeString);
        snprintf(line2, 17, "CPU=%-2d IDLE=%d", MIN(cpuUsage, 99), MIN(secSinceChange, 99));

    } else if (displayMode == ++n) {
        // Show estimated position (inches), heading (degrees) and linear velocity (inches/sec)
        if (now < tempExpiresAt) {
            // 0123456789abcdef
            // Position
            // Heading Speed  /
            snprintf(line1, 17, "Position");
            snprintf(line2, 17, "Heading, RPM");
        } else {
            // 0123456789abcdef
            // X+123.4 Y-123.4
            // A+123.4 V-123.4
            double rpm = position.v * 60 / (M_TWOPI * WHEEL_RADIUS);  // convert inch/second to rotation/minute
            snprintf(line1, 17, "X%+4.1f Y%+4.1f", position.x, position.y);
            snprintf(line2, 17, "A%+4.1f V%+4.1f", position.a * RADIANS_TO_DEGREES, rpm);
        }

    } else if (displayMode == ++n) {
        // Show estimated heading (degrees), angular velocity (degrees/sec), gyro heading (degrees)
        if (now < tempExpiresAt) {
            // 0123456789abcdef
            // Heading Turning
            // Gyro
            snprintf(line1, 17, "Heading Turning");
            snprintf(line2, 17, "Gyro");
        } else {
            // 0123456789abcdef
            // A+123.4 W-123.4
            // G+123.4
            snprintf(line1, 17, "A%+4.1f W%+4.1f", position.a * RADIANS_TO_DEGREES, position.w * RADIANS_TO_DEGREES);
            snprintf(line2, 17, "G%+4d", gyroGet(gyro));
        }

    } else if (displayMode == ++n) {
        if (now < tempExpiresAt) {
            snprintf(line1, 17, "Encoders");
        } else {
            // 0123456789abcdef
            // Left=  -123456
            // Right= +123456
            snprintf(line1, 17, "Left=  %-+6d", encoderGet(encoderLeft));
            snprintf(line2, 17, "Right= %-+6d", encoderGet(encoderRight));
        }

    } else if (displayMode == ++n) {
        bool bumperLeft = digitalRead(BUMPER_LEFT);
        bool bumperRight = digitalRead(BUMPER_RIGHT);
        snprintf(line1, 17, "Bumpers");
        snprintf(line2, 17, "L=%d R=%d", bumperLeft, bumperRight);

    } else if (displayMode == ++n) {
        if (now < tempExpiresAt) {
            snprintf(line1, 17, "Battery Levels");
        } else {
            // 0123456789abcdef
            // Battery= 7.123
            // Backup=  9.123
            snprintf(line1, 17, "Battery= %.3f", powerLevelMain() / 1000.0);
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
