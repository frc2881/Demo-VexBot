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

#include "main.h"

/* The first joystick (usually the only joystick). */
#define JOYSTICK_MASTER 1
/* The partner joystick (optional, for two driver control). */
#define JOYSTICK_PARTER 2

/* Motor output mappings */
#define MOTOR_LEFT 1
#define MOTOR_CLAW 6
#define MOTOR_ARM 7
#define MOTOR_RIGHT 10

#define CONTROL_TANK 0
#define CONTROL_ARCADE 1
#define CONTROL_GRYO 2

void chooseControlState();
void arcadeDrive(int power, int turnCC);
void tankDrive(int left, int right);
void mechArm(bool up, bool down);
void mechClaw(bool open, bool close);
void debugState();

static short s_controlMode = 0;
static bool s_lastControlButton = 0;

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
void operatorControl() {
	int counter = 0;

//  taskCreate(logControls, TASK_DEFAULT_STACK_SIZE*2, 0, TASK_PRIORITY_LOWEST);

	while (1) {
    chooseControlState();

		if (s_controlMode == CONTROL_TANK) {
			tankDrive(joystickGetAnalog(JOYSTICK_MASTER, 3), joystickGetAnalog(JOYSTICK_MASTER, 2));
		} else if (s_controlMode == CONTROL_ARCADE) {
			arcadeDrive(joystickGetAnalog(JOYSTICK_MASTER, 2), joystickGetAnalog(JOYSTICK_MASTER, 1));
		} else {
			arcadeDrive(-joystickGetAnalog(JOYSTICK_MASTER, ACCEL_X), joystickGetAnalog(JOYSTICK_MASTER, ACCEL_Y));
		}

    mechArm(joystickGetDigital(JOYSTICK_MASTER, 6, JOY_UP), joystickGetDigital(JOYSTICK_MASTER, 6, JOY_DOWN));
		mechClaw(joystickGetDigital(JOYSTICK_MASTER, 5, JOY_UP), joystickGetDigital(JOYSTICK_MASTER, 5, JOY_DOWN));

		if (++counter == 20) {
			debugState();
			counter = 0;
		}

		delay(20);
	}
}

/*
 * Drives the chassis.  A positive 'turnCC' argument turns clockwise.
 */
void arcadeDrive(int power, int turnCC) {
	motorSet(MOTOR_LEFT, power + turnCC);  // set left wheels
	motorSet(MOTOR_RIGHT, -(power - turnCC));  // set right wheels (reversed)
}

void tankDrive(int left, int right) {
	motorSet(MOTOR_LEFT, left);  // set left wheels
	motorSet(MOTOR_RIGHT, -right);  // set right wheels (reversed)
}

void mechArm(bool up, bool down) {
	motorSet(MOTOR_ARM, -(up * 127 - down * 127));
}

void mechClaw(bool open, bool close) {
	motorSet(MOTOR_CLAW, open * 127 - close * 127);
}

void chooseControlState() {
	// button '8 down' cycles through control modes
  bool controlButton = joystickGetDigital(JOYSTICK_MASTER, 8, JOY_DOWN);
	if (s_lastControlButton != controlButton) {
		if (s_lastControlButton && !controlButton) {
			s_controlMode = (s_controlMode + 1) % 3;
		}
		s_lastControlButton = controlButton;
	}
}

void debugState() {
	int battery = powerLevelMain();

	// Look at the Joystick.  Each control is numbered on the Joystick itself.
	int joystick = JOYSTICK_MASTER;
	int rVert = joystickGetAnalog(joystick, 2);
	int rHorz = joystickGetAnalog(joystick, 1);

	int lVert = joystickGetAnalog(joystick, 3);
	int lHorz = joystickGetAnalog(joystick, 4);

	int accelX = joystickGetAnalog(joystick, ACCEL_X);
	int accelY = joystickGetAnalog(joystick, ACCEL_Y);

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

	printf("accel=%d,%d\n", accelX, accelY);
	return;

	printf("battery=%d, joyLeft=%d,%d joyRight=%d,%d, accel=%d,%d\n",
		battery,
		lVert, lHorz, rVert, rHorz, accelX, accelY);
		//	printf("battery=%d, joyLeft=%d,%d joyRight=%d,%d, accel=%d,%d, b5=%d%d b6=%d%d, b7=%d%d%d%d, b8=%d%d%d%d\n",
		// ldDn, ldUp, rdDn, rdUp,
		// luDn, luUp, luLt, luRt, ruDn, ruUp, ruLt, ruRt);
}
