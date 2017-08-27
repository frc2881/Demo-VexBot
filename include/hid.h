/** @file hid.c
 * @brief Human Input Device (HID) helper routines
 *
 * Manages a stable snapshot of the current state of the human input devices (the controllers and lcd buttons).
 * Adds change flags and tracks last changed timestamps for detecting button changes etc.
 */

#ifndef input_h
#define input_h

typedef struct {
    bool pressed;
    short changed;  // 1 immediately after press, -1 after release, 0 otherwise
} Button;

typedef struct {
    Button up;
    Button down;
} ButtonGroup2;

typedef struct {
    Button up;
    Button down;
    Button left;
    Button right;
} ButtonGroup4;

typedef struct {
    short vert;
    short horz;
    short vertScale;
    short horzScale;
} Joystick;

typedef struct {
    unsigned char port;
    // Joystick 3, 4
    Joystick left;
    // Joystick 2, 1
    Joystick right;
    // Joystick internal gyro (tilt forward/back, left/right)
    Joystick accel;
    // Button group 5
    ButtonGroup2 leftTrigger;
    // Button group 6
    ButtonGroup2 rightTrigger;
    // Button group 7
    ButtonGroup4 leftButtons;
    // Button group 8
    ButtonGroup4 rightButtons;
    // Time in millis when a controller value changed (except accelerometer)
    unsigned long lastChangedTime;
} Controller;

typedef struct {
    FILE* port;
    Button left;
    Button center;
    Button right;
    // Time in millis when an lcd button changed
    unsigned long lastChangedTime;
} LcdInput;

void hidInit(int numControllers, FILE *lcdPort);
void hidUpdate();

Controller *hidController(unsigned char controller);
LcdInput *hidLcd();

#endif
