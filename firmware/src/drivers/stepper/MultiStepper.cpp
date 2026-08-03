// MultiStepper.cpp
//
// Copyright (C) 2015 Mike McCauley
// $Id: MultiStepper.cpp,v 1.3 2020/04/20 00:15:03 mikem Exp mikem $

#include "MultiStepper.h"
#include "AccelStepper.h"


bool addStepper(MultiStepper *multi_stepper, AccelStepper& stepper)
{
    if (multi_stepper->_num_steppers >= MULTISTEPPER_MAX_STEPPERS)
	return false; // No room for more
    multi_stepper->_steppers[multi_stepper->_num_steppers++] = &stepper;
    return true;
}

void multi_moveTo(MultiStepper *multi_stepper, long absolute[])
{
    // First find the stepper that will take the longest time to move
    float longestTime = 0.0;

    uint8_t i;
    for (i = 0; i < _num_steppers; i++)
    {
	long thisDistance = absolute[i] - _steppers[i]->currentPosition();
	float thisTime = abs(thisDistance) / _steppers[i]->maxSpeed();

	if (thisTime > longestTime)
	    longestTime = thisTime;
    }

    if (longestTime > 0.0)
    {
	// Now work out a new max speed for each stepper so they will all 
	// arrived at the same time of longestTime
	for (i = 0; i < _num_steppers; i++)
	{
	    long thisDistance = absolute[i] - _steppers[i]->currentPosition();
	    float thisSpeed = thisDistance / longestTime;
	    _steppers[i]->moveTo(absolute[i]); // New target position (resets speed)
	    _steppers[i]->setSpeed(thisSpeed); // New speed
	}
    }
}

// Returns true if any motor is still running to the target position.
bool multi_run(MultiStepper *multi_stepper)
{
    uint8_t i;
    bool ret = false;
    for (i = 0; i < multi_stepper->_num_steppers; i++)
    {
		if ( multi_stepper->_steppers[i]->distanceToGo() != 0)
		{
			multi_stepper->_steppers[i]->runSpeed();
			ret = true;
		}
    }
    return ret;
}

// Blocks until all steppers reach their target position and are stopped
void    multi_runSpeedToPosition(MultiStepper *multi_stepper)
{ 
    while (run(multi_stepper->_steppers[i]));
}

