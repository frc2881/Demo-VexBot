#ifndef motor_h
#define motor_h

void smartMotorInit();
void smartMotorUpdate();
void smartMotorSlewEnabled(bool flag);

short smartMotorGet(unsigned char channel);
void smartMotorSet(unsigned char channel, short speed);
void smartMotorSlew(unsigned char channel, float up, float down);
void smartMotorReversed(unsigned char channel, bool reversed);

#endif
