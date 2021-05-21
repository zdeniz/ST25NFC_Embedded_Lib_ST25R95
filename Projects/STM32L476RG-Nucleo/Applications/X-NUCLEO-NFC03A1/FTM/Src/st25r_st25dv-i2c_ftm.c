/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2018 STMicroelectronics, all rights reserved
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/

#include <string.h>

#include "st25ftm_common.h"
#include "st25ftm_config.h"
#include "st_errno.h"
#include "stm32l4xx_hal.h"

#define ST25FTM_NUMBER_OF_ATTEMPTS (40)

/* ST25DV-I2C register definition */
#define ST25DV_I2C_DYN_REG_EH_CTRL_ADDR (0x2)
#define ST25DV_I2C_DYN_REG_MB_CTRL_ADDR (0xD)
#define ST25DV_I2C_DYN_REG_MB_CTRL_HOST_PUT_MSG (0x2)
#define ST25DV_I2C_DYN_REG_MB_CTRL_RF_PUT_MSG (0x4)
static rfalNfcvListenDevice *currentDevice = NULL;
static CRC_HandleTypeDef hcrc;

void ST25FTM_SetDevice(rfalNfcvListenDevice *device)
{
  currentDevice = device;
}


ST25FTM_MessageStatus_t ST25FTM_ReadMessage(uint8_t *msg, uint32_t* msg_len)
{
  *msg_len = 0;
  uint16_t err;
  uint16_t rcvLen = 0;
  static uint8_t error_count = 0;

  if(currentDevice == NULL)
  {
    return ST25FTM_MSG_ERROR;
  }

  /* read the whole mailbox */
  err = rfalST25xVPollerReadMessage( RFAL_NFCV_REQ_FLAG_DEFAULT, NULL, 0, 0, msg, ST25FTM_BUFFER_LENGTH, &rcvLen );
  if(err == ERR_NONE)
  {
    /* remove status byte */
    *msg_len = rcvLen - 1;
    memmove(msg,&msg[1],*msg_len);
    error_count = 0;
    return ST25FTM_MSG_OK;
  }
  ST25FTM_LOG("FTM_ReadMsg: %d\r\n",err);

  if(error_count > ST25FTM_NUMBER_OF_ATTEMPTS)
  {
    /* too many consecutive errors received, give up */
    ST25FTM_Reset();
    error_count = 0;
  } else {
    error_count++;
    /* try to select the tag again, in case it is back in the field */
    /* return value is ignored as the current function is going to return an error anyway */
    rfalNfcvPollerSelect(RFAL_NFCV_REQ_FLAG_DEFAULT, currentDevice->InvRes.UID );
  }
  /* an error occurs, pretend there is no data read */
  return ST25FTM_MSG_ERROR;
}

ST25FTM_MessageStatus_t ST25FTM_WriteMessage(uint8_t* msg, uint32_t msg_len)
{
  uint8_t txBuf[300];
  uint16_t err;
  static uint8_t error_count = 0;

  if(currentDevice == NULL)
  {
    return ST25FTM_MSG_ERROR;
  }

  err = rfalST25xVPollerWriteMessage( RFAL_NFCV_REQ_FLAG_DEFAULT, NULL, msg_len - 1, msg, txBuf, sizeof(txBuf) );
  if(err == ERR_NONE)
  {
    error_count = 0;
    return ST25FTM_MSG_OK;
  } else if (err == ERR_PROTO) {
    ST25FTM_LOG("FTM_WriteMsg: %d\r\n",err);
    error_count = 0;
    return ST25FTM_MSG_BUSY;
  } else {
    ST25FTM_LOG("FTM_WriteMsg: %d\r\n",err);
    if(error_count > ST25FTM_NUMBER_OF_ATTEMPTS)
    {
      /* too many consecutive errors received, give up */
      ST25FTM_Reset();
      error_count = 0;
    } else {
      error_count++;
      /* try to select the tag again, in case it is back in the field */
      /* return value is ignored as the current function is going to return an error anyway */
      rfalNfcvPollerSelect(RFAL_NFCV_REQ_FLAG_DEFAULT, currentDevice->InvRes.UID );
    }
    return ST25FTM_MSG_ERROR;
  }
}

ST25FTM_MessageOwner_t ST25FTM_GetMessageOwner(void)
{
  uint8_t mbStatus = 0;
  uint16_t err;
  static uint8_t error_count = 0;

  if(currentDevice == NULL)
  {
    return ST25FTM_MESSAGE_OWNER_ERROR;
  }

  err = rfalST25xVPollerReadDynamicConfiguration( RFAL_NFCV_REQ_FLAG_DEFAULT, NULL, 0x0D, &mbStatus );
  if(err == ERR_NONE )
  {
    error_count = 0;
    if(mbStatus & ST25DV_I2C_DYN_REG_MB_CTRL_HOST_PUT_MSG)
    {
      return ST25FTM_MESSAGE_PEER;
    } else if (mbStatus & ST25DV_I2C_DYN_REG_MB_CTRL_RF_PUT_MSG) {
      return ST25FTM_MESSAGE_ME;
    } else {
      return ST25FTM_MESSAGE_EMPTY;
    }
  }
  ST25FTM_LOG("FTM_MailboxBusy: %d\r\n",err);
  if(error_count > ST25FTM_NUMBER_OF_ATTEMPTS)
  {
    /* too many consecutive errors received, give up */
    ST25FTM_Reset();
    error_count = 0;
  } else {
    error_count++;
    /* try to select the tag again, in case it is back in the field */
    /* return value is ignored as the current function is going to return an error anyway */
    rfalNfcvPollerSelect(RFAL_NFCV_REQ_FLAG_DEFAULT, currentDevice->InvRes.UID );
  }
  return ST25FTM_MESSAGE_OWNER_ERROR;
}

int ST25FTM_DeviceInit(void)
{
  uint8_t ret;
  uint8_t pwd[] = {0,0,0,0,0,0,0,0};

  if(currentDevice == NULL)
  {
    return 1;
  }

  /* Present password to change ST25DV-I2C configuration */
  ret = rfalST25xVPollerPresentPassword( RFAL_NFCV_REQ_FLAG_DEFAULT, NULL, 0,pwd,  sizeof(pwd));
  if(ret != ERR_NONE)
  {
      return 1;
  }

  /* Disable Energy Harvesting */
  ret = rfalST25xVPollerWriteDynamicConfiguration( RFAL_NFCV_REQ_FLAG_DEFAULT, NULL, ST25DV_I2C_DYN_REG_EH_CTRL_ADDR, 0 );
  if(ret != ERR_NONE)
  {
      return 1;
  }

  /* Initialize Mailbox for FTM */
  ret = rfalST25xVPollerWriteDynamicConfiguration( RFAL_NFCV_REQ_FLAG_DEFAULT, NULL, ST25DV_I2C_DYN_REG_MB_CTRL_ADDR, 0 );
  if(ret != ERR_NONE)
  {
      return 1;
  }
  ret = rfalST25xVPollerWriteDynamicConfiguration( RFAL_NFCV_REQ_FLAG_DEFAULT, NULL, ST25DV_I2C_DYN_REG_MB_CTRL_ADDR, 1 );
  if(ret != ERR_NONE)
  {
      return 1;
  }
  return 0;
}

void ST25FTM_UpdateFieldStatus(void)
{
  /* This function is only relevant for dynamic tag */
  gFtmState.rfField = ST25FTM_FIELD_ON;

}

/* CRC services */
void ST25FTM_CRC_Initialize(void)
{
  __HAL_RCC_CRC_CLK_ENABLE();
  hcrc.Instance = CRC;
  __HAL_CRC_RESET_HANDLE_STATE(&hcrc);

  hcrc.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_DISABLE;
  hcrc.Init.GeneratingPolynomial    = 0x04C11DB7;
  hcrc.Init.CRCLength               = CRC_POLYLENGTH_32B;
  hcrc.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat              = CRC_INPUTDATA_FORMAT_WORDS;
  (void)HAL_CRC_Init( &hcrc );
}

uint32_t ST25FTM_GetCrc(uint8_t *data, uint32_t length)
{
  uint32_t crc;
  uint32_t nbWords = length / 4U;
  uint8_t extra_bytes = (uint8_t)(length%4U);

  /* The uint32_t* cast is ok as buffer length is managed above */
  crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)data,nbWords);
  if(extra_bytes != 0U)
  {
    uint32_t lastWord = 0;

    (void)memcpy(&lastWord,&data[nbWords*4U],extra_bytes);
    crc = HAL_CRC_Accumulate(&hcrc, &lastWord,1);
  }
  return crc;

}

