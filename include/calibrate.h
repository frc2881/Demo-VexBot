#ifndef calibrate_h
#define calibrate_h

#include <API.h>

typedef enum {
    CALIBRATE_NONE,
    CALIBRATE_MOTOR_RPM,
} CalibrationMode;

typedef struct {
    CalibrationMode mode;
    unsigned long nextAt;
    unsigned char channel;
    short input;
    double lastRpm;
} Calibration;

extern Calibration calibration;

void calibrationStart(CalibrationMode mode);
void calibrationUpdate(unsigned long now);
void calibrationEnd();

#endif //calibrate_h
