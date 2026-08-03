// MultiStepper.h

#ifndef MultiStepper_h
#define MultiStepper_h

#include "hw.h"

#define MULTISTEPPER_MAX_STEPPERS 10

bool addStepper(MultiStepper *multi_stepper, AccelStepper& stepper);

typedef struct _MultiStepper{

AccelStepper* _steppers[MULTISTEPPER_MAX_STEPPERS];
uint8_t       _num_steppers;

} MultiStepper;

void multi_moveTo(long absolute[]);

bool multi_run();

void multi_runSpeedToPosition();



#endif
