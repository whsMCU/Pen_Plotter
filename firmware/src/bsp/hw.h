/*
 * hw.h
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */

#ifndef SRC_HW_HW_H_
#define SRC_HW_HW_H_

#include "hw_def.h"
#include "def.h"
#include "bsp.h"
#include "gpio.h"
#include "tim.h"
//#include "can.h"
#include "uart.h"
#include "i2c.h"
//#include "lcd.h"
#include "ssd1306.h"
#include "maths.h"
#include "macros.h"
#include "cli.h"
#include "cli_gui.h"
#include "log.h"

#include "RotaryEncoder.h"
#include "scheduler.h"


// Grbl versioning system
#define GRBL_VERSION "1.1h"
#define GRBL_VERSION_BUILD "20190825"

// Define the Grbl system include files. NOTE: Do not alter organization.
#include "config.h"
#include "nuts_bolts.h"
#include "settings.h"
#include "system.h"
#include "defaults.h"
#include "cpu_map.h"
#include "coolant_control.h"
#include "eeprom.h"
#include "gcode.h"
#include "limits.h"
#include "motion_control.h"
#include "planner.h"
#include "print.h"
#include "probe.h"
#include "protocol.h"
#include "report.h"
#include "serial.h"
#include "spindle_control.h"
#include "stepper.h"
#include "jog.h"


void hwInit(void);


#endif /* SRC_HW_HW_H_ */
