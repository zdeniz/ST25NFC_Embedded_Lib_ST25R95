#include <stdlib.h>
#include <string.h>

#include "st25ftm_protocol.h"
#include "st25ftm_config.h"
#include "bigecho.h"
#include "ftm_demo.h"

#define ST25_PRODUCT_CODE_UID_INDEX (5)

static void sendReceive(uint8_t *cmd, uint32_t cmdLen, uint8_t *rsp, uint32_t* rspLen)
{
  uint32_t time;

  platformLog("Starting FTM transmission (%d bytes)\r\n", cmdLen);
  time = HAL_GetTick();

  ST25FTM_SendCommand(cmd, cmdLen, ST25FTM_SEND_WITH_ACK);

  while(!ST25FTM_IsTransmissionComplete() && !ST25FTM_IsIdle())
  {
    ST25FTM_Runner();
    HAL_Delay(5);
  }

  if(ST25FTM_IsTransmissionComplete())
  {
    time = (int)HAL_GetTick() - (int)time;
    platformLog("FTM transmission completed in %d ms\r\n", time);
  } else {
    platformLog("FTM transmission failed, aborting\r\n");
    return;
  }

  ST25FTM_ReceiveCommand(rsp,rspLen);
  platformLog("Ready for FTM reception\r\n");
  time = HAL_GetTick();

  while(!ST25FTM_IsReceptionComplete() && !ST25FTM_IsIdle())
  {
    ST25FTM_Runner();
    HAL_Delay(5);
  }

  if(ST25FTM_IsReceptionComplete())
  {
    time = (int)HAL_GetTick() - (int)time;
    platformLog("FTM reception completed (%d bytes) in %d ms\r\n", *rspLen,time);
  } else {
    platformLog("FTM reception failed\r\n");
    *rspLen = 0;
  }
}

int ftm_demo(rfalNfcvListenDevice *nfcvDev)
{
  uint8_t rsp[8192];
  uint32_t send_len = sizeof(echo_cmd);
  uint32_t buf_len = sizeof(rsp);

  if((nfcvDev->InvRes.UID[ST25_PRODUCT_CODE_UID_INDEX] < 0x24)
     || (nfcvDev->InvRes.UID[ST25_PRODUCT_CODE_UID_INDEX] > 0x27))
  {
    /* This is not a ST25DV-I2C: return */
    return 1;
  }

  ST25FTM_Init();
  /* The ST25R95 Tx buffer is 256 byte long, including
     - Request flag (1 byte)
     - Command ID (1 byte)
     - Manufacturer ID (1 byte)
     - CRC (2 bytes)
     -> Limit transceive data to 251 byte */
  ST25FTM_SetRxFrameMaxLength(251);
  ST25FTM_SetTxFrameMaxLength(251);

  ST25FTM_SetDevice(nfcvDev);
  if(ST25FTM_DeviceInit() != 0)
  {
    ST25FTM_SetDevice(NULL);
    platformLog("Initialization error\r\n\r\n");
    return 1;
  }

  sendReceive(echo_cmd,send_len,rsp, &buf_len);

  /* Check received length against sent length */
  if(buf_len != send_len)
  {
    platformLog("FTM demo terminated with error\r\n\r\n");
    ST25FTM_SetDevice(NULL);
    return 1;
  }

  /* Check received buffer against sent buffer */
  if(memcmp(echo_cmd,rsp,send_len))
  {
    platformLog("FTM demo terminated with error\r\n\r\n");
    ST25FTM_SetDevice(NULL);
    return 1;
  }
  platformLog("FTM demo successfully completed\r\n\r\n");
  ST25FTM_SetDevice(NULL);
  return 0;
}
