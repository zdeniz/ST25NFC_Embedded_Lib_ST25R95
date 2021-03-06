/**
  ******************************************************************************
  * @file  : nucleo_l476rg.h
  * @brief : header file for the BSP Common driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
*/
 
				
				
				
				
 
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NUCLEO_L476RG_H
#define __NUCLEO_L476RG_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/ 
#include "nucleo_l476rg_conf.h"
#include "nucleo_l476rg_errno.h"
#include "main.h"

#if (USE_BSP_COM_FEATURE > 0)
  #if (USE_COM_LOG > 0)
    #ifndef __GNUC__
      #include <stdio.h>
    #endif
  #endif
#endif
/** @addtogroup BSP
 * @{
 */

/** @addtogroup NUCLEO_L476RG
 * @{
 */

/** @addtogroup NUCLEO_L476RG_LOW_LEVEL
 * @{
 */ 

/** @defgroup NUCLEO_L476RG_LOW_LEVEL_Exported_Types NUCLEO_L476RG LOW LEVEL Exported Types
 * @{
 */
 
 /** 
  * @brief Define for NUCLEO_L476RG board  
  */ 
#if !defined (USE_NUCLEO_L476RG)
 #define USE_NUCLEO_L476RG
#endif
#ifndef USE_BSP_COM_FEATURE
   #define USE_BSP_COM_FEATURE                  0U
#endif

#ifndef USE_BSP_COM
  #define USE_BSP_COM                           0U
#endif
 
#ifndef USE_COM_LOG
  #define USE_COM_LOG                           1U
#endif
  
#ifndef BSP_BUTTON_KEY_IT_PRIORITY
  #define BSP_BUTTON_KEY_IT_PRIORITY            15U
#endif 
  
typedef enum 
{
  LED2 = 0
} Led_TypeDef;

typedef enum 
{
  BUTTON_KEY = 0U
} Button_TypeDef;

typedef enum 
{  
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

#if (USE_BSP_COM_FEATURE > 0)
typedef enum 
{
  COM1 = 0U,
}COM_TypeDef;

typedef enum
{          
 COM_STOPBITS_1     =   UART_STOPBITS_1,                                 
 COM_STOPBITS_2     =   UART_STOPBITS_2,
}COM_StopBitsTypeDef;

typedef enum
{
 COM_PARITY_NONE     =  UART_PARITY_NONE,                  
 COM_PARITY_EVEN     =  UART_PARITY_EVEN,                  
 COM_PARITY_ODD      =  UART_PARITY_ODD,                   
}COM_ParityTypeDef;

typedef enum
{
 COM_HWCONTROL_NONE    =  UART_HWCONTROL_NONE,               
 COM_HWCONTROL_RTS     =  UART_HWCONTROL_RTS,                
 COM_HWCONTROL_CTS     =  UART_HWCONTROL_CTS,                
 COM_HWCONTROL_RTS_CTS =  UART_HWCONTROL_RTS_CTS, 
}COM_HwFlowCtlTypeDef;

typedef struct
{
  uint32_t             BaudRate;      
  uint32_t             WordLength;    
  COM_StopBitsTypeDef  StopBits;      
  COM_ParityTypeDef    Parity;               
  COM_HwFlowCtlTypeDef HwFlowCtl;                           
}COM_InitTypeDef;
#endif

#define MX_UART_InitTypeDef          COM_InitTypeDef
#define MX_UART_StopBitsTypeDef      COM_StopBitsTypeDef
#define MX_UART_ParityTypeDef        COM_ParityTypeDef
#define MX_UART_HwFlowCtlTypeDef     COM_HwFlowCtlTypeDef
#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
typedef struct
{
  void (* pMspInitCb)(UART_HandleTypeDef *);
  void (* pMspDeInitCb)(UART_HandleTypeDef *);
} BSP_COM_Cb_t;
#endif /* (USE_HAL_UART_REGISTER_CALLBACKS == 1) */

/**
 * @}
 */ 

/**
 * @}
 */ 

/** @defgroup NUCLEO_L476RG_LOW_LEVEL_BUTTON NUCLEO_L476RG LOW LEVEL BUTTON
 * @{
 */ 
/* Button state */
#define BUTTON_RELEASED                   0U
#define BUTTON_PRESSED                    1U 

#define BUTTONn                           1U

/**
 * @brief Key push-button
 */
#define KEY_BUTTON_PIN	                  GPIO_PIN_13
#define KEY_BUTTON_GPIO_PORT              GPIOC
#define KEY_BUTTON_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()   
#define KEY_BUTTON_GPIO_CLK_DISABLE()     __HAL_RCC_GPIOC_CLK_DISABLE()  
#define KEY_BUTTON_EXTI_IRQn              EXTI15_10_IRQn
#define KEY_BUTTON_EXTI_LINE              EXTI_LINE_13 

/**
 * @}
 */ 

/** @defgroup NUCLEO_L476RG_LOW_LEVEL_COM NUCLEO_L476RG LOW LEVEL COM
 * @{
 */
/**
 * @brief Definition for COM portx, connected to USART2
 */
#define COMn                             1U 
#define COM1_UART                        USART2

#define COM_POLL_TIMEOUT                 1000
;
extern UART_HandleTypeDef hcom_uart[COMn];
#define  huart2 hcom_uart[COM1]

/**
 * @}
 */
  
/**
  * @}
  */ 

/**
  * @}
  */

/** @defgroup NUCLEO_L476RG_LOW_LEVEL_Exported_Variables LOW LEVEL Exported Constants
  * @{
  */   
extern EXTI_HandleTypeDef* hExtiButtonHandle[BUTTONn];
/**
  * @}
  */ 
    
/** @defgroup NUCLEO_L476RG_LOW_LEVEL_Exported_Functions NUCLEO_L476RG LOW LEVEL Exported Functions
 * @{
 */ 

int32_t  BSP_GetVersion(void);  
int32_t  BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);
int32_t  BSP_PB_DeInit(Button_TypeDef Button);
int32_t  BSP_PB_GetState(Button_TypeDef Button);
void     BSP_PB_Callback(Button_TypeDef Button);
void     BSP_PB_IRQHandler (Button_TypeDef Button);
#if (USE_BSP_COM_FEATURE > 0)
int32_t  BSP_COM_Init(COM_TypeDef COM);
int32_t  BSP_COM_DeInit(COM_TypeDef COM);
#endif

#if (USE_COM_LOG > 0)
int32_t  BSP_COM_SelectLogPort(COM_TypeDef COM);
#endif

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1) 
int32_t BSP_COM_RegisterDefaultMspCallbacks(COM_TypeDef COM);
int32_t BSP_COM_RegisterMspCallbacks(COM_TypeDef COM , BSP_COM_Cb_t *Callback);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

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

#endif /* __NUCLEO_L476RG__H */
    
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
