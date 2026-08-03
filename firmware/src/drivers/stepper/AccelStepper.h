
#ifndef AccelStepper_h
#define AccelStepper_h

#include "hw.h"

#define YIELD

#define HIGH 1
#define LOW 0

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

typedef enum
{
	DIRECTION_CCW = 0,  ///< Counter-Clockwise
	DIRECTION_CW  = 1   ///< Clockwise
} Direction;


typedef struct _AccelStepper{

	bool _direction;

	unsigned long  _stepInterval;

	uint8_t        _interface;          // 0, 1, 2, 4, 8, See MotorInterfaceType
	uint8_t        _pin[2];
	uint8_t        _pinInverted[2];
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

void AccelStepper_init(AccelStepper *stepper, uint8_t enPin, uint8_t stepPin, uint8_t dirPin, bool enable);

extern AccelStepper stepperX;
extern AccelStepper stepperY;

void    moveTo(AccelStepper *stepper, long absolute);
void    move(AccelStepper *stepper, long relative);
bool    run(AccelStepper *stepper);
bool    runSpeed(AccelStepper *stepper);
void    setMaxSpeed(AccelStepper *stepper, float speed);
float   maxSpeed(AccelStepper *stepper);
void    setAcceleration(AccelStepper *stepper, float acceleration);
float   acceleration(AccelStepper *stepper);
void    setSpeed(AccelStepper *stepper, float speed);
float   speed(AccelStepper *stepper);
long    distanceToGo(AccelStepper *stepper);
long    targetPosition(AccelStepper *stepper);
long    currentPosition(AccelStepper *stepper);
void    setCurrentPosition(AccelStepper *stepper, long position);
void    runToPosition(AccelStepper *stepper);
bool    runSpeedToPosition(AccelStepper *stepper);
void    runToNewPosition(AccelStepper *stepper, long position);
void    stop(AccelStepper *stepper);
void    disableOutputs(AccelStepper *stepper);
void    enableOutputs(AccelStepper *stepper);
void    setMinPulseWidth(AccelStepper *stepper, unsigned int minWidth);
void    setEnablePin(AccelStepper *stepper, uint8_t enablePin);
void    setPinsInverted(AccelStepper *stepper, bool directionInvert, bool stepInvert, bool enableInvert);
bool    isRunning(AccelStepper *stepper);

unsigned long  computeNewSpeed(AccelStepper *stepper);

//void   setOutputPins(uint8_t mask);
void   step(AccelStepper *stepper, long step);
long   stepForward(AccelStepper *stepper);
long   stepBackward(AccelStepper *stepper);

void   step1(AccelStepper *stepper, long step);


#endif 
