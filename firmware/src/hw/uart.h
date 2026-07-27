/*
 * uart.h
 *
 *  Created on: Sep 5, 2024
 *      Author: 차용선
 */

#ifndef SRC_HW_UART_H_
#define SRC_HW_UART_H_

#include "hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _USE_HW_UART

#define UART_MAX_CH         HW_UART_MAX_CH


bool     uartInit(void);
bool     uartOpen(uint8_t ch, uint32_t baud);
uint32_t uartAvailable(uint8_t ch);
uint8_t  uartRead(uint8_t ch);
uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length);
uint32_t uartPrintf(uint8_t ch, char *fmt, ...);
uint32_t uartGetBaud(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif



#endif /* SRC_HW_UART_H_ */
