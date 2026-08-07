// AccelStepper.cpp
//
// Copyright (C) 2009-2020 Mike McCauley
// $Id: AccelStepper.cpp,v 1.24 2020/04/20 00:15:03 mikem Exp mikem $

#include "AccelStepper.h"

AccelStepper stepperX;
AccelStepper stepperY;

uint32_t callback_time, callback_pretime = 0;

static void TimerCallbackISR(void)
{
	callback_time = micros() - callback_pretime;
  callback_pretime = micros();
  // Change direction at the limits
  if (distanceToGo(&stepperX) == 0)
  {
  	moveTo(&stepperX, -currentPosition(&stepperX));
  }
  run(&stepperX);

}

void AccelStepper_init(AccelStepper *stepper, uint8_t enablePin, uint8_t stepPin, uint8_t dirPin, bool enable)
{
	stepper->_currentPos = 0;
	stepper->_targetPos = 0;
	stepper->_speed = 0.0;
	stepper->_maxSpeed = 0.0;
	stepper->_acceleration = 0.0;
	stepper->_sqrt_twoa = 1.0;
	stepper->_stepInterval = 0;
	stepper->_minPulseWidth = 1;
	stepper->_enablePin = enablePin;
	stepper->_lastStepTime = 0;
	stepper->_stepPin = stepPin;
	stepper->_dirPin = dirPin;
	stepper->_enableInverted = false;
    
    // NEW
	stepper->_n = 0;
	stepper->_c0 = 0.0;
	stepper->_cn = 0.0;
	stepper->_cmin = 1.0;
	stepper->_direction = DIRECTION_CCW;

	timAttachInterrupt(_DEF_TIM2, TimerCallbackISR);

	gpioPinWrite(stepper->_enablePin,  _DEF_LOW);
	// Some reasonable default
	setAcceleration(stepper, 1);
	setMaxSpeed(stepper, 1);
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
  (void)(step); // Unused

  if(stepper->_direction == 1)
  {
    gpioPinWrite(stepper->_dirPin,  _DEF_HIGH);
    gpioPinWrite(stepper->_stepPin,  _DEF_HIGH);
  }
  else
  {
    gpioPinWrite(stepper->_dirPin,  _DEF_LOW);
    gpioPinWrite(stepper->_stepPin,  _DEF_HIGH);
  }
  // Caution 200ns setup time
  // Delay the minimum allowed pulse width
  delayMicroseconds(stepper->_minPulseWidth);
  gpioPinWrite(stepper->_stepPin,  _DEF_LOW);
}
    
// Prevents power consumption on the outputs
void    disableOutputs(AccelStepper *stepper)
{   
  gpioPinWrite(stepper->_enablePin, _DEF_HIGH);
}

void    enableOutputs(AccelStepper *stepper)
{
  gpioPinWrite(stepper->_enablePin, _DEF_LOW);
}

void setMinPulseWidth(AccelStepper *stepper, unsigned int minWidth)
{
	stepper->_minPulseWidth = minWidth;
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
