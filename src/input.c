#include <API.h>
#include <input.h>

//
// Human Interface Device (HID) helper routines
//

#define MAX_CONTROLLERS 2

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

inline void _hidSetButton(Button *input, bool newValue, bool *changed) {
    *changed |= input->pressed != newValue;
    input->changed = newValue - input->pressed;
    input->pressed = newValue;
}

inline void _hidSetAnalog(short *input, short newValue, bool *changed) {
    *changed |= *input != newValue;
    *input = newValue;
}

void _hidUpdateJoystick(Joystick *input, unsigned char port, unsigned char vert, unsigned char horz, bool *changed) {
    _hidSetAnalog(&input->vert, joystickGetAnalog(port, vert) * input->vertScale, changed);
    _hidSetAnalog(&input->horz, joystickGetAnalog(port, horz) * input->horzScale, changed);
}

void _hidUpdateButton(Button *input, unsigned char port, unsigned char group, unsigned char button, bool *changed) {
    _hidSetButton(input, joystickGetDigital(port, group, button), changed);
}

void _hidUpdateGroup2(ButtonGroup2 *input, unsigned char port, unsigned char group, bool *changed) {
    _hidUpdateButton(&input->up, port, group, JOY_UP, changed);
    _hidUpdateButton(&input->down, port, group, JOY_DOWN, changed);
}

void _hidUpdateGroup4(ButtonGroup4 *input, unsigned char port, unsigned char group, bool *changed) {
    _hidUpdateButton(&input->up, port, group, JOY_UP, changed);
    _hidUpdateButton(&input->down, port, group, JOY_DOWN, changed);
    _hidUpdateButton(&input->left, port, group, JOY_LEFT, changed);
    _hidUpdateButton(&input->right, port, group, JOY_RIGHT, changed);
}

void _hidUpdateController(Controller *input) {
    bool changed = false;
    unsigned char port = input->port;
    _hidUpdateJoystick(&input->left, port, 3, 4, &changed);
    _hidUpdateJoystick(&input->right, port, 2, 1, &changed);
    _hidUpdateGroup2(&input->leftButtons2, port, 5, &changed);
    _hidUpdateGroup2(&input->rightButtons2, port, 6, &changed);
    _hidUpdateGroup4(&input->leftButtons4, port, 7, &changed);
    _hidUpdateGroup4(&input->rightButtons4, port, 8, &changed);
    if (changed) {
        input->lastChangedTime = millis();
    }
    // Ignore 'changed' on the accelerometer--too many false positives
    _hidUpdateJoystick(&input->accel, port, ACCEL_X, ACCEL_Y, &changed);
}

void _hidUpdateLcd(LcdInput *lcd) {
    bool changed = false;
    unsigned int buttons = lcdReadButtons(lcd->port);
    _hidSetButton(&lcd->left, (buttons & LCD_BTN_LEFT) != 0, &changed);
    _hidSetButton(&lcd->center, (buttons & LCD_BTN_CENTER) != 0, &changed);
    _hidSetButton(&lcd->right, (buttons & LCD_BTN_RIGHT) != 0, &changed);
    if (changed) {
        lcd->lastChangedTime = millis();
    }
}

void hidUpdate() {
    for (unsigned char i = 0; i < _hidNumControllers; i++) {
        _hidUpdateController(&_hidController[i]);
    }
    if (_hidLcd.port) {
        _hidUpdateLcd(&_hidLcd);
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
