/** @file hid.c
 * @brief Human Input Device (HID) helper routines
 *
 * Manages a stable snapshot of the current state of the human input devices (the controllers and lcd buttons).
 * Adds change flags and tracks last changed timestamps for detecting button changes etc.
 */

#include <API.h>
#include "hid.h"

#define MAX_CONTROLLERS 2

static void hidUpdateLcd(LcdInput *lcd);
static void hidUpdateController(Controller *input);
static void hidUpdateJoystick(Joystick *input, unsigned char port, unsigned char vert, unsigned char horz, bool *changed);
static void hidUpdateGroup2(ButtonGroup2 *input, unsigned char port, unsigned char group, bool *changed);
static void hidUpdateGroup4(ButtonGroup4 *input, unsigned char port, unsigned char group, bool *changed);
static void hidUpdateButton(Button *input, unsigned char port, unsigned char group, unsigned char button, bool *changed);

static int _hidNumControllers;
static Controller _hidController[MAX_CONTROLLERS];
static LcdInput _hidLcd;

void hidInit(int numControllers, FILE *lcdPort) {
    _hidNumControllers = numControllers;
    for (unsigned char i = 0; i < numControllers; i++) {
        Controller *input = &_hidController[i];
        input->port = i + 1;
        input->left.horzScale = input->left.vertScale = 1;
        input->right.horzScale = input->right.vertScale = 1;
        input->accel.horzScale = input->accel.vertScale = 1;
        input->lastChangedTime = millis();
    }
    if (lcdPort) {
        _hidLcd.port = lcdPort;
        _hidLcd.lastChangedTime = millis();
    }
}

void hidUpdate() {
    for (unsigned char i = 0; i < _hidNumControllers; i++) {
        hidUpdateController(&_hidController[i]);
    }
    if (_hidLcd.port) {
        hidUpdateLcd(&_hidLcd);
    }
}

Controller *hidController(unsigned char controller) {
    if (controller < 1 || controller > _hidNumControllers) {
        controller = 1;
    }
    return &_hidController[controller - 1];
}

LcdInput *hidLcd() {
    return &_hidLcd;
}

static inline void hidSetButton(Button *input, bool newValue, bool *changed) {
    *changed |= input->pressed != newValue;
    input->changed = newValue - input->pressed;
    input->pressed = newValue;
}

static inline void hidSetAnalog(short *input, short newValue, bool *changed) {
    *changed |= *input != newValue;
    *input = newValue;
}

static void hidUpdateController(Controller *input) {
    bool changed = false;
    unsigned char port = input->port;
    hidUpdateJoystick(&input->left, port, 3, 4, &changed);
    hidUpdateJoystick(&input->right, port, 2, 1, &changed);
    hidUpdateGroup2(&input->leftTrigger, port, 5, &changed);
    hidUpdateGroup2(&input->rightTrigger, port, 6, &changed);
    hidUpdateGroup4(&input->leftButtons, port, 7, &changed);
    hidUpdateGroup4(&input->rightButtons, port, 8, &changed);
    if (changed) {
        input->lastChangedTime = millis();
    }
    // Ignore 'changed' on the accelerometer--too many false positives
    hidUpdateJoystick(&input->accel, port, ACCEL_X, ACCEL_Y, &changed);
}

static void hidUpdateJoystick(Joystick *input, unsigned char port, unsigned char vert, unsigned char horz, bool *changed) {
    hidSetAnalog(&input->vert, joystickGetAnalog(port, vert) * input->vertScale, changed);
    hidSetAnalog(&input->horz, joystickGetAnalog(port, horz) * input->horzScale, changed);
}

static void hidUpdateGroup2(ButtonGroup2 *input, unsigned char port, unsigned char group, bool *changed) {
    hidUpdateButton(&input->up, port, group, JOY_UP, changed);
    hidUpdateButton(&input->down, port, group, JOY_DOWN, changed);
}

static void hidUpdateGroup4(ButtonGroup4 *input, unsigned char port, unsigned char group, bool *changed) {
    hidUpdateButton(&input->up, port, group, JOY_UP, changed);
    hidUpdateButton(&input->down, port, group, JOY_DOWN, changed);
    hidUpdateButton(&input->left, port, group, JOY_LEFT, changed);
    hidUpdateButton(&input->right, port, group, JOY_RIGHT, changed);
}

static void hidUpdateButton(Button *input, unsigned char port, unsigned char group, unsigned char button, bool *changed) {
    hidSetButton(input, joystickGetDigital(port, group, button), changed);
}

static void hidUpdateLcd(LcdInput *lcd) {
    bool changed = false;
    unsigned int buttons = lcdReadButtons(lcd->port);
    hidSetButton(&lcd->left, (buttons & LCD_BTN_LEFT) != 0, &changed);
    hidSetButton(&lcd->center, (buttons & LCD_BTN_CENTER) != 0, &changed);
    hidSetButton(&lcd->right, (buttons & LCD_BTN_RIGHT) != 0, &changed);
    if (changed) {
        lcd->lastChangedTime = millis();
    }
}
