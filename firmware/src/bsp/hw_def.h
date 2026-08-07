/*
 *
 *  Created on: Dec 6, 2020
 *      Author: baram
 */

#ifndef SRC_HW_HW_DEF_H_
#define SRC_HW_HW_DEF_H_


//#define _USE_HW_CDC
//#define _USE_HW_USB
#define _USE_HW_FLASH

#define _USE_HW_TIM
#define      HW_TIM_MAX_CH          2

#define _USE_HW_GPIO
#define      HW_GPIO_MAX_CH         10

//#define _USE_HW_BUTTON
//#define      HW_BUTTON_MAX_CH       3
//#define      HW_BUTTON_OBJ_USE      1

#define _USE_HW_UART
#define      HW_UART_MAX_CH         2
//#define      HW_UART_CH_SWD         _DEF_USB
//#define      HW_UART_CH_CLI         _DEF_USB

#define _USE_HW_I2C
#define      HW_I2C_MAX_CH          1


#define _USE_HW_CLI
#define      HW_CLI_CMD_NAME_MAX    16
#define      HW_CLI_CMD_LIST_MAX    16
#define      HW_CLI_LINE_HIS_MAX    4
#define      HW_CLI_LINE_BUF_MAX    32

//#define _USE_HW_LOG
//#define      HW_LOG_CH              HW_UART_CH_SWD

//#define _USE_HW_CLI_GUI
//#define      HW_CLI_GUI_WIDTH       80
//#define      HW_CLI_GUI_HEIGHT      24

#define _USE_HW_LCD
#define      HW_LCD_MODEL           1
#if HW_LCD_MODEL == 0
#define _USE_HW_ST7735
#define      HW_ST7735_MODEL        0
#define      HW_LCD_WIDTH           160
#define      HW_LCD_HEIGHT          80
#endif
#if HW_LCD_MODEL == 1
#define _USE_HW_SSD1306
#define      HW_SSD1306_MODEL       0
#define      HW_LCD_WIDTH           1
#define      HW_LCD_HEIGHT          1
#endif
#if HW_LCD_MODEL == 2
#define _USE_HW_ST7735
#define      HW_ST7735_MODEL        1
#define      HW_LCD_WIDTH           160
#define      HW_LCD_HEIGHT          128
#endif


#define FAST_DATA_ZERO_INIT
#define FAST_CODE


#ifndef M_PI
#define M_PI       3.14159265358979323846f
#endif /* M_PI */

#define D2R (3.141592653f / 180.0f)
#define R2D (180.0f / 3.141592653f)

#define TRUE 1
#define FALSE 0

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
//#define abs(x) ((x) > 0 ? (x) : -(x))
#define zofs(x, y, z) ((x) > (y+z) ? (x) : ((x) < (y-z) ? (x) : (y)))
#define map(x, in_min, in_max, out_min, out_max) ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

#define ROUND(x, dig)  ( floor((x) * pow((float)(10), dig) + 0.5f) / pow((float)(10), dig) )

#define CONVERT_PARAMETER_TO_FLOAT(param) (0.001f * param)
#define CONVERT_PARAMETER_TO_PERCENT(param) (0.01f * param)


#endif /* SRC_HW_HW_DEF_H_ */
