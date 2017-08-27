#ifndef motor_h
#define motor_h

// Lifecycle
void smartMotorInit();
void smartMotorUpdate();

// Get or Set current state
short smartMotorGet(unsigned char channel);
void smartMotorSet(unsigned char channel, short speed);
void smartMotorStopAll();

// Settings
void smartMotorSlewEnabled(bool flag);
void smartMotorSlew(unsigned char channel, float up, float down);
void smartMotorReversed(unsigned char channel, bool reversed);

#endif
