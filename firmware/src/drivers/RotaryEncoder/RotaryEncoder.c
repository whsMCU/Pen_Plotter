/*
 * RotaryEncoder.c
 *
 *  Created on: 2026. 7. 30.
 *      Author: WANG
 */

#include "RotaryEncoder.h"
#include "nuts_bolts.h"

// The array holds the values �1 for the entries where a position was decremented,
// a 1 for the entries where the position was incremented
// and 0 in all the other (no change or not valid) cases.

const int8_t KNOBDIR[] = {
  0, -1, 1, 0,
  1, 0, 0, -1,
  -1, 0, 0, 1,
  0, 1, -1, 0
};

// positions: [3] 1 0 2 [3] 1 0 2 [3]
// [3] is the positions where my rotary switch detends
// ==> right, count up
// <== left,  count down

RotaryEncoder encoder;


void RotaryEncoder_init(LatchMode mode)
{

	encoder._mode = mode;

  // No Hardware specific setup here.
  // use the ...
	encoder._pin1 = RotaryEncoder_1;
	encoder._pin2 = RotaryEncoder_2;

  // start with position 0;
	encoder._position = 0;
	encoder._oldState = 0;
	encoder._positionExtPrev = 0;
	encoder._positionExt = 0;
	encoder._positionExtTimePrev = millis();
	encoder._positionExtTime = millis();
}

long getPosition() {
  return encoder._positionExt;
}  // getPosition()

Direction_t getDirection() {
  Direction_t ret = NOROTATION;

  if (encoder._positionExtPrev > encoder._positionExt) {
    ret = COUNTERCLOCKWISE;
    encoder._positionExtPrev = encoder._positionExt;
  } else if (encoder._positionExtPrev < encoder._positionExt) {
    ret = CLOCKWISE;
    encoder._positionExtPrev = encoder._positionExt;
  } else {
    ret = NOROTATION;
    encoder._positionExtPrev = encoder._positionExt;
  }

  return ret;
}

void setPosition(long newPosition) {
  switch (encoder._mode) {
    case FOUR3:
    case FOUR0:
      // only adjust the external part of the position.
    	encoder._position = ((newPosition << 2) | (encoder._position & 0x03L));
    	encoder._positionExt = newPosition;
    	encoder._positionExtPrev = newPosition;
      break;

    case TWO03:
      // only adjust the external part of the position.
    	encoder._position = ((newPosition << 1) | (encoder._position & 0x01L));
    	encoder._positionExt = newPosition;
    	encoder._positionExtPrev = newPosition;
      break;
  }  // switch

}  // setPosition()

// Slow, but Simple Variant by directly Read-Out of the Digital State within loop-call
void tick(void) {

  static uint32_t lastTick = 0;

  uint32_t now = millis();

  // 2ms 이내 변화 무시
  if((now - lastTick) < 3)
      return;

  lastTick = now;

  bool sig1 = gpioPinRead(RotaryEncoder_1);
  bool sig2 = gpioPinRead(RotaryEncoder_2);
  tick_cal(sig1, sig2);
}  // tick()


// When a faster method than digitalRead is available you can _tick with the 2 values directly.
void tick_cal(bool sig1, bool sig2) {
  unsigned long now = millis();
  int8_t thisState = sig1 | (sig2 << 1);

  if (encoder._oldState != thisState) {
  	encoder._position += KNOBDIR[thisState | (encoder._oldState << 2)];
  	encoder._oldState = thisState;

    switch (encoder._mode) {
      case FOUR3:
        if (thisState == LATCH3) {
          // The hardware has 4 steps with a latch on the input state 3
        	encoder._positionExt = encoder._position >> 2;
          encoder._positionExtTimePrev = encoder._positionExtTime;
          encoder._positionExtTime = now;
        }
        break;

      case FOUR0:
        if (thisState == LATCH0) {
          // The hardware has 4 steps with a latch on the input state 0
        	encoder._positionExt = encoder._position >> 2;
        	encoder._positionExtTimePrev = encoder._positionExtTime;
        	encoder._positionExtTime = now;
        }
        break;

      case TWO03:
        if ((thisState == LATCH0) || (thisState == LATCH3)) {
          // The hardware has 2 steps with a latch on the input state 0 and 3
        	encoder._positionExt = encoder._position >> 1;
        	encoder._positionExtTimePrev = encoder._positionExtTime;
        	encoder._positionExtTime = now;
        }
        break;
    }  // switch
  }  // if
}  // tick()

unsigned long getMillisBetweenRotations() {
  return (encoder._positionExtTime - encoder._positionExtTimePrev);
}

unsigned long getRPM() {
  // calculate max of difference in time between last position changes or last change and now.
  unsigned long timeBetweenLastPositions = encoder._positionExtTime - encoder._positionExtTimePrev;
  unsigned long timeToLastPosition = millis() - encoder._positionExtTime;
  unsigned long t = max(timeBetweenLastPositions, timeToLastPosition);
  return 60000.0 / ((float)(t * 20));
}






