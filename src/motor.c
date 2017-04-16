#include <API.h>
#include "motor.h"

typedef struct {
  short desired;
  /* Rate of increase towards max power in PWM steps per 20 milliseconds.  Default is 15. */
  short slewUp;
  /* Rate of decrease towards off in PWM steps per 20 milliseconds.  Default is 255. */
  short slewDown;
  short scale;
} SMART_MOTOR;

static SMART_MOTOR smartMotorState[10];

void smartMotorInit() {
  motorStopAll();
  for (unsigned char channel = 1; channel <= 10; channel++) {
    SMART_MOTOR* m = &smartMotorState[channel - 1];
    m->desired = 0;
    m->slewUp = 15;
    m->slewDown = 255;
    m->scale = 1;
  }
}

void smartMotorUpdate() {
  for (unsigned char channel = 1; channel <= 10; channel++) {
    SMART_MOTOR* m = &smartMotorState[channel - 1];
    short actual = motorGet(channel);
    short desired = m->desired;
    if (actual == desired) {
      continue;
    }
    // If reversing direction, decelerate to 0 before accelerating in the opposite direction
    if ((actual < 0 && desired > 0) || (actual > 0 && desired < 0)) {
      desired = 0;
    }
    // At this point, 'actual' and 'desired' have the same sign (or are 0)
    short direction = (desired >= 0) ? 1 : -1;
    actual = abs(actual);
    desired = abs(desired);
    // And now 'actual' and 'desired' are both non-negative
    short speed;
    if (actual < desired) {
      // Go faster...
      speed = actual + m->slewUp;
      if (speed > desired) {
        speed = desired;
      }
    } else {
      // Slow down...
      speed = actual - m->slewDown;
      if (speed < desired) {
        speed = desired;
      }
    }
    motorSet(channel, speed * direction);
  }
}

void smartMotorSlew(unsigned char channel, short up, short down) {
  if (channel <= 0 || channel > 10) return;
  SMART_MOTOR* m = &smartMotorState[channel - 1];
  m->slewUp = up;
  m->slewDown = down;
}

void smartMotorReversed(unsigned char channel, bool reversed) {
  if (channel <= 0 || channel > 10) return;
  SMART_MOTOR* m = &smartMotorState[channel - 1];
  m->scale = reversed ? -1 : 1;
}

void smartMotorSet(unsigned char channel, short speed) {
  if (channel <= 0 || channel > 10) return;
  SMART_MOTOR* m = &smartMotorState[channel - 1];
  // deadband up to 10
  m->desired = (abs(speed) >= 10 ? speed : 0) * m->scale;
}
