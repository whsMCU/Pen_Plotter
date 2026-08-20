/*
  stm32_hal.c - STM32F103C8T6 (Blue Pill) 실제 하드웨어 초기화
  Part of Grbl (STM32 포팅)

  원본 GRBL 1.1h에는 없던 파일입니다. main.c에서 각 _init() 함수보다
  먼저 호출되어 클럭/GPIO/타이머/EXTI/USART 하드웨어를 실제로 구성합니다.
  이후 stepper.c/limits.c/system.c/spindle_control.c 등은 stm32_port.h가
  제공하는 매크로(STEP_PORT, LIMIT_PIN, TIM4->CCER 등)를 통해 이 하드웨어를
  건드립니다.
*/
#include "grbl.h"

/* stm32_port.h의 DUMMY_REG가 참조하는 실제 저장소 (부작용 없는 더미 레지스터) */
volatile uint32_t stm32_dummy_reg;

/* serial.c / limits.c / system.c 에서 ISR() 매크로로 이름이 바뀌어 정의되는 실제 함수들 */
extern void usart1_rx_complete_isr(void);
extern void usart1_udr_empty_isr(void);

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
  /* USER CODE END USART1_IRQn 0 */
  //HAL_UART_IRQHandler(&huart1);

  if (USART1->SR & USART_SR_RXNE) {
    usart1_rx_complete_isr();               // USART1->DR 읽기 -> RXNE 자동 클리어
  }
  if ((USART1->SR & USART_SR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)) {
    usart1_udr_empty_isr();                 // USART1->DR 쓰기 -> TXE 자동 클리어, 또는 TXEIE 직접 disable
  }
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

extern void limit_pin_isr_body(void);
extern void control_pin_isr_body(void);


/* ================= 시스템 클럭: HSE 8MHz -> PLL x9 -> 72MHz ================= */
void stm32_clock_config(void)
{
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  RCC_OscInitTypeDef osc = {0};
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState = RCC_HSE_ON;
  osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLMUL = RCC_PLL_MUL9; // 8MHz x 9 = 72MHz
  HAL_RCC_OscConfig(&osc);

  RCC_ClkInitTypeDef clk = {0};
  clk.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;   // HCLK  = 72MHz
  clk.APB1CLKDivider = RCC_HCLK_DIV2;    // PCLK1 = 36MHz (TIM2/TIM3/TIM4는 x2 되어 72MHz)
  clk.APB2CLKDivider = RCC_HCLK_DIV1;    // PCLK2 = 72MHz (USART1)
  HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);

  /* DWT 사이클카운터 - stm32_delay_us()용 */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void stm32_delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = us * (SystemCoreClock / 1000000UL);
  while ((DWT->CYCCNT - start) < cycles) { /* busy-wait */ }
}


/* ================= GPIO 초기화 ================= */
void stm32_board_gpio_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  // PB3/PB4는 기본적으로 JTDO/NJTRST(JTAG)로 예약되어 있음. SWD만 남기고
  // JTAG를 해제해야 스핀들 인에이블/방향 핀으로 사용 가능.
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

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
}


/* ================= EXTI: NVIC 활성화 (트리거/AFIO 매핑은 GPIO_Init에서 이미 처리됨) ========= */
void stm32_exti_init(void)
{
  HAL_NVIC_SetPriority(EXTI0_IRQn, 3, 0);      HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 3, 0);      HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 0);      HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 0);    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0);  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void exti_dispatch(uint32_t line_mask)
{
  if (line_mask & LIMIT_MASK)   { limit_pin_isr_body(); }
  if (line_mask & CONTROL_MASK) { control_pin_isr_body(); }
}

void EXTI0_IRQHandler(void)     { if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0)) { __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0); exti_dispatch(GPIO_PIN_0); } }
void EXTI1_IRQHandler(void)     { if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_1)) { __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1); exti_dispatch(GPIO_PIN_1); } }
void EXTI2_IRQHandler(void)     { if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_2)) { __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2); exti_dispatch(GPIO_PIN_2); } }

void EXTI9_5_IRQHandler(void)
{
  for (uint32_t pin = GPIO_PIN_5; pin <= GPIO_PIN_9; pin <<= 1) {
    if (__HAL_GPIO_EXTI_GET_IT(pin)) { __HAL_GPIO_EXTI_CLEAR_IT(pin); exti_dispatch(pin); }
  }
}

void EXTI15_10_IRQHandler(void)
{
  for (uint32_t pin = GPIO_PIN_10; pin <= GPIO_PIN_15; pin <<= 1) {
    if (__HAL_GPIO_EXTI_GET_IT(pin)) { __HAL_GPIO_EXTI_CLEAR_IT(pin); exti_dispatch(pin); }
  }
}


/* ================= TIM2(스텝 주기) + TIM3(펄스폭) ================= */
void stm32_stepper_timer_init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();

  // TIM2: 72MHz 그대로(무분주) 카운트. stepper.c의 OCR1A(=TIM2->ARR)가 매 세그먼트마다
  // 직접 주기를 재설정하고, CNT/CEN/DIER는 st_wake_up()/st_go_idle()이 제어함.
  TIM2->PSC = 0;
  TIM2->ARR = 0xFFFF;
  TIM2->CR1 = 0;   // CEN=0 (idle 상태로 시작, st_wake_up()에서 켬)
  TIM2->DIER = 0;  // UIE=0
  TIM2->EGR = TIM_EGR_UG; // 프리스케일러/ARR 즉시 반영
  TIM2->SR = 0;

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);

  // TIM3: 1MHz(1us) 틱, 원펄스 모드. ARR(펄스폭)은 st_wake_up()이 매번 설정.
  TIM3->PSC = (72 - 1); // 72MHz/72 = 1MHz
  TIM3->ARR = 10 - 1;   // 기본 10us (st_wake_up에서 실제 값으로 재설정)
  TIM3->CR1 = TIM_CR1_OPM; // One Pulse Mode: ARR 도달 시 자동으로 CEN=0
  TIM3->DIER = TIM_DIER_UIE;
  TIM3->EGR = TIM_EGR_UG; //Generate an update event
  TIM3->SR = 0;

  HAL_NVIC_SetPriority(TIM3_IRQn, 1, 1);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}


/* ================= TIM4 CH1: 스핀들 PWM (PB6) ================= */
void stm32_spindle_pwm_init(void)
{
  __HAL_RCC_TIM4_CLK_ENABLE();

  TIM4->PSC = (72 - 1);        // 1MHz 카운트 클럭
  TIM4->ARR = SPINDLE_PWM_MAX_VALUE; // 999 -> 1kHz PWM, 1000단계 분해능
  TIM4->CCR1 = 0;
  TIM4->CCMR1 = (TIM4->CCMR1 & ~TIM_CCMR1_OC1M) | (6 << TIM_CCMR1_OC1M_Pos); // PWM mode 1
  TIM4->CCMR1 |= TIM_CCMR1_OC1PE;
  TIM4->CCER &= ~TIM_CCER_CC1E;  // spindle_init()->spindle_stop()이 필요시 켬
  TIM4->CR1 |= TIM_CR1_ARPE;
  TIM4->EGR = TIM_EGR_UG;
  TIM4->CR1 |= TIM_CR1_CEN; // 타이머 자체는 항상 구동, PWM 출력 on/off는 CCER(CC1E)로 제어
}
