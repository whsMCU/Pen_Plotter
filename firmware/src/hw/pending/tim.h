/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIM_H__
#define __TIM_H__

#include "hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _USE_HW_TIM
/* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern TIM_HandleTypeDef htim3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void tim_Init(void);
bool tim_Begin(uint8_t ch);

void timAttachInterrupt(uint8_t ch, void (*func)());

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif

#endif /* __TIM_H__ */

