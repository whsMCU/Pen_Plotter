/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
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
#include "tim.h"

#ifdef _USE_HW_TIM
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/

//#include "cli.h"
/* USER CODE BEGIN 0 */
typedef struct
{
  bool is_start;

  void (*func_cb)(void);

  TIM_HandleTypeDef *h_tim;

} tim_t;
/* USER CODE END 0 */

tim_t tim_tbl[HW_TIM_MAX_CH];

TIM_HandleTypeDef htim3;

#ifdef _USE_HW_CLI
static void cliTimer(cli_args_t *args);
#endif


void tim_Init(void)
{
  for (int i=0; i<HW_TIM_MAX_CH; i++)
  {
    tim_tbl[i].is_start = false;
    tim_tbl[i].func_cb  = NULL;
  }
}

bool tim_Begin(uint8_t ch)
{
  bool ret = false;
  tim_t *p_tim = &tim_tbl[ch];

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  switch(ch)
  {
    case _DEF_TIM3:
      p_tim->h_tim = &htim3;
      p_tim->func_cb = NULL;

      htim3.Instance = TIM3;
      htim3.Init.Prescaler = 72-1;
      htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
      htim3.Init.Period = 65535;
      htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
      htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
      if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
      {
        ret = false;
      }
      sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
      if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
      {
        ret = false;
      }
      sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
      sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
      if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
      {
        ret = false;
      }
      if (HAL_TIM_Base_Start_IT(&htim3) == HAL_OK)
      {
        p_tim->is_start = true;
        ret = true;
      }
      //logPrintf("[%s] tim5_Init()\r\n", ret ? "OK":"NG");
      break;

    default:
      break;
  }
  #ifdef _USE_HW_CLI
    cliAdd("timer", cliTimer);
  #endif

  return ret;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  tim_t *p_tim;
  for(uint8_t i = 0; i<HW_TIM_MAX_CH; i++)
  {
      if (htim->Instance == tim_tbl[i].h_tim->Instance)
      {
        p_tim = &tim_tbl[i];
        if (p_tim->func_cb != NULL)
        {
          (*p_tim->func_cb)();
        }
      }
  }
}

void timAttachInterrupt(uint8_t ch, void (*func)())
{
  tim_t *p_tim = &tim_tbl[ch];

  if (p_tim->is_start == false)     return;

  p_tim->func_cb = func;
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{
	if(tim_baseHandle->Instance==TIM3)
	{
	/* USER CODE BEGIN TIM3_MspInit 0 */

	/* USER CODE END TIM3_MspInit 0 */
	/* TIM3 clock enable */
	__HAL_RCC_TIM3_CLK_ENABLE();

	/* TIM3 interrupt Init */
	HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM3_IRQn);
	/* USER CODE BEGIN TIM3_MspInit 1 */

	/* USER CODE END TIM3_MspInit 1 */
	}
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{
  if(tim_baseHandle->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspDeInit 0 */

  /* USER CODE END TIM3_MspDeInit 0 */
	/* Peripheral clock disable */
	__HAL_RCC_TIM3_CLK_DISABLE();

	/* TIM3 interrupt Deinit */
	HAL_NVIC_DisableIRQ(TIM3_IRQn);
  /* USER CODE BEGIN TIM3_MspDeInit 1 */

  /* USER CODE END TIM3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
#ifdef _USE_HW_CLI
void cliTimer(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "test") == true)
  {
    uint32_t curr_tick, last_tick;
    last_tick = micros();
    while(cliKeepLoop())
    {
      curr_tick = micros();
      cliPrintf("%d\r\n", curr_tick - last_tick);
      last_tick = curr_tick;
      delay(100);
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("timer test\r\n");
  }
}
#endif
#endif
/* USER CODE END 1 */
