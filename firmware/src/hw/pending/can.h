/*
 * can.h
 *
 *  Created on: Sep 7, 2024
 *      Author: WANG
 */

#ifndef SRC_HW_CAN_H_
#define SRC_HW_CAN_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw.h"


#ifdef _USE_HW_CAN


#define CAN_MAX_CH            HW_CAN_MAX_CH
#define CAN_MSG_RX_BUF_MAX    HW_CAN_MSG_RX_BUF_MAX


typedef enum
{
  CAN_100K,
  CAN_125K,
  CAN_250K,
  CAN_500K,
  CAN_1M
} CanBaud_t;

typedef enum
{
  CAN_NORMAL,
  CAN_LOOPBACK
} CanMode_t;

typedef enum
{
  CAN_STD,
  CAN_EXT
} CanIdType_t;

typedef enum
{
  CAN_DLC_0,
  CAN_DLC_1,
  CAN_DLC_2,
  CAN_DLC_3,
  CAN_DLC_4,
  CAN_DLC_5,
  CAN_DLC_6,
  CAN_DLC_7,
  CAN_DLC_8,
} CanDlc_t;


typedef struct
{
  CanMode_t  mode;
  CanBaud_t  baud;
} can_info_t;
typedef struct
{
  uint32_t id;
  uint16_t length;
  uint8_t  data[8];

  CanDlc_t      dlc;
  CanIdType_t   id_type;
} can_msg_t;


bool     canInit(void);
bool     canOpen(uint8_t ch, CanMode_t mode, CanBaud_t baud);
bool     canIsOpen(uint8_t ch);
void     canClose(uint8_t ch);
bool     canGetInfo(uint8_t ch, can_info_t *p_info);

CanDlc_t canGetDlc(uint8_t length);
uint8_t  canGetLen(CanDlc_t dlc);

bool     canMsgInit(can_msg_t *p_msg, CanIdType_t  id_type, CanDlc_t dlc);
uint32_t canMsgAvailable(uint8_t ch);
bool     canMsgWrite(uint8_t ch, can_msg_t *p_msg);
bool     canMsgRead(uint8_t ch, can_msg_t *p_msg);

bool     canLock(void);
bool     canUnLock(void);

#endif

#ifdef __cplusplus
 }
#endif

#endif /* SRC_HW_CAN_H_ */
