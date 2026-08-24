# grbl-stm32f103

[GRBL 1.1h](https://github.com/gnea/grbl)를 STM32F103C8T6 (Blue Pill) 보드로 포팅한 프로젝트입니다.
AVR ATmega328P 전용 하드웨어 종속 코드만 STM32 HAL 기반으로 새로 작성했고, 플래너·G코드 파서·프로토콜 처리 등 하드웨어 독립적인 로직은 원본을 그대로 사용합니다.

> ⚠️ 이 포팅은 개인 프로젝트/커스텀 CNC·플로터용으로 작업 중입니다. 실제 기계에 연결하기 전 반드시 오실로스코프로 STEP/DIR 파형을 확인하고, 드라이버 전류(Vref)를 모터 정격에 맞게 설정하세요.

## 주요 사양

| 항목 | 내용 |
|---|---|
| 베이스 펌웨어 | GRBL 1.1h |
| 타겟 보드 | STM32F103CBT6 (Blue Pill) |
| 클럭 | HSE 8MHz → PLL x9 → **72MHz** |
| 개발 환경 | STM32CubeIDE / STM32 HAL |
| 시리얼 | USART1, 115200bps |

## 하드웨어 구성

### 타이머 역할

| 타이머 | 원본(AVR) 대응 | 역할 |
|---|---|---|
| TIM2 | TIMER1 (COMPA) | 스텝 주기 생성 (Bresenham/AMASS) |
| TIM3 | TIMER0 (OVF) | STEP 펄스 폭 리셋 (One Pulse Mode) |
| TIM4 CH1 | TIMER2 | 스핀들 PWM (1kHz, 1000단계) |

### 핀 매핑

| 기능 | 핀 | 비고 |
|---|---|---|
| X_STEP / X_DIR | PA0 / PA1 | |
| Y_STEP / Y_DIR | PA2 / PA3 | |
| Z_STEP / Z_DIR | PA4 / PA5 | |
| STEPPERS_DISABLE | PA6 | |
| USART1 TX / RX | PA9 / PA10 | |
| X/Y/Z LIMIT | PB0 / PB1 / PB5 | EXTI0/1/5 |
| SPINDLE_ENABLE / DIR | PB3 / PB4 | ⚠ JTAG 핀, SWJ_NOJTAG 리맵 필요(코드에 포함됨) |
| SPINDLE_PWM | PB6 | TIM4_CH1 |
| COOLANT_FLOOD / MIST | PB7 / PB8 | |
| RESET / FEED_HOLD / CYCLE_START / SAFETY_DOOR | PB9~PB12 | EXTI9~12 |
| PROBE | PB13 | 폴링 방식 |

### 인터럽트 우선순위 (NVIC)

```
TIM3 (0, 최고) > TIM2 (1) > USART1 (2~3) > EXTI (3, 최저)
```

TIM3(펄스 리셋)가 TIM2(스텝 주기)보다 **반드시 더 높아야** 합니다. TIM2 ISR이 다음 세그먼트를 준비하는 동안에도 TIM3가 끼어들어 정확한 시점에 STEP 핀을 내릴 수 있어야, `$0`(pulse_microseconds) 설정대로 펄스 폭이 정확하게 유지됩니다. (원본 AVR은 `sei()`로 이 역할을 했지만, ARM NVIC는 우선순위 기반 선점이라 명시적으로 순서를 지정해줘야 합니다.)

## 소프트웨어 구조

```
PC (G코드 센더)
  → Serial RX
  → G코드 파서 (gc_execute_line)
  → 모션 계산 + 플래너 버퍼 (mc_line)
  → 세그먼트 준비 (st_prep_buffer, 메인 루프)
  → [세그먼트 버퍼]
  → TIM2 ISR — 스텝 펄스 생성 (최고 실시간성)
  → TIM3 ISR — 펄스 폭 리셋
```

- **메인 루프**(우선순위 없음)는 파싱/속도 계획처럼 무거운 계산을 담당합니다.
- **하드웨어 인터럽트**(TIM2/TIM3)는 미리 계산된 세그먼트를 그대로 실행만 하는, 최대한 가벼운 코드로 유지됩니다.
- 실시간 명령(`?`, `!`, `~`, 리셋 등)은 시리얼 수신 단계에서 즉시 가로채 별도 플래그로 처리됩니다.

## 빌드 방법

1. STM32CubeIDE에서 STM32F103C8Tx 기준 새 프로젝트 생성
2. 이 저장소의 파일들을 `Core/Src`, `Core/Inc`에 복사 (CubeMX가 자동 생성한 `main.c`, `stm32f1xx_it.c`는 **덮어쓰거나 비활성화** — 이 프로젝트는 클럭/GPIO/타이머/USART 초기화를 코드 레벨에서 직접 수행합니다)
3. CubeMX GUI에서 USART1 페리페럴을 별도로 생성하지 마세요 — `serial_init()`이 레지스터 레벨에서 직접 초기화합니다. GUI로 같이 생성하면 `USART1_IRQHandler`가 중복 정의되어 링크 에러가 나거나, HAL의 UART 상태머신과 충돌해 무한 인터럽트에 빠질 수 있습니다.
4. 빌드 후 ST-Link 등으로 플래싱

## 알려진 이슈 / 디버깅 노트

AVR → ARM 포팅 과정에서 실제로 만났던 문제들입니다. 비슷한 포팅을 하시는 분들께 참고가 될 것 같아 정리해둡니다.

| 증상 | 원인 | 해결 |
|---|---|---|
| `settings.c` 컴파일 에러 | `const __flash` 키워드는 AVR-GCC 전용 (하버드 구조) | STM32는 통합 메모리 구조라 그냥 `const`로 충분 |
| `st_go_idle()` 안 `delay_ms()` 호출 시 영구 멈춤 | `_delay_ms()`를 `HAL_Delay()`로 매핑했는데, 이 함수가 최고 우선순위 ISR(TIM2) 안에서 호출됨. `HAL_Delay()`는 `SysTick`(TIM2보다 우선순위 낮음)이 틱을 올려줘야 빠져나오는데, 낮은 우선순위는 높은 우선순위 ISR을 선점 못 함 → 데드락 | `_delay_ms()`를 DWT 사이클카운터 기반 busy-wait로 교체 (인터럽트에 의존하지 않음) |
| G코드 스트리밍 중 에러 없이 조용히 멈춤 | 고속 커팅 중 `USART1`(우선순위 낮음) RX 인터럽트가 `TIM2`/`TIM3`에 밀려 바이트 유실, 하필 개행문자가 유실되면 송신측·수신측이 서로를 기다리며 데드락 | USART1_RX를 DMA1 Channel5로 전환 — CPU/인터럽트 우선순위와 무관하게 하드웨어가 즉시 캡처 |
| `ok\r\n` 응답이 간헐적으로 유실 | `UCSR0B \|= (1<<UDRIE0)`이 AVR에선 원자적 단일명령(`sbi`)이지만 ARM에선 읽기-수정-쓰기 3단계라 TX ISR과 레이스 발생 | 해당 레지스터 조작을 짧은 인터럽트 마스킹으로 보호 |
| `cycles` 계산값이 이상하게 튐 | `TICKS_PER_MICROSECOND*1000000*60`가 16MHz(AVR) 기준으로는 `uint32_t` 범위 안이지만 72MHz로 올리면서 오버플로우 | 곱셈 앞에 `(float)` 캐스팅 추가 |
| STEP 펄스 폭이 설정값보다 들쭉날쭉 늘어남, 고속에서 탈조 | `TIM3`(펄스 리셋)가 `TIM2`(스텝 주기)보다 NVIC 우선순위가 낮으면, TIM2 ISR이 세그먼트를 로드하는 동안 TIM3가 선점을 못 해 STEP 핀 클리어가 지연됨 | TIM3 우선순위를 TIM2보다 높게 설정 |
| 모터 진동이 거칠고 위치가 어긋남 (펄스 파형은 정상) | 펌웨어 문제가 아니라 드라이버 전류 제한(Vref)이 모터 정격보다 높게 설정되어 과전류 | Vref를 모터 정격 전류에 맞게 재조정 |

**핵심 교훈**: AVR에서 `sei()`/`cli()`나 단일 비트 레지스터 연산으로 암묵적으로 보장되던 원자성/선점 순서가, ARM NVIC의 명시적 우선순위 기반 모델로 바뀌면서 깨지는 지점이 여러 곳 있었습니다. 포팅 시 "이 레지스터를 여러 컨텍스트(메인 루프/ISR)에서 동시에 건드리는가", "이 인터럽트가 저 인터럽트보다 반드시 먼저 끝나야 하는가"를 축별로 다시 점검할 필요가 있습니다.

## 라이선스

원본 [GRBL](https://github.com/gnea/grbl)은 GPLv3 라이선스입니다. 이 저장소도 동일하게 GPLv3를 따릅니다.

## 크레딧

- [GRBL 1.1h](https://github.com/gnea/grbl) — Sungeun K. Jeon, Sonny Jeon 및 기여자들
