/*
 * scheduler.h
 *
 *  Created on: 2026. 7. 31.
 *      Author: WANG
 */

#ifndef SRC_SCHEDULER_SCHEDULER_H_
#define SRC_SCHEDULER_SCHEDULER_H_

#include "hw.h"

typedef void (*TaskFunc)(void);

typedef struct
{
    TaskFunc func;

    uint32_t period;      // 실행 주기(ms)
    uint32_t lastTick;

    uint8_t priority;     // 작을수록 우선순위 높음

    volatile uint8_t ready;

}Task_t;

void Scheduler_Run(void);

#endif /* SRC_SCHEDULER_SCHEDULER_H_ */
