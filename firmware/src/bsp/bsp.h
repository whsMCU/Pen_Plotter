/*
 * bsp.h
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_


#include "stm32f1xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif

//#define _USE_LOG_PRINT    0
//
//#if _USE_LOG_PRINT
//#define logPrintf(fmt, ...)     printf(fmt, ##__VA_ARGS__)
//#else
//#define logPrintf(fmt, ...)
//#endif

uint32_t getCycleCounter(void);

int32_t clockCyclesToMicros(int32_t clockCycles);
int32_t clockCyclesTo10thMicros(int32_t clockCycles);
uint32_t clockMicrosToCycles(uint32_t micros);

void delay(uint32_t ms);
uint32_t millis(void);
uint32_t micros(void);
void delayMicroseconds(uint32_t us);

typedef uint32_t millis_t;

#define SEC_TO_MS(N) millis_t((N)*1000UL)
#define MIN_TO_MS(N) SEC_TO_MS((N)*60UL)
#define MS_TO_SEC(N) millis_t((N)/1000UL)

#define PENDING(NOW,SOON) ((int32_t)(NOW-(SOON))<0)
#define ELAPSED(NOW,SOON) (!PENDING(NOW,SOON))

#define _BV(n) (1<<(n))

#ifdef __cplusplus
}
#endif

#endif /* SRC_BSP_BSP_H_ */
