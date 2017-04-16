#include <API.h>
#include "input.h"

// Just one controller for now
static INPUT_CONTROLLER inputController[1];

void InputInit() {
  for (unsigned char controller = 1; controller <= 1; controller++) {
    INPUT_CONTROLLER* input = &inputController[controller - 1];
    input->left.horzScale = input->left.vertScale = 1;
    input->right.horzScale = input->right.vertScale = 1;
    input->accel.horzScale = 1;
    input->accel.vertScale = -1;  // so tilt down moves forward, tilt up moves back
  }
}

void InputUpdateJoystick(INPUT_JOYSTICK* input, unsigned char joystick,
                          unsigned char vert, unsigned char horz) {
  input->vert = joystickGetAnalog(joystick, vert) * input->vertScale;
  input->horz = joystickGetAnalog(joystick, horz) * input->horzScale;
}

void InputUpdateButton(INPUT_BUTTON* input, unsigned char joystick,
                       unsigned char buttonGroup, unsigned char button) {
  bool previous = input->state;
  bool current = joystickGetDigital(joystick, buttonGroup, button);
  input->state = current;
  input->change = current - previous;
}

void InputUpdateGroup2(INPUT_GROUP_2* input, unsigned char joystick,
                       unsigned char buttonGroup) {
  InputUpdateButton(&input->up, joystick, buttonGroup, JOY_UP);
  InputUpdateButton(&input->down, joystick, buttonGroup, JOY_DOWN);
}

void InputUpdateGroup4(INPUT_GROUP_4* input, unsigned char joystick,
                       unsigned char buttonGroup) {
  InputUpdateButton(&input->up, joystick, buttonGroup, JOY_UP);
  InputUpdateButton(&input->down, joystick, buttonGroup, JOY_DOWN);
  InputUpdateButton(&input->left, joystick, buttonGroup, JOY_LEFT);
  InputUpdateButton(&input->right, joystick, buttonGroup, JOY_RIGHT);
}

void InputUpdateController(INPUT_CONTROLLER* input, unsigned char joystick) {
  InputUpdateJoystick(&input->left, joystick, 3, 4);
  InputUpdateJoystick(&input->right, joystick, 2, 1);
  InputUpdateJoystick(&input->accel, joystick, ACCEL_X, ACCEL_Y);
  InputUpdateGroup2(&input->leftButtons2, joystick, 5);
  InputUpdateGroup2(&input->rightButtons2, joystick, 6);
  InputUpdateGroup4(&input->leftButtons4, joystick, 7);
  InputUpdateGroup4(&input->rightButtons4, joystick, 8);
}

void InputUpdate() {
  for (unsigned char controller = 1; controller <= 1; controller++) {
    INPUT_CONTROLLER* input = &inputController[controller - 1];
    InputUpdateController(input, controller);
  }
}

INPUT_CONTROLLER* Input(unsigned char controller) {
  if (controller < 1 || controller > 1) {
    controller = 1;
  }
  return &inputController[controller - 1];
}
