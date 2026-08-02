/*
 * scheduler.c
 *
 *  Created on: 2026. 7. 31.
 *      Author: WANG
 */


#include "scheduler.h"

#define TASK_COUNT (sizeof(taskTable)/sizeof(Task_t))

//stepper motors

void Task_UART(void);
void Task_Gcode(void);
void Task_Motion(void);
void Task_Button(void);
void Task_LCD(void);
void Task_LED(void);

Task_t taskTable[] =
{
    {Task_UART,    1,   0, 0},
    {Task_Gcode,   1,   0, 1},
    {Task_Motion,  1,   0, 2},
    {Task_Button, 10,   0, 3},
    {Task_LCD,   100,   0, 4},
    {Task_LED,   500,   0, 5},
};

void Scheduler_Run(void)
{
    while (1)
    {
        uint32_t now = millis();

        int selected = -1;

        for (int i = 0; i < TASK_COUNT; i++)
        {
            if ((uint32_t)(now - taskTable[i].lastTick) >= taskTable[i].period)
            {
                if ((selected == -1) ||
                    (taskTable[i].priority < taskTable[selected].priority))
                {
                    selected = i;
                }
            }
        }

        if (selected == -1)
            break;

        taskTable[selected].lastTick += taskTable[selected].period;

        taskTable[selected].func();
    }
}

int32_t newPos = 0;
int32_t pos = 0;
int32_t dir = 0;

void Task_LCD(void)
{
  newPos = getPosition();
  if (pos != newPos) {

  	dir = getDirection();
    pos = newPos;
  }

  SSD1306_Clear();
  char buffer[32];

  sprintf(buffer, "P:%ld, D:%ld, f: %ld ms", newPos, dir, SSD1306_get_fps());

  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString(buffer, Font_7x10, White);
  ssd1306_UpdateScreen();
}

void Task_LED(void)
{
  gpioPinToggle(LED);
}
void Task_UART(void)
{
//	if (uartAvailable(_DEF_UART1) > 0)
//	{
//	  uint8_t rx_data;
//	  rx_data = uartRead(_DEF_UART1);
//
//	  uartPrintf(_DEF_UART1, "Rx : 0x%X\r\n", rx_data);
//	}
  cliMain();
}

void Task_Gcode(void)
{

}

void Task_Motion(void)
{

}

void Task_Button(void)
{

}
