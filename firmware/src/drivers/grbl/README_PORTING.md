# GRBL 1.1h → STM32F103C8T6 (Blue Pill) 포팅

업로드해주신 GRBL 1.1h **원본 소스**를 기반으로 포팅했습니다. 이전 답변(원본
없이 기억으로 재구성한 버전)과 달리, 이번에는 실제 파일을 열어 한 줄 한 줄
확인하며 수정했습니다.

## 포팅 전략

AVR 코드는 `STEP_PORT = (STEP_PORT & ~STEP_MASK) | bits;` 처럼 포트 레지스터를
직접 비트연산하는 스타일입니다. STM32의 `GPIOx->ODR`도 똑같이 비트연산 가능한
32비트 레지스터이므로, **`STEP_PORT` 같은 매크로를 `GPIOA->ODR`에 그대로
매핑**하면 원본 `.c` 파일을 거의 건드리지 않고 재사용할 수 있습니다. 이 원칙
덕분에 `limits.c`, `system.c`, `probe.c`, `coolant_control.c`,
`spindle_control.c`는 **단 한 줄도 수정하지 않았습니다.**

## 파일별 변경 내역

| 파일 | 상태 | 설명 |
|---|---|---|
| `stm32_port.h` | **신규** | 모든 핀/레지스터 매핑의 핵심. AVR 매크로 이름(`STEP_PORT`, `OCR1A`, `TIMSK1`...)을 실제 STM32 레지스터나 무해한 더미 변수에 연결 |
| `stm32_hal.c` | **신규** | 클럭(72MHz)/GPIO/TIM2·TIM3(스텝)/TIM4(스핀들 PWM)/EXTI 실제 초기화 + ISR 디스패치 |
| `avr_compat.h` | **신규** | `sei()/cli()`, `_delay_ms/us()`, `PROGMEM`, `ISR()` 매크로 등 C 라이브러리 수준 셰임 |
| `eeprom.c` | **전체 교체** | AVR 하드웨어 EEPROM → 플래시 마지막 1페이지(1KB) 에뮬레이션. 함수 시그니처는 원본과 동일 |
| `stepper.c` | **일부 수정** | `st_wake_up()`, `st_go_idle()`, 두 ISR, `stepper_init()`의 타이머 설정 블록만 수정. 나머지(AMASS, Bresenham 등 알고리즘)는 **원본 그대로** |
| `serial.c` | **일부 수정** | `serial_init()` 본문만 교체. ISR 바디는 `UDR0`→`USART1->DR` 매핑으로 원본 그대로 |
| `system.c` | **일부 수정** | `SREG` 원자적 접근 패턴(8곳)을 `__get_PRIMASK()/__set_PRIMASK()`로 치환. 나머지는 원본 그대로 |
| `main.c` | **일부 수정** | 맨 앞에 `HAL_Init()` + STM32 하드웨어 초기화 4줄 추가. 나머지는 원본 그대로 |
| `config.h` | **1줄 수정** | `CPU_MAP_ATMEGA328P` → `CPU_MAP_STM32F103` |
| `grbl.h` | **일부 수정** | AVR 헤더(`avr/io.h` 등) → `stm32f1xx_hal.h` + `avr_compat.h`, `stm32_port.h` include 추가 |
| `limits.c`, `system.c`(핀 초기화), `probe.c`, `coolant_control.c`, `spindle_control.c` | **무수정** | `stm32_port.h`의 매크로 매핑만으로 그대로 동작 |
| 그 외 전부 (`planner.c`, `gcode.c`, `motion_control.c`, `protocol.c`, `report.c`, `settings.c` 등) | **무수정** | 하드웨어를 건드리지 않는 순수 알고리즘/파서 코드라 원본 그대로 사용 |

## 핀 배치 (Blue Pill)

```
GPIOA
  PA0 X_STEP   PA1 X_DIR
  PA2 Y_STEP   PA3 Y_DIR
  PA4 Z_STEP   PA5 Z_DIR
  PA6 STEPPERS_DISABLE
  PA9 USART1_TX   PA10 USART1_RX
GPIOB
  PB0 X_LIMIT  PB1 Y_LIMIT  PB2 Z_LIMIT   (EXTI0/1/2)
  PB3 SPINDLE_ENABLE   PB4 SPINDLE_DIRECTION   ※JTAG 핀이라 SWJ_NOJTAG 리맵 필요(코드에 포함됨)
  PB6 SPINDLE_PWM (TIM4_CH1, 1kHz)
  PB7 COOLANT_FLOOD   PB8 COOLANT_MIST
  PB9 RESET  PB10 FEED_HOLD  PB11 CYCLE_START  PB12 SAFETY_DOOR   (EXTI9~12)
  PB13 PROBE
```

## 핵심 동작 원리

- **TIM2** = AVR TIMER1(스텝 주기) 대응. 72MHz 무분주로 카운트하며,
  `stepper.c`가 매 세그먼트마다 `OCR1A`(=`TIM2->ARR`)를 직접 갱신합니다.
  F_CPU를 72MHz로 정의했기 때문에 AMASS 컷오프 주파수 계산식이 원본과
  동일한 상대적 스케일로 자동 유지됩니다.
- **TIM3** = AVR TIMER0(펄스 폭 리셋) 대응. 1MHz 틱 + One Pulse Mode로,
  `st_wake_up()`이 `settings.pulse_microseconds`를 `TIM3->ARR`에 직접
  기록하고, ISR에서 `CNT=0; CEN=1`로 재시작합니다.
- **TIM4 CH1** = 스핀들 PWM. `spindle_control.c`는 수정 없이 `TIM4->CCER`
  (채널 on/off)와 `TIM4->CCR1`(듀티)을 직접 건드립니다.
- **EXTI0/1/2, EXTI9~12** = 리밋/컨트롤 핀. AVR의 PCINT(그룹 인터럽트)와
  달리 STM32는 핀별 EXTI 라인이라, `stm32_hal.c`의 `EXTIx_IRQHandler`가
  `limits.c`/`system.c`의 원본 ISR 바디(이름만 `limit_pin_isr_body`,
  `control_pin_isr_body`로 바뀜)를 그대로 호출합니다.

## CubeIDE 설정

1. MCU: **STM32F103C8Tx**
2. 이 폴더의 모든 파일을 CubeIDE 프로젝트의 `Core/Src`, `Core/Inc`에
   넣고 컴파일하면 됩니다. (CubeMX로 프로젝트 뼈대만 생성하고, `main.c`는
   CubeMX가 만든 것을 **이 파일로 덮어쓰세요** — `SystemClock_Config` 등을
   중복 정의하지 않도록)
3. `.ioc`에서 클럭/핀 설정을 별도로 할 필요 없습니다 — `stm32_hal.c`가
   코드 레벨에서 전부 설정합니다. CubeMX GUI 설정과 충돌하지 않도록
   `.ioc` 쪽 클럭/GPIO/타이머 설정은 비워두거나 무시하세요.
4. HSE 8MHz 크리스탈 기준입니다(Blue Pill 기본). 다른 크리스탈이면
   `stm32_hal.c`의 `stm32_clock_config()`의 `PLLMUL` 값을 조정하세요.

## ⚠️ 실제 기계 연결 전 확인할 것

- **오실로스코프로 STEP/DIR 파형 확인** — 특히 고속 이송($100~$102 스텝/mm,
  최대 이송속도 설정) 근처에서 펄스 폭과 간격이 `$0`(step pulse time),
  `$110~$112`(max rate) 설정과 일치하는지
- **리밋/컨트롤 스위치 극성** — NO/NC에 따라 `$5`(invert limit),
  `$args` 등 설정값 조정
- **PB3/PB4가 실제로 GPIO로 동작하는지** — 일부 Blue Pill 클론은 부팅 시
  ST-Link/디버거가 SWD로 잡고 있으면 `__HAL_AFIO_REMAP_SWJ_NOJTAG()`
  적용 후에도 초기 몇 ms 핀 상태가 불안정할 수 있음
- **플래시 EEPROM 에뮬레이션 주소(`0x0800FC00`)** — STM32F103C8은 보통
  64KB지만 실제로는 128KB 다이가 들어간 개체가 많습니다(공식 스펙 외
  여유 공간). 안전하게 64KB 기준 마지막 페이지를 썼으니 별도 조정은
  불필요하지만, 링커 스크립트의 FLASH 크기도 64KB로 맞춰두세요.
- 이 포팅은 컴파일러로 직접 빌드해보지 못한 상태입니다(이 환경에 ARM
  툴체인이 없음). CubeIDE에서 처음 빌드 시 사소한 타입/헤더 이슈가
  나올 수 있으니, 에러 메시지를 공유해주시면 바로 고쳐드릴게요.
