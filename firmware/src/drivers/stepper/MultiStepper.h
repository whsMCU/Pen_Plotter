// MultiStepper.h

#ifndef MultiStepper_h
#define MultiStepper_h

#include "hw.h"

#define MULTISTEPPER_MAX_STEPPERS 10

typedef struct _AccelStepper AccelStepper;

typedef struct _MultiStepper{

	AccelStepper* _steppers[MULTISTEPPER_MAX_STEPPERS];
	uint8_t       _num_steppers;

} MultiStepper;

bool addStepper(MultiStepper *multi_stepper, AccelStepper *stepper);

void multi_moveTo(MultiStepper *multi_stepper, long absolute[]);

bool multi_run(MultiStepper *multi_stepper);

void multi_runSpeedToPosition(MultiStepper *multi_stepper);



#endif
