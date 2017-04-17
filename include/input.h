#ifndef input_h
#define input_h

typedef struct {
  bool pressed;
  short changed;  // 1 immediately after press, -1 after release, 0 otherwise
} INPUT_BUTTON;

typedef struct {
  INPUT_BUTTON up;
  INPUT_BUTTON down;
} INPUT_GROUP_2;

typedef struct {
  INPUT_BUTTON up;
  INPUT_BUTTON down;
  INPUT_BUTTON left;
  INPUT_BUTTON right;
} INPUT_GROUP_4;

typedef struct {
  short vert;
  short horz;
  short vertScale;
  short horzScale;
} INPUT_JOYSTICK;

typedef struct {
  // Joystick 3, 4
  INPUT_JOYSTICK left;
  // Joystick 2, 1
  INPUT_JOYSTICK right;
  // Joystick internal gyro (tilt forward/back, left/right)
  INPUT_JOYSTICK accel;
  // Button group 5
  INPUT_GROUP_2 leftButtons2;
  // Button group 6
  INPUT_GROUP_2 rightButtons2;
  // Button group 7
  INPUT_GROUP_4 leftButtons4;
  // Button group 8
  INPUT_GROUP_4 rightButtons4;
} INPUT_CONTROLLER;

void inputInit();
void inputUpdate();

INPUT_CONTROLLER* inputController(unsigned char controller);

#endif
