/*
  eeprom.c - STM32F103 Flash 에뮬레이션 기반 EEPROM 대체
  Part of Grbl (STM32 포팅)

  원본 AVR eeprom.c는 하드웨어 EEPROM 컨트롤러(EECR/EEAR/EEDR)를 직접
  제어했으나, F103은 별도 EEPROM이 없으므로 마지막 플래시 페이지(1KB)를
  RAM에 캐시해두고 통째로 지우고 다시 쓰는 방식으로 에뮬레이션합니다.
  settings.c 등 상위 코드가 요구하는 함수 시그니처는 원본과 동일합니다.
*/
#include "grbl.h"

#define EEPROM_PAGE_ADDR   0x0800FC00UL   /* STM32F103C8 64KB 플래시의 마지막 1KB 페이지 */
#define EEPROM_PAGE_SIZE   1024U

static uint8_t ram_cache[EEPROM_PAGE_SIZE];
static uint8_t cache_loaded = 0;

static void eeprom_load_cache(void)
{
  if (!cache_loaded) {
    memcpy(ram_cache, (void *)EEPROM_PAGE_ADDR, EEPROM_PAGE_SIZE);
    cache_loaded = 1;
  }
}

static void eeprom_flush_cache(void)
{
  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef er = {0};
  er.TypeErase = FLASH_TYPEERASE_PAGES;
  er.PageAddress = EEPROM_PAGE_ADDR;
  er.NbPages = 1;
  uint32_t page_error;
  HAL_FLASHEx_Erase(&er, &page_error);

  for (uint32_t i = 0; i < EEPROM_PAGE_SIZE; i += 2) {
    uint16_t half = (uint16_t)(ram_cache[i] | (ram_cache[i+1] << 8));
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, EEPROM_PAGE_ADDR + i, half);
  }

  HAL_FLASH_Lock();
}

unsigned char eeprom_get_char(unsigned int addr)
{
  eeprom_load_cache();
  if (addr >= EEPROM_PAGE_SIZE) { return 0xFF; }
  return ram_cache[addr];
}

void eeprom_put_char(unsigned int addr, unsigned char new_value)
{
  eeprom_load_cache();
  if (addr >= EEPROM_PAGE_SIZE) { return; }
  if (ram_cache[addr] != new_value) {
    ram_cache[addr] = new_value;
    eeprom_flush_cache(); // 설정 저장은 빈도가 낮아 매 바이트 즉시 플러시해도 무방
  }
}

void memcpy_to_eeprom_with_checksum(unsigned int destination, char *source, unsigned int size)
{
  eeprom_load_cache();
  unsigned char checksum = 0;
  for (; size > 0; size--) {
    checksum = (checksum << 1) | (checksum >> 7);
    checksum += *source;
    if (destination < EEPROM_PAGE_SIZE) { ram_cache[destination] = *source; }
    destination++;
    source++;
  }
  if (destination < EEPROM_PAGE_SIZE) { ram_cache[destination] = checksum; }
  eeprom_flush_cache();
}

int memcpy_from_eeprom_with_checksum(char *destination, unsigned int source, unsigned int size)
{
  eeprom_load_cache();
  unsigned char data, checksum = 0;
  for (; size > 0; size--) {
    data = eeprom_get_char(source++);
    checksum = (checksum << 1) | (checksum >> 7);
    checksum += data;
    *(destination++) = (char)data;
  }
  return (checksum == eeprom_get_char(source));
}
