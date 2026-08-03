// AccelStepper.cpp
//
// Copyright (C) 2009-2020 Mike McCauley
// $Id: AccelStepper.cpp,v 1.24 2020/04/20 00:15:03 mikem Exp mikem $

#include "AccelStepper.h"

AccelStepper stepperX;
AccelStepper stepperY;

static void TimerCallbackISR(void)
{
  // Change direction at the limits
  if (distanceToGo(&stepperX) == 0)
  	moveTo(&stepperX, -currentPosition(&stepperX));
  run(&stepperX);
}

void AccelStepper_init(AccelStepper *stepper, uint8_t enPin, uint8_t stepPin, uint8_t dirPin, bool enable)
{
	stepper->_interface = DRIVER;
	stepper->_currentPos = 0;
	stepper->_targetPos = 0;
	stepper->_speed = 0.0;
	stepper->_maxSpeed = 0.0;
	stepper->_acceleration = 0.0;
	stepper->_sqrt_twoa = 1.0;
	stepper->_stepInterval = 0;
	stepper->_minPulseWidth = 1;
	stepper->_enablePin = enPin;
	stepper->_lastStepTime = 0;
	stepper->_pin[0] = stepPin;
	stepper->_pin[1] = dirPin;
	stepper->_enableInverted = false;
    
    // NEW
	stepper->_n = 0;
	stepper->_c0 = 0.0;
	stepper->_cn = 0.0;
	stepper->_cmin = 1.0;
	stepper->_direction = DIRECTION_CCW;

	timAttachInterrupt(_DEF_TIM2, TimerCallbackISR);

	int i;
	for (i = 0; i < 2; i++)
		stepper->_pinInverted[i] = 0;
	if (enable)
		enableOutputs(stepper);
	// Some reasonable default
	setAcceleration(stepper, 1);
	setMaxSpeed(stepper, 1);
}

void moveTo(AccelStepper *stepper, long absolute)
{
  if (stepper->_targetPos != absolute)
  {
  	stepper->_targetPos = absolute;
    	computeNewSpeed(stepper);
  }
}

void move(AccelStepper *stepper, long relative)
{
    moveTo(stepper, stepper->_currentPos + relative);
}

// Implements steps according to the current step interval
// You must call this at least once per step
// returns true if a step occurred
bool runSpeed(AccelStepper *stepper)
{
    // Dont do anything unless we actually have a step interval
    if (!stepper->_stepInterval)
    	return false;

    unsigned long time = micros();   
    if (time - stepper->_lastStepTime >= stepper->_stepInterval)
    {
	if (stepper->_direction == DIRECTION_CW)
	{
	    // Clockwise
		stepper->_currentPos += 1;
	}
	else
	{
	    // Anticlockwise  
		stepper->_currentPos -= 1;
	}
	step(stepper, stepper->_currentPos);

	stepper->_lastStepTime = time; // Caution: does not account for costs in step()

	return true;
    }
    else
    {
	return false;
    }
}

long distanceToGo(AccelStepper *stepper)
{
    return stepper->_targetPos - stepper->_currentPos;
}

long targetPosition(AccelStepper *stepper)
{
    return stepper->_targetPos;
}

long currentPosition(AccelStepper *stepper)
{
    return stepper->_currentPos;
}

// Useful during initialisations or after initial positioning
// Sets speed to 0
void setCurrentPosition(AccelStepper *stepper, long position)
{
	stepper->_targetPos = stepper->_currentPos = position;
	stepper->_n = 0;
	stepper->_stepInterval = 0;
	stepper->_speed = 0.0;
}

// Subclasses can override
unsigned long computeNewSpeed(AccelStepper *stepper)
{
    long distanceTo = distanceToGo(stepper); // +ve is clockwise from curent location

    long stepsToStop = (long)((stepper->_speed * stepper->_speed) / (2.0 * stepper->_acceleration)); // Equation 16

    if (distanceTo == 0 && stepsToStop <= 1)
    {
    	// We are at the target and its time to stop
    	stepper->_stepInterval = 0;
    	stepper->_speed = 0.0;
    	stepper->_n = 0;
			return stepper->_stepInterval;
    }

    if (distanceTo > 0)
    {
	// We are anticlockwise from the target
	// Need to go clockwise from here, maybe decelerate now
	if (stepper->_n > 0)
	{
	    // Currently accelerating, need to decel now? Or maybe going the wrong way?
	    if ((stepsToStop >= distanceTo) || stepper->_direction == DIRECTION_CCW)
	    	stepper->_n = -stepsToStop; // Start deceleration
	}
	else if (stepper->_n < 0)
	{
	    // Currently decelerating, need to accel again?
	    if ((stepsToStop < distanceTo) && stepper->_direction == DIRECTION_CW)
	    	stepper->_n = -stepper->_n; // Start accceleration
	}
    }
    else if (distanceTo < 0)
    {
	// We are clockwise from the target
	// Need to go anticlockwise from here, maybe decelerate
	if (stepper->_n > 0)
	{
	    // Currently accelerating, need to decel now? Or maybe going the wrong way?
	    if ((stepsToStop >= -distanceTo) || stepper->_direction == DIRECTION_CW)
	    	stepper->_n = -stepsToStop; // Start deceleration
	}
	else if (stepper->_n < 0)
	{
	    // Currently decelerating, need to accel again?
	    if ((stepsToStop < -distanceTo) && stepper->_direction == DIRECTION_CCW)
	    	stepper->_n = -stepper->_n; // Start accceleration
	}
    }

    // Need to accelerate or decelerate
    if (stepper->_n == 0)
    {
    	// First step from stopped
    	stepper->_cn = stepper->_c0;
    	stepper->_direction = (distanceTo > 0) ? DIRECTION_CW : DIRECTION_CCW;
    }
    else
    {
    	// Subsequent step. Works for accel (n is +_ve) and decel (n is -ve).
    	stepper->_cn = stepper->_cn - ((2.0 * stepper->_cn) / ((4.0 * stepper->_n) + 1)); // Equation 13
    	stepper->_cn = max(stepper->_cn, stepper->_cmin);
    }
    stepper->_n++;
    stepper->_stepInterval = stepper->_cn;
    stepper->_speed = 1000000.0 / stepper->_cn;
    if (stepper->_direction == DIRECTION_CCW)
    	stepper->_speed = -stepper->_speed;
			return stepper->_stepInterval;
}

// Run the motor to implement speed and acceleration in order to proceed to the target position
// You must call this at least once per step, preferably in your main loop
// If the motor is in the desired position, the cost is very small
// returns true if the motor is still running to the target position.
bool run(AccelStepper *stepper)
{
    if (runSpeed(stepper))
    	computeNewSpeed(stepper);
    return stepper->_speed != 0.0 || distanceToGo(stepper) != 0;
}

void setMaxSpeed(AccelStepper *stepper, float speed)
{
    if (speed < 0.0)
       speed = -speed;
    if (stepper->_maxSpeed != speed)
    {
    	stepper->_maxSpeed = speed;
    	stepper->_cmin = 1000000.0 / speed;
	// Recompute _n from current speed and adjust speed if accelerating or cruising
	if (stepper->_n > 0)
	{
		stepper->_n = (long)((stepper->_speed * stepper->_speed) / (2.0 * stepper->_acceleration)); // Equation 16
	    computeNewSpeed(stepper);
	}
    }
}

float   maxSpeed(AccelStepper *stepper)
{
    return stepper->_maxSpeed;
}

void setAcceleration(AccelStepper *stepper, float acceleration)
{
    if (acceleration == 0.0)
	return;
    if (acceleration < 0.0)
      acceleration = -acceleration;
    if (stepper->_acceleration != acceleration)
    {
    	// Recompute _n per Equation 17
    	stepper->_n = stepper->_n * (stepper->_acceleration / acceleration);
    	// New c0 per Equation 7, with correction per Equation 15
    	stepper->_c0 = 0.676 * sqrt(2.0 / acceleration) * 1000000.0; // Equation 15
    	stepper->_acceleration = acceleration;
    	computeNewSpeed(stepper);
    }
}

float   acceleration(AccelStepper *stepper)
{
    return stepper->_acceleration;
}

void setSpeed(AccelStepper *stepper, float speed)
{
    if (speed == stepper->_speed)
        return;
    speed = constrain(speed, -stepper->_maxSpeed, stepper->_maxSpeed);
    if (speed == 0.0)
    	stepper->_stepInterval = 0;
    else
    {
    	stepper->_stepInterval = fabs(1000000.0 / speed);
    	stepper->_direction = (speed > 0.0) ? DIRECTION_CW : DIRECTION_CCW;
    }
    stepper->_speed = speed;
}

float speed(AccelStepper *stepper)
{
    return stepper->_speed;
}

// Subclasses can override
void step(AccelStepper *stepper, long step)
{
    switch (stepper->_interface)
    {
				case DRIVER:
						step1(stepper, step);
						break;
    }
}

long stepForward(AccelStepper *stepper)
{
    // Clockwise
		stepper->_currentPos += 1;
		step(stepper, stepper->_currentPos);
		stepper->_lastStepTime = micros();
    return stepper->_currentPos;
}

long stepBackward(AccelStepper *stepper)
{
    // Counter-clockwise
		stepper->_currentPos -= 1;
		step(stepper, stepper->_currentPos);
		stepper->_lastStepTime = micros();
    return stepper->_currentPos;
}

// You might want to override this to implement eg serial output
// bit 0 of the mask corresponds to _pin[0]
// bit 1 of the mask corresponds to _pin[1]
// ....
void setOutputPins(AccelStepper *stepper, uint8_t mask)
{
    uint8_t numpins = 2;
    if (stepper->_interface == FULL4WIRE || stepper->_interface == HALF4WIRE)
    	numpins = 4;
    else if (stepper->_interface == FULL3WIRE || stepper->_interface == HALF3WIRE)
    	numpins = 3;
    uint8_t i;
    for (i = 0; i < numpins; i++)
    	gpioPinWrite(stepper->_pin[i], (mask & (1 << i)) ? (HIGH ^ stepper->_pinInverted[i]) : (LOW ^ stepper->_pinInverted[i]));
}

// 1 pin step function (ie for stepper drivers)
// This is passed the current step number (0 to 7)
// Subclasses can override
void step1(AccelStepper *stepper, long step)
{
    (void)(step); // Unused

    // _pin[0] is step, _pin[1] is direction
    setOutputPins(stepper, stepper->_direction ? 0b10 : 0b00); // Set direction first else get rogue pulses
    setOutputPins(stepper, stepper->_direction ? 0b11 : 0b01); // step HIGH
    // Caution 200ns setup time 
    // Delay the minimum allowed pulse width
    delayMicroseconds(stepper->_minPulseWidth);
    setOutputPins(stepper, stepper->_direction ? 0b10 : 0b00); // step LOW
}
    
// Prevents power consumption on the outputs
void    disableOutputs(AccelStepper *stepper)
{   
    if (! stepper->_interface) return;

    setOutputPins(stepper, 0); // Handles inversion automatically
    if (stepper->_enablePin != 0xff)
    {
    	gpioPinWrite(stepper->_enablePin, LOW ^ stepper->_enableInverted);
    }
}

void    enableOutputs(AccelStepper *stepper)
{
    if (! stepper->_interface)
    	return;

    if (stepper->_enablePin != 0xff)
    {
    	gpioPinWrite(stepper->_enablePin, HIGH ^ stepper->_enableInverted);
    }
}

void setMinPulseWidth(AccelStepper *stepper, unsigned int minWidth)
{
	stepper->_minPulseWidth = minWidth;
}

void setEnablePin(AccelStepper *stepper, uint8_t enablePin)
{
	stepper->_enablePin = enablePin;

    // This happens after construction, so init pin now.
    if (stepper->_enablePin != 0xff)
    {
    	gpioPinWrite(stepper->_enablePin, HIGH ^ stepper->_enableInverted);
    }
}

void setPinsInverted(AccelStepper *stepper, bool directionInvert, bool stepInvert, bool enableInvert)
{
	stepper->_pinInverted[0] = stepInvert;
	stepper->_pinInverted[1] = directionInvert;
	stepper->_enableInverted = enableInvert;
}


// Blocks until the target position is reached and stopped
void runToPosition(AccelStepper *stepper)
{
    while (run(stepper))
	YIELD; // Let system housekeeping occur
}

bool runSpeedToPosition(AccelStepper *stepper)
{
    if (stepper->_targetPos == stepper->_currentPos)
    	return false;
    if (stepper->_targetPos >stepper->_currentPos)
    	stepper->_direction = DIRECTION_CW;
    else
    	stepper->_direction = DIRECTION_CCW;
    return runSpeed(stepper);
}

// Blocks until the new target position is reached
void runToNewPosition(AccelStepper *stepper, long position)
{
    moveTo(stepper, position);
    runToPosition(stepper);
}

void stop(AccelStepper *stepper)
{
    if (stepper->_speed != 0.0)
    {    
    	long stepsToStop = (long)((stepper->_speed * stepper->_speed) / (2.0 * stepper->_acceleration)) + 1; // Equation 16 (+integer rounding)
			if (stepper->_speed > 0)
					move(stepper, stepsToStop);
			else
					move(stepper, -stepsToStop);
    }
}

bool isRunning(AccelStepper *stepper)
{
    return !(stepper->_speed == 0.0 && stepper->_targetPos == stepper->_currentPos);
}
