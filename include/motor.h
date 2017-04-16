#ifndef motor_h
#define motor_h

void SmartMotorInit();
void SmartMotorUpdate();

void SmartMotorSet(unsigned char channel, short speed);
void SmartMotorSlew(unsigned char channel, short up, short down);
void SmartMotorReversed(unsigned char channel, bool reversed);

#endif
