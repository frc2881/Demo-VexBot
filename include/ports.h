/** @file ports.h
 * @brief Robot Input/Output Port Mappings
 *
 * Defines all the ports currently in use on the robot.
 */

#ifndef ports_h
#define ports_h

#define LCD_PORT uart2

/* Motor output mappings.  Note: they're separated into banks 1-5 and 6-10, spread the load evenly. */
#define MOTOR_FLASHLIGHT 1    // plugged in as a 2-wire motor so it can be turned on and off
#define MOTOR_LEFT_F 2
#define MOTOR_RIGHT_F 3
#define MOTOR_RIGHT_R 8
#define MOTOR_LEFT_R 9

/* Analog input mappings */
#define GYRO 8

/* Digital input mappings.  Note: not allowed to use port 10 for encoders! */
#define BUMPER_RIGHT 1
#define BUMPER_LEFT 2
#define ENCODER_LEFT_BOTTOM 8
#define ENCODER_LEFT_TOP 9
#define ENCODER_RIGHT_TOP 11
#define ENCODER_RIGHT_BOTTOM 12

/* Digital output mappings */
#define LED_GREEN 4

extern Gyro gyro;
extern Encoder encoderLeft;
extern Encoder encoderRight;

#endif //ports_h
