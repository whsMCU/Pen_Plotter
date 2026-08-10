/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>


//#include "common/utils.h"

#include "scheduler/scheduler.h"
#include "scheduler/tasks.h"

#include "scheduler/tasks.h"

#include"grbl.h"

int32_t sys_position[N_AXIS];      // Real-time machine (aka home) position vector in steps.
int32_t sys_probe_position[N_AXIS]; // Last probe position in machine coordinates and steps.
volatile uint8_t sys_probe_state;   // Probing state value.  Used to coordinate the probing cycle with stepper ISR.
volatile uint8_t sys_rt_exec_state;   // Global realtime executor bitflag variable for state management. See EXEC bitmasks.
volatile uint8_t sys_rt_exec_alarm;   // Global realtime executor bitflag variable for setting various alarms.
volatile uint8_t sys_rt_exec_motion_override; // Global realtime executor bitflag variable for motion-based overrides.
volatile uint8_t sys_rt_exec_accessory_override; // Global realtime executor bitflag variable for spindle/coolant overrides.
#ifdef DEBUG
  volatile uint8_t sys_rt_exec_debug;
#endif


static void ledUpdate(uint32_t currentTimeUs)
{
    static uint32_t pre_time = 0;
    if(currentTimeUs - pre_time >= 1000000)
    {
        pre_time = currentTimeUs;
        gpioPinToggle(LED);
    }
}

uint32_t debug1;

static void debugPrint(uint32_t currentTimeUs)
{
  //uartPrintf_IT(_DEF_UART1, "adcValue =  %d, buff = %d\r\n", adcGetValue(0), adcInternalRead(4));
}

static void taskHandleSerial(uint32_t currentTimeUs)
{
    UNUSED(currentTimeUs);

    cliMain();

}

static void gcodeUpdate(uint32_t currentTimeUs)
{

}

static void motionUpdate(uint32_t currentTimeUs)
{

}

int32_t newPos = 0;
int32_t pos = 0;
int32_t dir = 0;

static void lcdUpdate(uint32_t currentTimeUs)
{
  newPos = getPosition();
  if (pos != newPos) {

  	dir = getDirection();
    pos = newPos;
  }

  if(!ssd1306_Update_satus())
  {
    SSD1306_Clear();
    char buffer[32];

    sprintf(buffer, "P:%ld, D:%ld, f: %ld ms", newPos, dir, SSD1306_get_fps());

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(buffer, Font_7x10, White);
  }

  ssd1306_UpdateScreen();
}

#define DEFINE_TASK(taskNameParam, taskFuncParam, desiredPeriodParam, staticPriorityParam) {  \
    .taskName = taskNameParam, \
    .taskFunc = taskFuncParam, \
    .desiredPeriodUs = desiredPeriodParam, \
	  .staticPriority = staticPriorityParam \
}

// Task info in .bss (unitialised data)
task_t tasks[TASK_COUNT];

// Task ID data in .data (initialised data)
task_attribute_t task_attributes[TASK_COUNT] = {
    [TASK_SYSTEM] = DEFINE_TASK("SYSTEM", taskSystemLoad, TASK_PERIOD_HZ(10), TASK_PRIORITY_MEDIUM_HIGH),
    [TASK_LED] = DEFINE_TASK("LED", ledUpdate, TASK_PERIOD_HZ(100), TASK_PRIORITY_LOW),
    [TASK_SERIAL] = DEFINE_TASK("SERIAL", taskHandleSerial, TASK_PERIOD_HZ(100), TASK_PRIORITY_LOW), // 100 Hz should be enough to flush up to 115 bytes @ 115200 baud
    [TASK_GCODE] = DEFINE_TASK("GCODE", gcodeUpdate, TASK_PERIOD_HZ(100), TASK_PRIORITY_LOW),
    [TASK_MOTION] = DEFINE_TASK("MOTION", motionUpdate, TASK_PERIOD_HZ(100), TASK_PRIORITY_LOW),
    [TASK_LCD] = DEFINE_TASK("LCD", lcdUpdate, TASK_PERIOD_HZ(100), TASK_PRIORITY_LOW),
    [TASK_DEBUG] = DEFINE_TASK("DEBUG", debugPrint, TASK_PERIOD_HZ(50), TASK_PRIORITY_LOW),
};

task_t *getTask(unsigned taskId)
{
    return &tasks[taskId];
}

// Has to be done before tasksInit() in order to initialize any task data which may be uninitialized at boot
void tasksInitData(void)
{
    for (int i = 0; i < TASK_COUNT; i++) {
        tasks[i].attribute = &task_attributes[i];
    }
}

void tasksInit(void)
{
    schedulerInit();


    setTaskEnabled(TASK_LED, true);
    setTaskEnabled(TASK_SERIAL, true);
    setTaskEnabled(TASK_GCODE, true);
    setTaskEnabled(TASK_MOTION, true);
    setTaskEnabled(TASK_LCD, true);
    setTaskEnabled(TASK_DEBUG, true);

}

