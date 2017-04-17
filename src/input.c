#include <API.h>
#include "input.h"

// Just one controller for now
static INPUT_CONTROLLER s_inputController[1];

void inputInit() {
  for (unsigned char controller = 1; controller <= 1; controller++) {
    INPUT_CONTROLLER* input = &s_inputController[controller - 1];
    input->left.horzScale = input->left.vertScale = 1;
    input->right.horzScale = input->right.vertScale = 1;
    input->accel.horzScale = 1;
    input->accel.vertScale = -1;  // so tilt down moves forward, tilt up moves back
  }
}

void inputUpdateJoystick(INPUT_JOYSTICK* input, unsigned char joystick,
                          unsigned char vert, unsigned char horz) {
  input->vert = joystickGetAnalog(joystick, vert) * input->vertScale;
  input->horz = joystickGetAnalog(joystick, horz) * input->horzScale;
}

void inputUpdateButton(INPUT_BUTTON* input, unsigned char joystick,
                       unsigned char buttonGroup, unsigned char button) {
  bool previous = input->pressed;
  bool current = joystickGetDigital(joystick, buttonGroup, button);
  input->pressed = current;
  input->changed = current - previous;
}

void inputUpdateGroup2(INPUT_GROUP_2* input, unsigned char joystick,
                       unsigned char buttonGroup) {
  inputUpdateButton(&input->up, joystick, buttonGroup, JOY_UP);
  inputUpdateButton(&input->down, joystick, buttonGroup, JOY_DOWN);
}

void inputUpdateGroup4(INPUT_GROUP_4* input, unsigned char joystick,
                       unsigned char buttonGroup) {
  inputUpdateButton(&input->up, joystick, buttonGroup, JOY_UP);
  inputUpdateButton(&input->down, joystick, buttonGroup, JOY_DOWN);
  inputUpdateButton(&input->left, joystick, buttonGroup, JOY_LEFT);
  inputUpdateButton(&input->right, joystick, buttonGroup, JOY_RIGHT);
}

void inputUpdateController(INPUT_CONTROLLER* input, unsigned char joystick) {
  inputUpdateJoystick(&input->left, joystick, 3, 4);
  inputUpdateJoystick(&input->right, joystick, 2, 1);
  inputUpdateJoystick(&input->accel, joystick, ACCEL_X, ACCEL_Y);
  inputUpdateGroup2(&input->leftButtons2, joystick, 5);
  inputUpdateGroup2(&input->rightButtons2, joystick, 6);
  inputUpdateGroup4(&input->leftButtons4, joystick, 7);
  inputUpdateGroup4(&input->rightButtons4, joystick, 8);
}

void inputUpdate() {
  for (unsigned char controller = 1; controller <= 1; controller++) {
    INPUT_CONTROLLER* input = &s_inputController[controller - 1];
    inputUpdateController(input, controller);
  }
}

INPUT_CONTROLLER* inputController(unsigned char controller) {
  if (controller < 1 || controller > 1) {
    controller = 1;
  }
  return &s_inputController[controller - 1];
}
