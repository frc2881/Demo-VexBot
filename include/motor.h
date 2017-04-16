#ifndef motor_h
#define motor_h

void smartMotorInit();
void smartMotorUpdate();

short smartMotorGet(unsigned char channel);
void smartMotorSet(unsigned char channel, short speed);
void smartMotorSlew(unsigned char channel, short up, short down);
void smartMotorReversed(unsigned char channel, bool reversed);

#endif
