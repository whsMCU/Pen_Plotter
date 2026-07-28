/*
 * flash.h
 *
 *  Created on: Sep 6, 2024
 *      Author: WANG
 */

#ifndef SRC_HW_FLASH_H_
#define SRC_HW_FLASH_H_

#include "hw.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _USE_HW_FLASH


bool flashInit(void);
bool flashErase(uint32_t addr, uint32_t length);
bool flashWrite(uint32_t addr, uint8_t *p_data, uint32_t length);
bool flashRead(uint32_t addr, uint8_t *p_data, uint32_t length);


#endif

#ifdef __cplusplus
}
#endif

#endif /* SRC_HW_FLASH_H_ */
