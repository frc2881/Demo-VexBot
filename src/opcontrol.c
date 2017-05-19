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

#include <API.h>
#include <input.h>
#include "main.h"
#include "motor.h"

/* Motor output mappings */
#define MOTOR_LEFT_F 1
#define MOTOR_LEFT_R 2
#define MOTOR_CLAW 6
#define MOTOR_ARM 7
#define MOTOR_RIGHT_F 10
#define MOTOR_RIGHT_R 9

#define CONTROL_TANK 0
#define CONTROL_ARCADE 1
#define CONTROL_GRYO 2

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

int chooseControlMode();

void arcadeDrive(int power, int turnCC);

void tankDrive(int left, int right);

void mechArm(bool up, bool down);

void mechArmAnalog(int speed);

void mechClaw(bool open, bool close);

void mechClawAnalog(int speed);

void debugUpdate();

void debugPrintState();

void lcdUpdate(const LcdInput *lcdInput, const Controller *joystick,
               Ultrasonic sonarLeft, Ultrasonic sonarRight,
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
//  taskCreate(logControls, TASK_DEFAULT_STACK_SIZE*2, 0, TASK_PRIORITY_LOWEST);

    // Initialize input from Joystick 1 (and optionally LCD 1 buttons)
    hidInit(1, NULL);
//    hidInit(1, uart1);

//    LcdInput *lcdInput = hidLcd();
    Controller *joystick = hidController(1);
    joystick->accel.vertScale = -1;  // Tilt down moves forward, tilt up moves back

    smartMotorInit();
    smartMotorReversed(MOTOR_RIGHT_F, true);
    smartMotorReversed(MOTOR_RIGHT_R, true);
    smartMotorReversed(MOTOR_ARM, true);
//    smartMotorSlew(MOTOR_ARM, 0.1, 0.5);  // Slow down the arm a lot since otherwise it's very jerky
//    smartMotorSlew(MOTOR_CLAW, 0.1, 100);  // Slow down the claw

//    Ultrasonic sonarLeft = ultrasonicInit(4, 3);
//    Ultrasonic sonarRight = ultrasonicInit(2, 1);

    unsigned long previousWakeTime = millis();
    unsigned long sleptAt = millis();
    while (true) {
        unsigned long now = millis();

        hidUpdate();

        // What chassis control algorithm has the user selected?
        int controlMode = chooseControlMode();

        if (joystick->leftButtons4.up.pressed) {
            // Override, when pressed use joysticks to control mechanisms
            mechArmAnalog(joystick->right.vert);
            mechClawAnalog(joystick->left.vert);
        } else {
            // Normal, use joysticks to control drive train
            if (controlMode == CONTROL_TANK) {
                tankDrive(joystick->left.vert, joystick->right.vert);
            } else if (controlMode == CONTROL_ARCADE) {
                arcadeDrive(joystick->right.vert, joystick->right.horz);
            } else {
                arcadeDrive(joystick->accel.vert, joystick->accel.horz);
            }
            mechArm(joystick->rightButtons2.up.pressed, joystick->rightButtons2.down.pressed);
            mechClaw(joystick->leftButtons2.up.pressed, joystick->leftButtons2.down.pressed);
        }

        // Experimental pneumatics
//        digitalWrite(1, joystick->rightButtons4.left.pressed);

        smartMotorSlewEnabled(!joystick->rightButtons4.up.pressed);
        smartMotorUpdate();

//        lcdUpdate(lcdInput, joystick, sonarLeft, sonarRight, sleptAt, now);

        debugUpdate();

        // Sleep for a while, give other tasks a chance t orun
        sleptAt = millis();
        taskDelayUntil(&previousWakeTime, 20);
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

void tankDrive(int left, int right) {
    smartMotorSet(MOTOR_LEFT_F, left);  // set left-front wheel(s)
    smartMotorSet(MOTOR_LEFT_R, left);  // set left-rear wheel(s)
    smartMotorSet(MOTOR_RIGHT_F, right);  // set right wheel(s)
    smartMotorSet(MOTOR_RIGHT_R, right);  // set right wheel(s)
}

void mechArm(bool up, bool down) {
    smartMotorSet(MOTOR_ARM, up * 127 - down * 127);
}

void mechArmAnalog(int speed) {
    smartMotorSet(MOTOR_ARM, speed);
}

void mechClaw(bool open, bool close) {
    smartMotorSet(MOTOR_CLAW, open * 127 - close * 127);
}

void mechClawAnalog(int speed) {
    smartMotorSet(MOTOR_CLAW, speed);
}

int chooseControlMode() {
    static int s_controlMode = 0;
    // button '8 down' cycles through control modes on button up
    if (hidController(1)->rightButtons4.down.changed == 1) {
        s_controlMode = (s_controlMode + 1) % 3;
    }
    return s_controlMode;
}

void debugUpdate() {
    Controller *master = hidController(1);
    if (master->leftButtons4.up.changed == 1 && s_debugInterval > 1) {
        s_debugInterval--;
    }
    if (master->leftButtons4.down.changed == 1) {
        s_debugInterval++;
    }
    if (++s_debugCounter >= s_debugInterval && master->leftButtons4.left.pressed) {
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

// Update the LCD (2 lines x 16 characters)
void lcdUpdate(const LcdInput *lcdInput, const Controller *joystick,
               Ultrasonic sonarLeft, Ultrasonic sonarRight,
               unsigned long sleptAt, unsigned long now) {

    if (now - lcdInput->lastChangedTime < 3000) {
        // Show which LCD buttons have been pressed
        // 0123456789abcdef
        // LCD L=0 M=1 R=1
        lcdPrint(uart1, 1, "LCD:  %c %c %c", lcdInput->left.pressed ? 'L' : ' ',
                 lcdInput->center.pressed ? 'M' : ' ', lcdInput->right.pressed ? 'R' : ' ');
    } else {
        // Show ultrasonic left & right
        // 0123456789abcdef
        // L=200  R=200
        int distLeft = ultrasonicGet(sonarLeft);
        int distRight = ultrasonicGet(sonarRight);
        lcdPrint(uart1, 1, "L=%3d  R=%3d", distLeft, distRight);
    }

    // 0123456789abcdef
    // W=20  CH=12345 *
    unsigned int millisSinceSlept = (unsigned int) (now - sleptAt);  // how much idle time did the cpu have?
    unsigned int secSinceChange = (unsigned int) ((now - MAX(joystick->lastChangedTime, lcdInput->lastChangedTime)) / 1000);
    char spinner = _spinnerChars[(now / 250) % 4];  // rotate once per second
    lcdPrint(uart1, 2, "W=%-3u CH=%-5u %c", millisSinceSlept, secSinceChange, spinner);
    lcdSetBacklight(uart1, secSinceChange < 60);  // save battery: turn off the backlight after 60 seconds
}
