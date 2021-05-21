/**
  ******************************************************************************
  * @file    main.h
  * @author  MMY Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2018 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
//#include "nucleo_l476rg_bus.h"


/** @defgroup X-CUBE-NFC3_Applications
 *  @brief Sample applications for X-NUCLEO-NFC03A1 STM32 expansion boards.
 *  @{
 */

/** @defgroup PollingTagDetect
 *  @brief This demo shows how to poll for several types of NFC cards/devices and how 
 *  to exchange data with these devices, using the RFAL library.
 *  @{
 */

/** @defgroup PTD_Main
 *  @brief Main application program
 * @{
 */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup PTD_Main_Exported_Constants
 *  @{
 */
#define LED_FIELD_Pin 1         /*!< Enable usage of led field pin on the platform      */
#define LED_FIELD_GPIO_Port 1   /*!< Enable usage of led field port on the platform     */

#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_10
#define LED2_GPIO_Port GPIOB
#define SSI_0_Pin GPIO_PIN_7
#define SSI_0_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_8
#define LED1_GPIO_Port GPIOA
#define nIRQ_IN_Pin GPIO_PIN_9
#define nIRQ_IN_GPIO_Port GPIOA
#define nIRQ_OUT_Pin GPIO_PIN_10
#define nIRQ_OUT_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_4
#define LED3_GPIO_Port GPIOB
#define LED4_Pin GPIO_PIN_5
#define LED4_GPIO_Port GPIOB
#define nSPI_SS_Pin GPIO_PIN_6
#define nSPI_SS_GPIO_Port GPIOB

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
/* #define USE_FULL_ASSERT    1U */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup PTD_Main_Exported_Constants
 *  @{
 */
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

/**
  * @}
  */

/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/** @defgroup PTD_Main_Exported_Functions
 *  @{
 */
   
void _Error_Handler(char * file, int line);

/**
  * @}
  */

/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
