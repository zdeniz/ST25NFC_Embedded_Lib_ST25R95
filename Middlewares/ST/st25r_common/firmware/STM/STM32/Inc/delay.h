/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2016 STMicroelectronics, all rights reserved
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

/*
 *      PROJECT:   ST25R391x firmware
 *      $Revision: $
 *      LANGUAGE:  ANSI C
 */

/*! \file delay.h
 *
 *  \brief SW Timer implementation header file
 *   
 *   This module makes use of a System Tick in millisconds and provides
 *   an abstraction for SW timers
 *
 */
 
 
 /*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "platform.h"

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
 
 /*! 
 *****************************************************************************
 * \brief  Microseconds Delay
 *  
 * This method delay for microseconds
 * 
 * \param[in]  micros : delay in Mikroseconds
 *
 * \return : void
 *****************************************************************************
 */
void delayUs(uint32_t micros);

uint32_t getUs(void);
