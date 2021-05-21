/**
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
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

/*! \file
 *
 *  \author 
 *
 *  \brief SPI communication handling implementation.
 *
 */
 
/* Includes ------------------------------------------------------------------*/

#include "spi.h"
#include "utils.h"
#include "st_errno.h"

#define SPI_TIMEOUT   1000

SPI_HandleTypeDef *pSpi = NULL;


void spiInit(SPI_HandleTypeDef *hspi)
{
    pSpi = hspi;

    /* enabling SPI block will put SCLK to output, guaranteeing proper state when spiSelect() gets called */
    __HAL_SPI_ENABLE(hspi);
}

void spiSelect(GPIO_TypeDef *ssPort, uint16_t ssPin)
{
    HAL_GPIO_WritePin(ssPort, ssPin, GPIO_PIN_RESET);
}

void spiDeselect(GPIO_TypeDef *ssPort, uint16_t ssPin)
{
    HAL_GPIO_WritePin(ssPort, ssPin, GPIO_PIN_SET);
}

HAL_StatusTypeDef spiTxRx(const uint8_t *txData, uint8_t *rxData, uint16_t length)
{
    if(pSpi == NULL)
    {
        return HAL_ERROR;
    }
  
    if( (txData != NULL) && (rxData == NULL) )
    {
        return HAL_SPI_Transmit(pSpi, (uint8_t*)txData, length, SPI_TIMEOUT);
    }
    else if( (txData == NULL) && (rxData != NULL) )
    {
        return HAL_SPI_Receive(pSpi, rxData, length, SPI_TIMEOUT);
    }

    return HAL_SPI_TransmitReceive(pSpi, (uint8_t*)txData, rxData, length, SPI_TIMEOUT);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
