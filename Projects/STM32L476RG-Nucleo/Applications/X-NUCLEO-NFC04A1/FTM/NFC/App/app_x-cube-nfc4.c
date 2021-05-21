/**
  ******************************************************************************
  * File Name          :  stmicroelectronics_x-cube-nfc4_1_3_0.c
  * Description        : This file provides code for the configuration
  *                      of the STMicroelectronics.X-CUBE-NFC4.1.3.0 instances.
  ******************************************************************************
  *
  * COPYRIGHT 2019 STMicroelectronics
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  ******************************************************************************
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "main.h"

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "app_x-cube-nfc4.h"
#include "main.h"
#include "common.h"
#include "st25ftm_protocol.h"

/** @defgroup ST25_Nucleo
  * @{
  */

/** @defgroup Main
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t cmdBuffer[8192];
uint32_t cmdLength;
/* Global variables ----------------------------------------------------------*/


/* Private functions ---------------------------------------------------------*/
void MX_NFC4_FTM_Init(void);
void MX_NFC4_FTM_Process(void);

void MX_NFC_Init(void)
{
  /* USER CODE BEGIN SV */ 

  /* USER CODE END SV */

  /* USER CODE BEGIN NFC4_Library_Init_PreTreatment */
  
  /* USER CODE END NFC4_Library_Init_PreTreatment */

  /* Initialize the peripherals and the NFC4 components */
  MX_NFC4_FTM_Init();

  /* USER CODE BEGIN SV */ 

  /* USER CODE END SV */
  
  /* USER CODE BEGIN NFC4_Library_Init_PostTreatment */
  
  /* USER CODE END NFC4_Library_Init_PostTreatment */
}
/*
 * LM background task
 */
void MX_NFC_Process(void)
{
  /* USER CODE BEGIN NFC4_Library_Process */
  MX_NFC4_FTM_Process();

  /* USER CODE END NFC4_Library_Process */
}

void MX_NFC4_FTM_Init(void)
{
	  /******************************************************************************/
  /* Configuration of X-NUCLEO-NFC02A1                                          */
  /******************************************************************************/
  /* Init of the Leds on X-NUCLEO-NFC04A1 board */
  NFC04A1_LED_Init(GREEN_LED );
  NFC04A1_LED_Init(BLUE_LED );
  NFC04A1_LED_Init(YELLOW_LED );
  NFC04A1_LED_On( GREEN_LED );
  HAL_Delay( 300 );
  NFC04A1_LED_On( BLUE_LED );
  HAL_Delay( 300 );
  NFC04A1_LED_On( YELLOW_LED );
  HAL_Delay( 300 );
  
  /* Init ST25DV driver */
  while( NFC04A1_NFCTAG_Init(NFC04A1_NFCTAG_INSTANCE) != NFCTAG_OK );

  /* Enable FTM */
  ST25FTM_Init();
  ST25FTM_SetRxFrameMaxLength(256);
  ST25FTM_SetTxFrameMaxLength(256);
  cmdLength = sizeof(cmdBuffer);
  ST25FTM_ReceiveCommand(cmdBuffer,&cmdLength);

  /* Init done */
  NFC04A1_LED_Off( GREEN_LED );
  HAL_Delay( 300 );
  NFC04A1_LED_Off( BLUE_LED );
  HAL_Delay( 300 );
  NFC04A1_LED_Off( YELLOW_LED );
  HAL_Delay( 300 );

}
/**
  * @brief  Process of the FTM application
  * @retval None
  */
void MX_NFC4_FTM_Process(void)
{
  static uint32_t time;
  char txt[64];

  ST25FTM_Runner();
  if(ST25FTM_IsNewFrame())
  {
    /* A new command has started */
    NFC04A1_LED_On( BLUE_LED );
    UARTConsolePrint("\r\nReception started\r\n");
    time = HAL_GetTick();
  }
  if(ST25FTM_IsReceptionComplete())
  {
    /* A command has been fully received */
    NFC04A1_LED_Off( BLUE_LED );
    NFC04A1_LED_On( GREEN_LED );
    time = (int)HAL_GetTick() - (int)time;
    sprintf(txt,"Reception completed: %lu bytes in %lu ms\r\n",cmdLength,time);
    UARTConsolePrint(txt);
    UARTConsolePrint("Sending data back\r\n");
    time = HAL_GetTick();
    /* send it back */
    ST25FTM_SendCommand(cmdBuffer,cmdLength,ST25FTM_SEND_WITH_ACK);
  }
  if(ST25FTM_IsTransmissionComplete() || ST25FTM_IsIdle())
  {
    /* Clear respond LED status */
    NFC04A1_LED_Off( GREEN_LED );
    /* the response has been done */
    time = (int)HAL_GetTick() - (int)time;
    sprintf(txt,"Transmission completed: %lu bytes in %lu ms\r\n",cmdLength, time);
    UARTConsolePrint(txt);
    cmdLength = sizeof(cmdBuffer);
    ST25FTM_ReceiveCommand(cmdBuffer,&cmdLength);
  }
  if(ST25FTM_CheckError())
  {
    /* an Error occured */
    UARTConsolePrint("An error occured, FTM is reset\r\n");
    NFC04A1_LED_On( YELLOW_LED );
    ST25FTM_Reset();
  }
}

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
