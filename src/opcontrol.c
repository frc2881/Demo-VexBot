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
#include "input.h"
#include "main.h"
#include "motor.h"

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

int chooseControlMode();
void arcadeDrive(int power, int turnCC);
void tankDrive(int left, int right);
void mechArm(bool up, bool down);
void mechArmAnalog(int speed);
void mechClaw(bool open, bool close);
void mechClawAnalog(int speed);
void debugUpdate();
void debugPrintState();

static short s_debugCounter = 0;
static short s_debugInterval = 25;

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
//  taskCreate(logControls, TASK_DEFAULT_STACK_SIZE*2, 0, TASK_PRIORITY_LOWEST);

  inputInit();

  smartMotorInit();
  smartMotorReversed(MOTOR_RIGHT, true);
  smartMotorReversed(MOTOR_ARM, true);
  smartMotorSlew(MOTOR_ARM, 2, 10);  // Slow down the arm a lot since otherwise it's very jerky
  smartMotorSlew(MOTOR_CLAW, 2, 255);  // Slow down the claw

  while (1) {
    inputUpdate();
    INPUT_CONTROLLER* master = inputController(1);

		// What chassis control algorithm has the user selected?
    int controlMode = chooseControlMode();

		if (master->leftButtons4.up.pressed) {
			// Override, when pressed use joysticks to control mechanisms
			mechArmAnalog(master->right.vert);
			mechClawAnalog(master->left.vert);
		} else {
			// Normal, use joysticks to control drive train
	    if (controlMode == CONTROL_TANK) {
	      tankDrive(master->left.vert, master->right.vert);
	    } else if (controlMode == CONTROL_ARCADE) {
	      arcadeDrive(master->right.vert, master->right.horz);
	    } else {
	      arcadeDrive(master->accel.vert, master->accel.horz);
	    }
	    mechArm(master->rightButtons2.up.pressed, master->rightButtons2.down.pressed);
	    mechClaw(master->leftButtons2.up.pressed, master->leftButtons2.down.pressed);
		}

		smartMotorSlewEnabled(!master->rightButtons4.up.pressed);
    smartMotorUpdate();
    debugUpdate();
    delay(20);
  }
}

/*
 * Drives the chassis.  A positive 'turnCC' argument turns clockwise.
 */
void arcadeDrive(int power, int turnCC) {
  smartMotorSet(MOTOR_LEFT, power + turnCC);  // set left wheels
  smartMotorSet(MOTOR_RIGHT, power - turnCC);  // set right wheels
}

void tankDrive(int left, int right) {
  smartMotorSet(MOTOR_LEFT, left);  // set left wheels
  smartMotorSet(MOTOR_RIGHT, right);  // set right wheels
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
  if (inputController(1)->rightButtons4.down.changed == 1) {
    s_controlMode = (s_controlMode + 1) % 3;
  }
	return s_controlMode;
}

void debugUpdate() {
  INPUT_CONTROLLER* master = inputController(1);
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
  INPUT_CONTROLLER* master = inputController(1);

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
