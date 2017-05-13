#include <API.h>
#include "motor.h"

typedef struct {
    short desired;
    short actual;
    /* Rate of increase towards max power in PWM steps per millisecond.  Default is 0.75. */
    float slewUp;
    /* Rate of decrease towards off in PWM steps per millisecond.  Default is 100. */
    float slewDown;
    short scale;
} SmartMotor;

static bool _smartMotorSlewEnabled;
static SmartMotor _smartMotorState[10];
static unsigned long _smartMotorLastUpdate;  // time in millis

void smartMotorInit() {
    motorStopAll();
    _smartMotorSlewEnabled = true;
    for (unsigned char channel = 1; channel <= 10; channel++) {
        SmartMotor *m = &_smartMotorState[channel - 1];
        m->desired = 0;
        m->actual = 0;
        m->slewUp = 0.75;
        m->slewDown = 100;
        m->scale = 1;
    }
}

void smartMotorUpdate() {
    unsigned long now = millis();
    unsigned long delta = now - _smartMotorLastUpdate;
    _smartMotorLastUpdate = now;

    for (unsigned char channel = 1; channel <= 10; channel++) {
        SmartMotor *m = &_smartMotorState[channel - 1];
        short actual = m->actual;
        short desired = m->desired;
        if (actual == desired) {
            continue;
        }
        if (!_smartMotorSlewEnabled) {
            m->actual = desired;
            motorSet(channel, desired);
            return;
        }
        // If reversing direction, decelerate to 0 before accelerating in the opposite direction
        if ((actual < 0 && desired > 0) || (actual > 0 && desired < 0)) {
            desired = 0;
        }
        // At this point, 'actual' and 'desired' have the same sign (or are 0)
        short direction = (desired < 0 || actual < 0) ? -1 : 1;
        actual = abs(actual);
        desired = abs(desired);
        // And now 'actual' and 'desired' are both non-negative
        short speed;
        if (desired > actual) {
            // Go faster...
            speed = actual + (int) (m->slewUp * delta);
            if (speed > desired) {
                speed = desired;
            }
        } else {
            // Slow down...
            speed = actual - (int) (m->slewDown * delta);
            if (speed < desired) {
                speed = desired;
            }
        }
        // Deadband from zero to 15
        m->actual = speed * direction;
        motorSet(channel, speed >= 15 ? m->actual : 0);
    }
}

void smartMotorSlewEnabled(bool flag) {
    _smartMotorSlewEnabled = flag;
}

void smartMotorSlew(unsigned char channel, float up, float down) {
    if (channel < 1 || channel > 10) return;
    SmartMotor *m = &_smartMotorState[channel - 1];
    m->slewUp = up;
    m->slewDown = down;
}

void smartMotorReversed(unsigned char channel, bool reversed) {
    if (channel < 1 || channel > 10) return;
    SmartMotor *m = &_smartMotorState[channel - 1];
    m->scale = reversed ? -1 : 1;
}

short smartMotorGet(unsigned char channel) {
    if (channel < 1 || channel > 10) return 0;
    SmartMotor *m = &_smartMotorState[channel - 1];
    return m->desired * m->scale;
}

void smartMotorSet(unsigned char channel, short speed) {
    if (channel < 1 || channel > 10) return;
    SmartMotor *m = &_smartMotorState[channel - 1];
    m->desired = speed * m->scale;
}
