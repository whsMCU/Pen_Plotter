/*
 * RotaryEncoder.h
 *
 *  Created on: 2026. 7. 30.
 *      Author: WANG
 */

#ifndef SRC_DRIVERS_ROTARYENCODER_ROTARYENCODER_H_
#define SRC_DRIVERS_ROTARYENCODER_ROTARYENCODER_H_

#include "hw.h"

#ifndef NO_PIN
#define NO_PIN -1
#endif

#define LATCH0 0  // input state at position 0
#define LATCH3 3  // input state at position 3

typedef enum {
	NOROTATION = 0,
	CLOCKWISE = 1,
	COUNTERCLOCKWISE = -1
}Direction_t;

typedef enum {
	FOUR3 = 1,  // 4 steps, Latch at position 3 only (compatible to older versions)
	FOUR0 = 2,  // 4 steps, Latch at position 0 (reverse wirings)
	TWO03 = 3   // 2 steps, Latch at position 0 and 3
}LatchMode;


extern const int8_t KNOBDIR[16];

typedef struct _RotaryEncoder{
  int _pin1, _pin2;  // Arduino pins used for the encoder.

  LatchMode _mode;  // Latch mode from initialization

  volatile int8_t _oldState;

  volatile long _position;         // Internal position (4 times _positionExt)
  volatile long _positionExt;      // External position
  volatile long _positionExtPrev;  // External position (used only for direction checking)

  unsigned long _positionExtTime;      // The time the last position change was detected.
  unsigned long _positionExtTimePrev;  // The time the previous position change was detected.
} RotaryEncoder;

void RotaryEncoder_init(LatchMode mode);

// retrieve the current position
long getPosition();

// simple retrieve of the direction the knob was rotated last time. 0 = No rotation, 1 = Clockwise, -1 = Counter Clockwise
Direction_t getDirection();

// adjust the current position
void setPosition(long newPosition);

// call this function every some milliseconds or by using an interrupt for handling state changes of the rotary encoder.
// This method uses the standard Arduino digitalRead() function with the 2 pins provided in the class creation.
void tick(void);

// Use this tick variant when a faster method than digitalRead is available and provide the values directly.
// The 2 pins provided in the class creation are ignored.
void tick_cal(bool sig1, bool sig2);

// Returns the time in milliseconds between the current observed
unsigned long getMillisBetweenRotations();

// Returns the RPM
unsigned long getRPM();

#endif /* SRC_DRIVERS_ROTARYENCODER_ROTARYENCODER_H_ */
