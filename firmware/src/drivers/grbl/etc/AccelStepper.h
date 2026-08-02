
#ifndef AccelStepper_h
#define AccelStepper_h

#include <stdlib.h>
#include "hw.h"

#define YIELD


typedef enum
{
	FUNCTION  = 0, ///< Use the functional interface, implementing your own driver functions (internal use only)
	DRIVER    = 1, ///< Stepper Driver, 2 driver pins required
	FULL2WIRE = 2, ///< 2 wire stepper, 2 motor pins required
	FULL3WIRE = 3, ///< 3 wire stepper, such as HDD spindle, 3 motor pins required
	FULL4WIRE = 4, ///< 4 wire full stepper, 4 motor pins required
	HALF3WIRE = 6, ///< 3 wire half stepper, such as HDD spindle, 3 motor pins required
	HALF4WIRE = 8  ///< 4 wire half stepper, 4 motor pins required
} MotorInterfaceType;

//AccelStepper(uint8_t interface = AccelStepper::FULL4WIRE, uint8_t pin1 = 2, uint8_t pin2 = 3, uint8_t pin3 = 4, uint8_t pin4 = 5, bool enable = true);

void AccelStepper_init(uint8_t interface, uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, bool enable);

//AccelStepper(void (*forward)(), void (*backward)());

void    moveTo(long absolute);
void    move(long relative);
bool    run();
bool    runSpeed();
void    setMaxSpeed(float speed);
float   maxSpeed();
void    setAcceleration(float acceleration);
float   acceleration();
void    setSpeed(float speed);
float   speed();
long    distanceToGo();
long    targetPosition();
long    currentPosition();
void    setCurrentPosition(long position);
void    runToPosition();
bool    runSpeedToPosition();
void    runToNewPosition(long position);
void    stop();
void    disableOutputs();
void    enableOutputs();
void    setMinPulseWidth(unsigned int minWidth);
void    setEnablePin(uint8_t enablePin);
void    setPinsInverted(bool directionInvert, bool stepInvert, bool enableInvert);
void    setPinsInverted_1(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert);
bool    isRunning();

typedef enum
{
	DIRECTION_CCW = 0,  ///< Counter-Clockwise
	DIRECTION_CW  = 1   ///< Clockwise
} Direction;



typedef struct _AccelStepper{

	bool _direction;

	unsigned long  _stepInterval;

	uint8_t        _interface;          // 0, 1, 2, 4, 8, See MotorInterfaceType
	uint8_t        _pin[4];
	uint8_t        _pinInverted[4];
	long           _currentPos;    // Steps
	long           _targetPos;     // Steps
	float          _speed;         // Steps per second
	float          _maxSpeed;
	float          _acceleration;
	float          _sqrt_twoa; // Precomputed sqrt(2*_acceleration)
	unsigned long  _lastStepTime;
	unsigned int   _minPulseWidth;
	bool           _enableInverted;
	uint8_t        _enablePin;
	void (*_forward)();
	void (*_backward)();
	long _n;
	float _c0;
	float _cn;
	float _cmin; // at max speed
} AccelStepper;

extern AccelStepper Stepper;

unsigned long  computeNewSpeed();

void   setOutputPins(uint8_t mask);
void   step(long step);
long   stepForward();
long   stepBackward();
void   step0(long step);
void   step1(long step);
void   step2(long step);
void   step3(long step);
void   step4(long step);
void   step6(long step);
void   step8(long step);

#endif 
