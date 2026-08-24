/*
 * gpio.c
 *
 *  Created on: 2021. 8. 14.
 *      Author: WANG
 */


#include "gpio.h"
#include "grbl.h"

#ifdef _USE_HW_GPIO

typedef struct
{
  GPIO_TypeDef *port;
  uint32_t      pin;
  uint8_t       mode;
  GPIO_PinState on_state;
  GPIO_PinState off_state;
  bool          init_value;
} gpio_tbl_t;


const gpio_tbl_t gpio_tbl[GPIO_MAX_CH] =
{
	{GPIOB, GPIO_PIN_2,   _DEF_OUTPUT,  		    GPIO_PIN_SET,   GPIO_PIN_RESET, _DEF_LOW},  //  0. LED
	{GPIOC, GPIO_PIN_13,  _DEF_INPUT_IT_RF,  	  GPIO_PIN_SET,   GPIO_PIN_RESET, _DEF_LOW},  //  1. RotaryEncoder_1
	{GPIOC, GPIO_PIN_14,  _DEF_INPUT_IT_RF,   	GPIO_PIN_SET,   GPIO_PIN_RESET, _DEF_LOW},  //  2. RotaryEncoder_2
	{GPIOC, GPIO_PIN_15,  _DEF_INPUT_IT_RISING, GPIO_PIN_SET,   GPIO_PIN_RESET, _DEF_LOW},  //  3. Push_button
};


#ifdef _USE_HW_CLI
static void cliGpio(cli_args_t *args);
#endif

bool gpioInit(void)
{
  bool ret = true;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  for (int i=0; i<GPIO_MAX_CH; i++)
  {
  	gpioPinMode(i, gpio_tbl[i].mode);
  	gpioPinWrite(i, gpio_tbl[i].init_value);
  }

  GPIO_InitTypeDef gi = {0};

  // --- STEP / DIRECTION / STEPPERS_DISABLE : GPIOA 출력 ---
  gi.Pin = (1<<X_STEP_BIT)|(1<<Y_STEP_BIT)|(1<<Z_STEP_BIT)
         | (1<<X_DIRECTION_BIT)|(1<<Y_DIRECTION_BIT)|(1<<Z_DIRECTION_BIT)
         | (1<<STEPPERS_DISABLE_BIT);
  gi.Mode = GPIO_MODE_OUTPUT_PP;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gi);

  // --- USART1 TX(PA9) ---
  gi.Pin = GPIO_PIN_9;
  gi.Mode = GPIO_MODE_AF_PP;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gi);

  // --- USART1 RX(PA10) ---
  gi.Pin = GPIO_PIN_10;
  gi.Mode = GPIO_MODE_INPUT;
  gi.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gi);

  // --- SPINDLE_ENABLE(PB3) / SPINDLE_DIRECTION(PB4) / COOLANT(PB7,PB8) : GPIOB 출력 ---
  gi.Pin = (1<<SPINDLE_ENABLE_BIT)|(1<<SPINDLE_DIRECTION_BIT)
         | (1<<COOLANT_FLOOD_BIT)|(1<<COOLANT_MIST_BIT);
  gi.Mode = GPIO_MODE_OUTPUT_PP;
  gi.Pull = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &gi);

  // --- SPINDLE_PWM(PB6, TIM4_CH1) : AF 출력 ---
  gi.Pin = (1<<SPINDLE_PWM_BIT);
  gi.Mode = GPIO_MODE_AF_PP;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &gi);

  // --- LIMIT(PB0,1,2) / CONTROL(PB9~12) : 입력, 풀업, 양쪽 엣지 인터럽트 ---
  gi.Pin = LIMIT_MASK | CONTROL_MASK;
  gi.Mode = GPIO_MODE_IT_RISING_FALLING;
  gi.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &gi);

  // --- PROBE(PB13) : 입력, 풀업, 폴링만 함(인터럽트 불필요) ---
  gi.Pin = (1<<PROBE_BIT);
  gi.Mode = GPIO_MODE_INPUT;
  gi.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &gi);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 3, 0);      HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);      HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 0);    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0);  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

#ifdef _USE_HW_CLI
  cliAdd("gpio", cliGpio);
#endif
#ifdef _USE_HW_LOG
  logPrintf("[%s] gpio_Init()\r\n", ret ? "OK":"NG");
#endif
  return ret;
}

bool gpioPinMode(uint8_t ch, uint8_t mode)
{
  bool ret = true;
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  if (ch >= GPIO_MAX_CH)
  {
    return false;
  }

  switch(mode)
  {
    case _DEF_INPUT:
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      break;

    case _DEF_INPUT_PULLUP:
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      break;

    case _DEF_INPUT_PULLDOWN:
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
      GPIO_InitStruct.Pull = GPIO_PULLDOWN;
      break;

    case _DEF_INPUT_IT_RISING:
      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      break;

    case _DEF_INPUT_IT_RF:
      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      break;

    case _DEF_OUTPUT:
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      break;

    case _DEF_OUTPUT_PULLUP:
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_PULLUP;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      break;

    case _DEF_OUTPUT_PULLDOWN:
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
      GPIO_InitStruct.Pull = GPIO_PULLDOWN;
      break;

    case _DEF_INPUT_AF_PP:
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      break;

    case _DEF_INPUT_ANALOG:
      GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      break;
  }

  GPIO_InitStruct.Pin = gpio_tbl[ch].pin;
  HAL_GPIO_Init(gpio_tbl[ch].port, &GPIO_InitStruct);

  return ret;
}

void gpioPinWrite(uint8_t ch, bool value)
{
  if (ch >= GPIO_MAX_CH)
  {
    return;
  }

  if (value)
  {
    HAL_GPIO_WritePin(gpio_tbl[ch].port, gpio_tbl[ch].pin, gpio_tbl[ch].on_state);
  }
  else
  {
    HAL_GPIO_WritePin(gpio_tbl[ch].port, gpio_tbl[ch].pin, gpio_tbl[ch].off_state);
  }
}

void gpioPinWrite_reg(uint8_t ch, bool value)
{
  if (ch >= GPIO_MAX_CH)
  {
    return;
  }

  if (value)
  {
    gpio_tbl[ch].port->BSRR = gpio_tbl[ch].pin;
  }
  else
  {
    gpio_tbl[ch].port->BRR = gpio_tbl[ch].pin;
  }
}

bool gpioPinRead(uint8_t ch)
{
  bool ret = false;

  if (ch >= GPIO_MAX_CH)
  {
    return false;
  }

  if (HAL_GPIO_ReadPin(gpio_tbl[ch].port, gpio_tbl[ch].pin) == gpio_tbl[ch].on_state)
  {
    ret = true;
  }

  return ret;
}

void gpioPinToggle(uint8_t ch)
{
  if (ch >= GPIO_MAX_CH)
  {
    return;
  }

  HAL_GPIO_TogglePin(gpio_tbl[ch].port, gpio_tbl[ch].pin);
}

uint32_t getgpioPin(uint8_t ch)
{
	return gpio_tbl[ch].pin;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

}

#ifdef _USE_HW_CLI
void cliGpio(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "show") == true)
  {
    while(cliKeepLoop())
    {
      for (int i=0; i<GPIO_MAX_CH; i++)
      {
        cliPrintf("%d", gpioPinRead(i));
      }
      cliPrintf("\r\n");
      delay(100);
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read") == true)
  {
    uint8_t ch;

    ch = (uint8_t)args->getData(1);

    while(cliKeepLoop())
    {
      cliPrintf("gpio read %d : %d\r\n", ch, gpioPinRead(ch));
      delay(100);
    }

    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "write") == true)
  {
    uint8_t ch;
    uint8_t data;

    ch   = (uint8_t)args->getData(1);
    data = (uint8_t)args->getData(2);

    gpioPinWrite(ch, data);

    cliPrintf("gpio write %d : %d\r\n", ch, data);
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("gpio show\r\n");
    cliPrintf("gpio read ch[0~%d]\r\n", GPIO_MAX_CH-1);
    cliPrintf("gpio write ch[0~%d] 0:1\r\n", GPIO_MAX_CH-1);
  }
}
#endif

#endif
