/** @file init.c
 * @brief File for initialization code
 *
 * This file should contain the user initialize() function and any functions related to it.
 *
 * Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 *
 * PROS contains FreeRTOS (http://www.freertos.org) whose source code may be
 * obtained from http://sourceforge.net/projects/freertos/files/ or on request.
 */

#include <API.h>
#include "main.h"
#include "motor.h"
#include "ports.h"

// All sensors are declared here since this is where they're initialized
Gyro gyro;
Encoder encoderLeft;
Encoder encoderRight;

/*
 * Runs pre-initialization code. This function will be started in kernel mode one time while the
 * VEX Cortex is starting up. As the scheduler is still paused, most API functions will fail.
 *
 * The purpose of this function is solely to set the default pin modes (pinMode()) and port
 * states (digitalWrite()) of limit switches, push buttons, and solenoids. It can also safely
 * configure a UART port (usartOpen()) but cannot set up an LCD (lcdInit()).
 */
void initializeIO() {
    // Reset the Cortex if static shock etc. causes the Cortex to lock up.
    watchdogInit();

    pinMode(BUMPER_LEFT, INPUT);
    pinMode(BUMPER_RIGHT, INPUT);

    // Initialize output pins (pneumatics, leds, etc.)
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, HIGH);
}

/*
 * Runs user initialization code. This function will be started in its own task with the default
 * priority and stack size once when the robot is starting up. It is possible that the VEXnet
 * communication link may not be fully established at this time, so reading from the VEX
 * Joystick may fail.
 *
 * This function should initialize most sensors (gyro, encoders, ultrasonics), LCDs, global
 * variables, and IMEs.
 *
 * This function must exit relatively promptly, or the operatorControl() and autonomous() tasks
 * will not start. An autonomous mode selection menu like the pre_auton() in other environments
 * can be implemented in this task if desired.
 */
void initialize() {
    lcdInit(LCD_PORT);
    lcdClear(LCD_PORT);
    lcdSetBacklight(LCD_PORT, true);

    //    piInit(RASPBERRY_PI_PORT);

    gyro = gyroInit(GYRO, 196);  // 196 is the default, tweak if Gyro appears to under or over-report rotation

    encoderLeft = encoderInit(ENCODER_LEFT_TOP, ENCODER_LEFT_BOTTOM, false);
    encoderRight = encoderInit(ENCODER_RIGHT_TOP, ENCODER_RIGHT_BOTTOM, true);

//  sonarLeft = ultrasonicInit(4, 3);
//  sonarRight = ultrasonicInit(2, 1);

    smartMotorInit();
    smartMotorReversed(MOTOR_RIGHT_F, true);
    smartMotorReversed(MOTOR_RIGHT_R, true);
}
