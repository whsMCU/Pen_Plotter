/*
 * can.c
 *
 *  Created on: Sep 7, 2024
 *      Author: WANG
 */


#include "can.h"
#include "qbuffer.h"
#include "cli.h"


#ifdef _USE_HW_CAN

CAN_HandleTypeDef hcan;

typedef struct
{
  uint32_t prescaler;
  uint32_t sjw;
  uint32_t tseg1;
  uint32_t tseg2;
} can_baud_cfg_t;

const can_baud_cfg_t can_baud_cfg[] =
    {
        {20, CAN_SJW_1TQ, CAN_BS1_15TQ, CAN_BS2_2TQ}, // 100K, 87.5%
        {18, CAN_SJW_1TQ, CAN_BS1_13TQ, CAN_BS2_2TQ}, // 125K, 87.5%
        {9,  CAN_SJW_1TQ, CAN_BS1_13TQ, CAN_BS2_2TQ}, // 250K, 87.5%
        {4,  CAN_SJW_1TQ, CAN_BS1_15TQ, CAN_BS2_2TQ}, // 500K, 87.5%
        {2,  CAN_SJW_1TQ, CAN_BS1_15TQ, CAN_BS2_2TQ}, // 1M,   87.5%
    };

const can_baud_cfg_t *p_baud   = can_baud_cfg;


const uint32_t dlc_len_tbl[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

const uint32_t dlc_tbl[] =
    {
        CAN_DLC_BYTES_0,
        CAN_DLC_BYTES_1,
        CAN_DLC_BYTES_2,
        CAN_DLC_BYTES_3,
        CAN_DLC_BYTES_4,
        CAN_DLC_BYTES_5,
        CAN_DLC_BYTES_6,
        CAN_DLC_BYTES_7,
        CAN_DLC_BYTES_8
    };

static const uint32_t mode_tbl[] =
    {
        CAN_MODE_NORMAL,
        CAN_MODE_LOOPBACK
    };


typedef struct
{
  bool is_init;
  bool is_open;

  uint32_t err_code;
  uint8_t  state;

  uint32_t fifo_idx;
  CanMode_t  mode;
  CanBaud_t  baud;

  uint32_t rx_cnt;
  uint32_t tx_cnt;

  CAN_HandleTypeDef *hcan;
  CAN_FilterTypeDef hcan_filter;

  qbuffer_t q_msg;
  can_msg_t can_msg[CAN_MSG_RX_BUF_MAX];

  CAN_TxHeaderTypeDef   TxHeader;
  CAN_RxHeaderTypeDef   RxHeader;
  uint8_t               TxData[8];
  uint8_t               RxData[8];
  uint32_t              TxMailbox;
} can_tbl_t;

static can_tbl_t can_tbl[CAN_MAX_CH];

static volatile uint32_t err_int_cnt = 0;

#ifdef _USE_HW_RTOS
static osMutexId mutex_lock;
#else
static bool is_lock = false;
#endif

#ifdef _USE_HW_CLI
static void cliCan(cli_args_t *args);
#endif


bool canInit(void)
{
  bool ret = true;

  uint8_t i;

  for(i = 0; i < CAN_MAX_CH; i++)
  {
    can_tbl[i].is_init  = true;
    can_tbl[i].is_open  = false;
    can_tbl[i].state    = 0;

    can_tbl[i].rx_cnt = 0;
    can_tbl[i].tx_cnt = 0;

    qbufferCreateBySize(&can_tbl[i].q_msg, (uint8_t *)&can_tbl[i].can_msg[0], sizeof(can_msg_t), CAN_MSG_RX_BUF_MAX);
  }

  #ifdef _USE_HW_RTOS
  osMutexDef(mutex_lock);
  mutex_lock = osMutexCreate (osMutex(mutex_lock));
  #else
  is_lock = false;
  #endif

#ifdef _USE_HW_CLI
  cliAdd("can", cliCan);
#endif
  return ret;
}

bool canLock(void)
{
  bool ret = true;
  uint32_t timeout = 0;


#ifdef _USE_HW_RTOS
  if (timeout > 0)
    osMutexWait(mutex_lock, timeout);
  else
    osMutexWait(mutex_lock, osWaitForever);
#else
  uint32_t pre_time;

  pre_time = millis();
  while(1)
  {
    if (is_lock == false)
      break;

    if (timeout > 0 && millis()-pre_time >= timeout)
    {
      ret = false;
      break;
    }
  }
#endif

  return ret;
}

bool canUnLock(void)
{
#ifdef _USE_HW_RTOS
  osMutexRelease(mutex_lock);
#else
  is_lock = false;
#endif

  return true;
}

bool canOpen(uint8_t ch, CanMode_t mode, CanBaud_t baud)
{
  bool ret = true;
  CAN_HandleTypeDef  *p_can;

  if (ch >= CAN_MAX_CH) return false;

  can_tbl[ch].hcan = &hcan;

  p_can = can_tbl[ch].hcan;

  switch(ch)
  {
    case _DEF_CAN1:
      p_can->Instance                  = CAN1;
      p_can->Init.Mode                 = mode_tbl[mode];
      p_can->Init.TimeTriggeredMode    = DISABLE;
      p_can->Init.AutoBusOff           = DISABLE;
      p_can->Init.AutoWakeUp           = DISABLE;
      p_can->Init.ReceiveFifoLocked    = DISABLE;
      p_can->Init.TransmitFifoPriority = DISABLE;
      p_can->Init.AutoRetransmission   = DISABLE;
      p_can->Init.Prescaler            = p_baud[baud].prescaler;
      p_can->Init.SyncJumpWidth        = p_baud[baud].sjw;
      p_can->Init.TimeSeg1             = p_baud[baud].tseg1;
      p_can->Init.TimeSeg2             = p_baud[baud].tseg2;


      can_tbl[ch].hcan_filter.FilterActivation      = CAN_FILTER_ENABLE;
      can_tbl[ch].hcan_filter.FilterBank            = 0;
      can_tbl[ch].hcan_filter.FilterFIFOAssignment  = CAN_RX_FIFO0;
      can_tbl[ch].hcan_filter.FilterIdHigh          = 0x0000;
      can_tbl[ch].hcan_filter.FilterIdLow           = 0x0000;
      can_tbl[ch].hcan_filter.FilterMaskIdHigh      = 0x0000;
      can_tbl[ch].hcan_filter.FilterMaskIdLow       = 0x0000;
      can_tbl[ch].hcan_filter.FilterMode            = CAN_FILTERMODE_IDMASK;
      can_tbl[ch].hcan_filter.FilterScale           = CAN_FILTERSCALE_32BIT;
      can_tbl[ch].hcan_filter.SlaveStartFilterBank  = 14;

      can_tbl[ch].mode                = mode;
      can_tbl[ch].baud                = baud;
      can_tbl[ch].fifo_idx            = CAN_RX_FIFO0;

      ret = true;
      break;

    default:
      ret = false;
      break;
  }

  if (ret != true)
  {
    return false;
  }

  if (HAL_CAN_Init(p_can) != HAL_OK)
  {
    return false;
  }


  if (HAL_CAN_ConfigFilter(p_can, &can_tbl[ch].hcan_filter) != HAL_OK)
  {
    return false;
  }

  if (HAL_CAN_Start(p_can) != HAL_OK)
  {
    return false;
  }

  if (HAL_CAN_ActivateNotification(p_can, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    return false;
  }

  can_tbl[ch].is_open = true;


  return ret;
}

bool canIsOpen(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return false;

  return can_tbl[ch].is_open;
}

void canClose(uint8_t ch)
{

}

bool canGetInfo(uint8_t ch, can_info_t *p_info)
{
  if(ch >= CAN_MAX_CH) return false;

  p_info->baud = can_tbl[ch].baud;
  p_info->mode = can_tbl[ch].mode;

  return true;
}

CanDlc_t canGetDlc(uint8_t length)
{
  CanDlc_t ret;

  if (length >= 8)
    ret = CAN_DLC_8;
  else
    ret = (CanDlc_t)length;

  return ret;
}

uint8_t canGetLen(CanDlc_t dlc)
{
  return dlc_len_tbl[(int)dlc];
}

uint32_t canMsgAvailable(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return 0;

  return qbufferAvailable(&can_tbl[ch].q_msg);
}

bool canMsgInit(can_msg_t *p_msg, CanIdType_t id_type, CanDlc_t dlc)
{
  p_msg->id_type = id_type;
  p_msg->dlc     = dlc;
  p_msg->length  = dlc_len_tbl[dlc];
  return true;
}

bool canMsgWrite(uint8_t ch, can_msg_t *p_msg)
{
  CAN_HandleTypeDef  *p_can;
  CAN_TxHeaderTypeDef tx_header;
  bool ret = true;


  if(ch > CAN_MAX_CH) return false;

  if (can_tbl[ch].is_open != true) return false;

  p_can = can_tbl[ch].hcan;

  switch(p_msg->id_type)
  {
    case CAN_STD :
      tx_header.IDE = CAN_ID_STD;
      break;

    case CAN_EXT :
      tx_header.IDE = CAN_ID_EXT;
      break;
  }
  tx_header.StdId              = p_msg->id;
  tx_header.ExtId              = 0x0;
  tx_header.RTR                = CAN_RTR_DATA;
  tx_header.DLC                = dlc_tbl[p_msg->dlc];
  tx_header.TransmitGlobalTime = DISABLE;

  can_tbl[ch].TxData[0] = 0xCA;
  can_tbl[ch].TxData[1] = 0xFE;

  if (HAL_CAN_AddTxMessage(p_can, &tx_header, can_tbl[ch].TxData, &can_tbl[ch].TxMailbox) != HAL_OK)
  {
    /* Transmission request Error */
    return false;
  }

  if (ret == true)
  {
    can_tbl[ch].tx_cnt++;
  }
  return ret;
}

bool canMsgRead(uint8_t ch, can_msg_t *p_msg)
{
  bool ret = true;


  if(ch >= CAN_MAX_CH) return 0;

  ret = qbufferRead(&can_tbl[ch].q_msg, (uint8_t *)p_msg, 1);

  return ret;
}



uint32_t canGetError(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return 0;

  return can_tbl[ch].err_code;
}

uint32_t canGetRxCount(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return 0;

  return can_tbl[ch].rx_cnt;
}

uint32_t canGetTxCount(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return 0;

  return can_tbl[ch].tx_cnt;
}

uint32_t canGetState(uint8_t ch)
{
  if(ch >= CAN_MAX_CH) return 0;

  return HAL_CAN_GetState(can_tbl[ch].hcan);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  can_msg_t *rx_buf;
  CAN_RxHeaderTypeDef rx_header;


  rx_buf  = (can_msg_t *)qbufferPeekWrite(&can_tbl[0].q_msg);

  if (HAL_CAN_GetRxMessage(hcan, can_tbl[0].fifo_idx, &rx_header, rx_buf->data) == HAL_OK)
  {
    if(rx_header.IDE == CAN_ID_STD)
    {
      rx_buf->id      = rx_header.StdId;
      rx_buf->id_type = CAN_STD;
    }
    else
    {
      rx_buf->id      = rx_header.ExtId;
      rx_buf->id_type = CAN_EXT;
    }
    rx_buf->length = rx_header.DLC;
    rx_buf->dlc = canGetDlc(rx_buf->length);

    can_tbl[0].rx_cnt++;

    qbufferWrite(&can_tbl[0].q_msg, NULL, 1);

    qbufferRead(&can_tbl[0].q_msg, NULL, 1);
  }
}


void canErrPrint(uint8_t ch)
{
  uint32_t err_code;


  if(ch >= CAN_MAX_CH) return;

  err_code = can_tbl[ch].err_code;

//  if (err_code & CAN_ERR_PASSIVE) logPrintf("  ERR : CAN_ERR_PASSIVE\n");
//  if (err_code & CAN_ERR_WARNING) logPrintf("  ERR : CAN_ERR_WARNING\n");
//  if (err_code & CAN_ERR_BUS_OFF) logPrintf("  ERR : CAN_ERR_BUS_OFF\n");
}

void canInfoPrint(uint8_t ch)
{
  can_tbl_t *p_can = &can_tbl[ch];

  cliPrintf("ch            : ");
  switch(ch)
  {
    case _DEF_CAN1:
      cliPrintf("_DEF_CAN1\r\n");
      break;
    case _DEF_CAN2:
      cliPrintf("_DEF_CAN2\r\n");
      break;
  }

  cliPrintf("is_open       : ");
  if (p_can->is_open)
    cliPrintf("true\r\n");
  else
    cliPrintf("false\r\n");

  cliPrintf("baud          : ");
  switch(p_can->baud)
  {
    case CAN_100K:
      cliPrintf("100K\r\n");
      break;
    case CAN_125K:
      cliPrintf("125K\r\n");
      break;
    case CAN_250K:
      cliPrintf("250\r\n");
      break;
    case CAN_500K:
      cliPrintf("250\r\n");
      break;
    case CAN_1M:
      cliPrintf("1M\r\n");
      break;

    default:
      break;
  }

  cliPrintf("mode          : ");
  switch(p_can->mode)
  {
    case CAN_NORMAL:
      cliPrintf("NORMAL\r\n");
      break;
    case CAN_LOOPBACK:
      cliPrintf("LOOPBACK\r\n");
      break;
  }
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN GPIO Configuration
    PB8     ------> CAN_RX
    PB9     ------> CAN_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_AFIO_REMAP_CAN1_2();

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN GPIO Configuration
    PB8     ------> CAN_RX
    PB9     ------> CAN_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);

    /* CAN1 interrupt Deinit */
  /* USER CODE BEGIN CAN1:USB_LP_CAN1_RX0_IRQn disable */
    /**
    * Uncomment the line below to disable the "USB_LP_CAN1_RX0_IRQn" interrupt
    * Be aware, disabling shared interrupt may affect other IPs
    */
    /* HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn); */
  /* USER CODE END CAN1:USB_LP_CAN1_RX0_IRQn disable */

    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}


#ifdef _USE_HW_CLI
void cliCan(cli_args_t *args)
{
  bool ret = false;


  canLock();

  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<CAN_MAX_CH; i++)
    {
      canInfoPrint(i);

      canErrPrint(i);

      cliPrintf("\n");
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "read"))
  {
    uint32_t index = 0;
    uint8_t ch;

    ch = constrain(args->getData(1), 0, CAN_MAX_CH - 1);

    while(cliKeepLoop())
    {
      if (canMsgAvailable(ch))
      {
        can_msg_t msg;

        canMsgRead(ch, &msg);

        index %= 1000;
        cliPrintf("ch %d %03d(R) <- id ", ch, index++);
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "send"))
  {
    uint32_t pre_time;
    uint32_t index = 0;
    uint32_t err_code;
    uint8_t ch;

    ch = constrain(args->getData(1), 0, CAN_MAX_CH - 1);


    err_code = can_tbl[ch].err_code;

    while(cliKeepLoop())
    {
      can_msg_t msg;

      if (millis()-pre_time >= 1000)
      {
        pre_time = millis();

        msg.id_type = CAN_EXT;
        msg.dlc     = CAN_DLC_2;
        msg.id      = 0x314;
        msg.length  = 2;
        msg.data[0] = 1;
        msg.data[1] = 2;
        if (canMsgWrite(_DEF_CAN1, &msg) > 0)
        {
          index %= 1000;
          cliPrintf("ch %d %03d(T) -> id ", ch, index++);
          if (msg.id_type == CAN_STD)
          {
            cliPrintf("std ");
          }
          else
          {
            cliPrintf("ext ");
          }
          cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
          for (int i=0; i<msg.length; i++)
          {
            cliPrintf("0x%02X ", msg.data[i]);
          }
          cliPrintf("\n");
        }

//        if (canGetRxErrCount(ch) > 0 || canGetTxErrCount(ch) > 0)
//        {
//          cliPrintf("ch %d ErrCnt : %d, %d\n", ch, canGetRxErrCount(_DEF_CAN1), canGetTxErrCount(_DEF_CAN1));
//        }
//
//        if (err_int_cnt > 0)
//        {
//          cliPrintf("ch %d Cnt : %d\n", ch, err_int_cnt);
//          err_int_cnt = 0;
//        }
      }

      if (can_tbl[ch].err_code != err_code)
      {
        cliPrintf("ch %d ErrCode : 0x%X\n", ch, can_tbl[ch].err_code);
        canErrPrint(ch);
        err_code = can_tbl[ch].err_code;
      }

//      if (canUpdate())
//      {
//        cliPrintf("ch %d BusOff Recovery\n", ch);
//      }


      if (canMsgAvailable(ch))
      {
        canMsgRead(ch, &msg);

        index %= 1000;
        cliPrintf("ch %d %03d(R) <- id ", ch, index++);
        if (msg.id_type == CAN_STD)
        {
          cliPrintf("std ");
        }
        else
        {
          cliPrintf("ext ");
        }
        cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
        for (int i=0; i<msg.length; i++)
        {
          cliPrintf("0x%02X ", msg.data[i]);
        }
        cliPrintf("\n");
      }
    }
    ret = true;
  }

  canUnLock();

  if (ret == false)
  {
    cliPrintf("can info\n");
    cliPrintf("can read ch[0~%d]\n", CAN_MAX_CH-1);
    cliPrintf("can send ch[0~%d]\n", CAN_MAX_CH-1);
  }
}
#endif

#endif
