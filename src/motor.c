#include <API.h>
#include <math.h>
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

static short linearize(short level, const float* mapping);

static bool _smartMotorEnabled;
static SmartMotor _motorState[10];
static unsigned long _motorLastUpdate;  // time in millis

#define DEADBAND_393 12
static float _linearize393[] = {0, 13.7, 17.1, 20.4, 23.7, 27.9, 32.7, 39.8, 50.3, 69.3, 127};

void smartMotorInit() {
    motorStopAll();
    smartMotorEnabled(true);
    for (unsigned char channel = 1; channel <= 10; channel++) {
        SmartMotor *m = &_motorState[channel - 1];
        m->desired = 0;
        m->actual = 0;
        m->slewUp = 0.75;   // 0.75*20 millis = 15 points per typical interval, roughly 100 millis from 0 to full power
        m->slewDown = 2;    // power ramps down faster than it ramps up
        m->scale = 1;
    }
}

void smartMotorUpdate() {
    unsigned long now = millis();
    unsigned long delta = now - _motorLastUpdate;
    _motorLastUpdate = now;

    for (unsigned char channel = 1; channel <= 10; channel++) {
        SmartMotor *m = &_motorState[channel - 1];
        short actual = m->actual;
        short desired = m->desired;
        if (actual == desired) {
            continue;
        }
        if (!_smartMotorEnabled) {
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
        m->actual = speed * direction;  // note: 'actual' is used for computing slew so it's pre-linearization

        // Apply linearization map and deadband for the Vex 393 motor
        short power = linearize(m->actual, _linearize393);
        power = abs(power) < DEADBAND_393 ? 0 : power;

        motorSet(channel, power);
    }
}

void smartMotorEnabled(bool flag) {
    _smartMotorEnabled = flag;
}

void smartMotorSlew(unsigned char channel, float up, float down) {
    if (channel < 1 || channel > 10) return;
    SmartMotor *m = &_motorState[channel - 1];
    m->slewUp = up;
    m->slewDown = down;
}

void smartMotorReversed(unsigned char channel, bool reversed) {
    if (channel < 1 || channel > 10) return;
    SmartMotor *m = &_motorState[channel - 1];
    m->scale = reversed ? -1 : 1;
}

short smartMotorGet(unsigned char channel) {
    if (channel < 1 || channel > 10) return 0;
    SmartMotor *m = &_motorState[channel - 1];
    return m->desired * m->scale;
}

void smartMotorSet(unsigned char channel, short speed) {
    if (channel < 1 || channel > 10) return;
    SmartMotor *m = &_motorState[channel - 1];
    if (speed > 127) {
        speed = 127;
    } else if (speed < -127) {
        speed = -127;
    }
    m->desired = speed * m->scale;
}

void smartMotorStopAll() {
    for (unsigned char channel = 1; channel <= 10; channel++) {
        SmartMotor *m = &_motorState[channel - 1];
        m->desired = 0;
    }
}

/*
 * Almost all kinds of feedback algorithms (eg. PID loops etc) expect that motor speed is linearly related to
 * to the motor input.  That is, if you increase the input by 10%, the motor speed should increase by 10% as a
 * result.  But the Vex motors do *not* have a linear response.  For example, with the 393 motor, up to an input
 * level of 50 the motor speed increases quite a bit faster than linearly, and after 70 motor response is quite flat.
 * This method adjusts input levels to achieve a linear response.
 */
short linearize(short level, const float* mapping) {
    if (level <= -127) {
        return -127;
    }
    if (level >= 127) {
        return 127;
    }

    // Map the level to an integer 0 <= n < 10 plus a fractional component 0 <= f < 1
    short direction = level < 0 ? -1 : 1;
    float integer;
    float f = modff(abs(level) * 10 / 127.0, &integer);
    short n = lroundf(integer);

    // Pick the two closest entries in the mapping table corresponding to the input level
    float left = mapping[n];
    float right = mapping[n + 1];

    // Compute the weighted average of the two entries
    short result = lroundf(left * (1 - f) + right * f);

    // Fix the sign +/- of the result
    return result * direction;
}
