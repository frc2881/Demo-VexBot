#include <API.h>
#include <input.h>

#define MAX_CONTROLLERS 2

static int _inputNumControllers;
static Controller _inputController[MAX_CONTROLLERS];
static LcdInput _inputLcd;

void inputInit(int numControllers, FILE *lcdPort) {
    _inputNumControllers = numControllers;
    for (unsigned char i = 0; i < numControllers; i++) {
        Controller *input = &_inputController[i];
        input->port = i + 1;
        input->left.horzScale = input->left.vertScale = 1;
        input->right.horzScale = input->right.vertScale = 1;
        input->accel.horzScale = input->accel.vertScale = 1;
        input->lastChangedTime = millis();
    }
    _inputLcd.port = lcdPort;
    _inputLcd.lastChangedTime = millis();
}

inline void _inputSetButton(Button *input, bool newValue, bool *changed) {
    *changed |= input->pressed != newValue;
    input->changed = newValue - input->pressed;
    input->pressed = newValue;
}

inline void _inputSetAnalog(short *input, short newValue, bool *changed) {
    *changed |= *input != newValue;
    *input = newValue;
}

void _inputUpdateJoystick(Joystick *input, unsigned char port, unsigned char vert, unsigned char horz, bool *changed) {
    _inputSetAnalog(&input->vert, joystickGetAnalog(port, vert) * input->vertScale, changed);
    _inputSetAnalog(&input->horz, joystickGetAnalog(port, horz) * input->horzScale, changed);
}

void _inputUpdateButton(Button *input, unsigned char port, unsigned char group, unsigned char button, bool *changed) {
    _inputSetButton(input, joystickGetDigital(port, group, button), changed);
}

void _inputUpdateGroup2(ButtonGroup2 *input, unsigned char port, unsigned char group, bool *changed) {
    _inputUpdateButton(&input->up, port, group, JOY_UP, changed);
    _inputUpdateButton(&input->down, port, group, JOY_DOWN, changed);
}

void _inputUpdateGroup4(ButtonGroup4 *input, unsigned char port, unsigned char group, bool *changed) {
    _inputUpdateButton(&input->up, port, group, JOY_UP, changed);
    _inputUpdateButton(&input->down, port, group, JOY_DOWN, changed);
    _inputUpdateButton(&input->left, port, group, JOY_LEFT, changed);
    _inputUpdateButton(&input->right, port, group, JOY_RIGHT, changed);
}

void _inputUpdateController(Controller *input) {
    bool changed = false;
    unsigned char port = input->port;
    _inputUpdateJoystick(&input->left, port, 3, 4, &changed);
    _inputUpdateJoystick(&input->right, port, 2, 1, &changed);
    _inputUpdateGroup2(&input->leftButtons2, port, 5, &changed);
    _inputUpdateGroup2(&input->rightButtons2, port, 6, &changed);
    _inputUpdateGroup4(&input->leftButtons4, port, 7, &changed);
    _inputUpdateGroup4(&input->rightButtons4, port, 8, &changed);
    if (changed) {
        input->lastChangedTime = millis();
    }
    // Ignore 'changed' on the accelerometer--too many false positives
    _inputUpdateJoystick(&input->accel, port, ACCEL_X, ACCEL_Y, &changed);
}

void _inputUpdateLcd(LcdInput *lcd) {
    bool changed = false;
    unsigned int buttons = lcdReadButtons(lcd->port);
    _inputSetButton(&lcd->left, (buttons & LCD_BTN_LEFT) != 0, &changed);
    _inputSetButton(&lcd->center, (buttons & LCD_BTN_CENTER) != 0, &changed);
    _inputSetButton(&lcd->right, (buttons & LCD_BTN_RIGHT) != 0, &changed);
    if (changed) {
        lcd->lastChangedTime = millis();
    }
}

void inputUpdate() {
    for (unsigned char i = 0; i < _inputNumControllers; i++) {
        _inputUpdateController(&_inputController[i]);
    }
    if (_inputLcd.port) {
        _inputUpdateLcd(&_inputLcd);
    }
}

Controller *inputController(unsigned char controller) {
    if (controller < 1 || controller > _inputNumControllers) {
        controller = 1;
    }
    return &_inputController[controller - 1];
}

LcdInput *inputLcd() {
    return &_inputLcd;
}
