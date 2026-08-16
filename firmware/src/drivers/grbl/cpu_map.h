/*
  stm32_port.h - STM32F103C8T6 (Blue Pill) 핀/레지스터 매핑
  원본 cpu_map.h는 CPU_MAP_ATMEGA328P가 정의되지 않으면 내용이 비므로,
  이 파일이 사실상 STM32용 cpu_map.h 역할을 합니다.

  설계 원칙: AVR은 PORT/PIN 레지스터를 직접 비트연산(|=, &=, ^=)하는 스타일이라,
  STEP_PORT 같은 매크로를 GPIOx->ODR(출력)/IDR(입력)에 그대로 매핑하면
  stepper.c, limits.c, system.c, probe.c, coolant_control.c, spindle_control.c
  원본 코드를 거의 수정 없이 재사용할 수 있습니다.

  핀 배치
  ─────────────────────────────────────────
  GPIOA
    PA0 X_STEP   PA1 X_DIR
    PA2 Y_STEP   PA3 Y_DIR
    PA4 Z_STEP   PA5 Z_DIR
    PA6 STEPPERS_DISABLE
    PA9 USART1_TX  PA10 USART1_RX
  GPIOB
    PB0 X_LIMIT  PB1 Y_LIMIT  PB2 Z_LIMIT
    PB3 SPINDLE_ENABLE   PB4 SPINDLE_DIRECTION
    PB8 SPINDLE_PWM (TIM4_CH3)
    PB14 COOLANT_FLOOD    PB15 COOLANT_MIST
    PB9 RESET  PB10 FEED_HOLD  PB11 CYCLE_START  PB12 SAFETY_DOOR
    PB13 PROBE
  ─────────────────────────────────────────
*/
#ifndef stm32_port_h
#define stm32_port_h

#ifdef CPU_MAP_STM32F103

/* DDR(방향 설정) 역할을 하는 줄들은 실제 GPIO 모드(CRL/CRH) 설정을
   stm32_hal.c의 stm32_board_gpio_init()에서 한 번에 처리하므로 여기서는
   부작용 없는 더미 변수로 흡수합니다. */
extern volatile uint32_t stm32_dummy_reg;
#define DUMMY_REG stm32_dummy_reg

/* ================= STEP / DIRECTION / STEPPERS_DISABLE (GPIOA) ================= */
#define STEP_DDR        DUMMY_REG
#define STEP_PORT       (GPIOA->ODR)
#define X_STEP_BIT      0
#define Y_STEP_BIT      2
#define Z_STEP_BIT      4
#define STEP_MASK       ((1<<X_STEP_BIT)|(1<<Y_STEP_BIT)|(1<<Z_STEP_BIT))

#define DIRECTION_DDR    DUMMY_REG
#define DIRECTION_PORT   (GPIOA->ODR)
#define X_DIRECTION_BIT  1
#define Y_DIRECTION_BIT  3
#define Z_DIRECTION_BIT  5
#define DIRECTION_MASK   ((1<<X_DIRECTION_BIT)|(1<<Y_DIRECTION_BIT)|(1<<Z_DIRECTION_BIT))

#define STEPPERS_DISABLE_DDR    DUMMY_REG
#define STEPPERS_DISABLE_PORT   (GPIOA->ODR)
#define STEPPERS_DISABLE_BIT    6
#define STEPPERS_DISABLE_MASK   (1<<STEPPERS_DISABLE_BIT)

/* ================= LIMIT SWITCHES (GPIOB, EXTI0/1/2) ================= */
#define LIMIT_DDR        DUMMY_REG
#define LIMIT_PIN        (GPIOB->IDR)
#define LIMIT_PORT       (GPIOB->ODR)
#define X_LIMIT_BIT      0
#define Y_LIMIT_BIT      1
#define Z_LIMIT_BIT      2
#define LIMIT_MASK       ((1<<X_LIMIT_BIT)|(1<<Y_LIMIT_BIT)|(1<<Z_LIMIT_BIT))
#define LIMIT_INT        0                    /* AVR의 "그룹 인터럽트 enable"에 대응 없음 -> 더미 */
#define LIMIT_INT_vect   limit_pin_isr_body    /* ISR(LIMIT_INT_vect) -> void limit_pin_isr_body(void) */
#define LIMIT_PCMSK      (EXTI->IMR)           /* EXTI 라인 번호 = GPIO 핀 번호이므로 마스크 그대로 사용 가능 */

/* ================= CONTROL PINS: RESET/FEED_HOLD/CYCLE_START/DOOR (GPIOB, EXTI9~12) ========= */
#define CONTROL_DDR       DUMMY_REG
#define CONTROL_PIN        (GPIOB->IDR)
#define CONTROL_PORT       (GPIOB->ODR)
#define CONTROL_RESET_BIT         9
#define CONTROL_FEED_HOLD_BIT     10
#define CONTROL_CYCLE_START_BIT   11
#define CONTROL_SAFETY_DOOR_BIT   12
#define CONTROL_INT        0
#define CONTROL_INT_vect   control_pin_isr_body
#define CONTROL_PCMSK      (EXTI->IMR)
#define CONTROL_MASK       ((1<<CONTROL_RESET_BIT)|(1<<CONTROL_FEED_HOLD_BIT)|(1<<CONTROL_CYCLE_START_BIT)|(1<<CONTROL_SAFETY_DOOR_BIT))
#define CONTROL_INVERT_MASK  CONTROL_MASK

#define PCICR  DUMMY_REG   /* AVR PCICR(그룹 인터럽트 enable) 대응 없음 -> 더미 */

/* ================= PROBE (GPIOB) ================= */
#define PROBE_DDR   DUMMY_REG
#define PROBE_PIN   (GPIOB->IDR)
#define PROBE_PORT  (GPIOB->ODR)
#define PROBE_BIT   13
#define PROBE_MASK  (1<<PROBE_BIT)

/* ================= COOLANT (GPIOB) ================= */
#define COOLANT_FLOOD_DDR   DUMMY_REG
#define COOLANT_FLOOD_PORT  (GPIOB->ODR)
#define COOLANT_FLOOD_BIT   14
#define COOLANT_MIST_DDR    DUMMY_REG
#define COOLANT_MIST_PORT   (GPIOB->ODR)
#define COOLANT_MIST_BIT    15

/* ================= SPINDLE ENABLE/DIRECTION (GPIOB) + PWM (TIM4 CH1) ================= */
#define SPINDLE_ENABLE_DDR    DUMMY_REG
#define SPINDLE_ENABLE_PORT   (GPIOB->ODR)
#define SPINDLE_ENABLE_BIT    3
#define SPINDLE_DIRECTION_DDR    DUMMY_REG
#define SPINDLE_DIRECTION_PORT   (GPIOB->ODR)
#define SPINDLE_DIRECTION_BIT    4

/* TIM4 ARR=999로 1000단계 PWM (stm32_hal.c의 stm32_spindle_pwm_init()과 반드시 일치) */
#define SPINDLE_PWM_MAX_VALUE     999
#ifndef SPINDLE_PWM_MIN_VALUE
  #define SPINDLE_PWM_MIN_VALUE   1
#endif
#define SPINDLE_PWM_OFF_VALUE     0
#define SPINDLE_PWM_RANGE         (SPINDLE_PWM_MAX_VALUE-SPINDLE_PWM_MIN_VALUE)

#define SPINDLE_TCCRA_REGISTER    (TIM4->CCER)   /* CC1E 비트 on/off = PWM 채널 연결/해제 (AVR COM 비트와 동일 역할) */
#define SPINDLE_TCCRB_REGISTER    DUMMY_REG       /* 실제 타이머 base 설정은 stm32_hal.c에서 1회 수행 */
#define SPINDLE_OCR_REGISTER      (TIM4->CCR1)    /* PWM duty 레지스터 */
#define SPINDLE_COMB_BIT          0                /* TIM4->CCER 의 CC1E 비트 위치 */
#define SPINDLE_TCCRA_INIT_MASK   0
#define SPINDLE_TCCRB_INIT_MASK   0

#define SPINDLE_PWM_DDR    DUMMY_REG
#define SPINDLE_PWM_PORT   (GPIOB->ODR)
#define SPINDLE_PWM_BIT    6

/* ================= 타이머 레지스터 매핑 (stepper.c ISR 등에서 그대로 사용) ================= */
#define OCR1A    (TIM2->ARR)   /* 다음 세그먼트의 스텝 주기 (TIM2 = AVR TIMER1 대응) */
#define TCCR0B   DUMMY_REG      /* TIMER0_OVF_vect 안의 "TCCR0B=0"은 TIM3 원펄스모드가 알아서 처리하므로 더미 */

/* ISR(TIMER1_COMPA_vect) / ISR(TIMER0_OVF_vect) -> 실제 NVIC 벡터 이름과 정확히 일치해야
   시작코드(startup_stm32f103xb.s)의 벡터 테이블이 자동으로 연결됨 */
#define TIMER1_COMPA_vect  TIM2_IRQHandler
#define TIMER0_OVF_vect    TIM3_IRQHandler

/* ================= 시리얼 (USART1) ================= */
#define SERIAL_RX     usart1_rx_complete_isr    /* ISR(SERIAL_RX)   -> void usart1_rx_complete_isr(void) */
#define SERIAL_UDRE   usart1_udr_empty_isr      /* ISR(SERIAL_UDRE) -> void usart1_udr_empty_isr(void) */
#define UDR0          (USART1->DR)
#define UCSR0B        (USART1->CR1)
#define UDRIE0        7                          /* USART_CR1의 TXEIE 비트 위치 */

/* ================= 하드웨어 초기화 함수 (stm32_hal.c 구현, main.c/stepper.c에서 호출) ========= */
void stm32_clock_config(void);
void stm32_board_gpio_init(void);
void stm32_exti_init(void);
void stm32_stepper_timer_init(void);
void stm32_spindle_pwm_init(void);

#endif // CPU_MAP_STM32F103
#endif
