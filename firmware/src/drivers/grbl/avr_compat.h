/*
  avr_compat.h - AVR 전용 매크로/함수를 STM32(Cortex-M3)로 매핑하는 호환 계층
  원본 GRBL 소스의 avr/io.h, avr/pgmspace.h, avr/interrupt.h, avr/wdt.h,
  util/delay.h 가 제공하던 것들 중 실제로 grbl 소스에서 쓰이는 것만 구현합니다.
*/
#ifndef avr_compat_h
#define avr_compat_h

#include "stm32f1xx_hal.h"

#ifndef F_CPU
#define F_CPU 72000000UL   // stm32_port.h의 TIM2(스텝 타이머) 클럭과 반드시 일치해야 함
#endif

/* ---- 인터럽트 전역 on/off ---- */
#define sei() __enable_irq()
#define cli() __disable_irq()

/* ---- ISR(vector) 매크로: "void vector(void) { ... }" 로 치환 ----
   vector 자리에는 stm32_port.h에서 각 파일별로 실제 STM32 IRQ 핸들러 이름
   (TIM2_IRQHandler 등) 또는 수동 호출용 내부 함수 이름으로 매핑된 매크로가
   들어옵니다. */
#define ISR(vector) void vector(void)

/* ---- 지연 함수 ---- */
void stm32_delay_us(uint32_t us);
#define _delay_ms(x) stm32_delay_us((uint32_t)(x) * 1000UL)
#define _delay_us(x) stm32_delay_us((uint32_t)(x))

/* ---- PROGMEM 관련: STM32는 통합 메모리라 플래시/RAM 구분 불필요 ---- */
#define PROGMEM
#define PSTR(x) (x)
#define pgm_read_byte_near(addr) (*(const unsigned char *)(addr))
#define pgm_read_byte(addr)      (*(const unsigned char *)(addr))

#endif
