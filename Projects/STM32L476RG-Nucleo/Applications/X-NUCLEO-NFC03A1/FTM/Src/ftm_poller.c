/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2019 STMicroelectronics, all rights reserved
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


/*! \file
 *
 *  \author 
 *
 *  \brief Demo application
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how 
 *  to exchange data with these devices, using the RFAL library.
 *
 *  This demo does not fully implement the activities according to the standards,
 *  it performs the required to communicate with a card/device and retrieve 
 *  its UID. Also blocking methods are used for data exchange which may lead to
 *  long periods of blocking CPU/MCU.
 *  For standard compliant example please refer to the Examples provided
 *  with the RFAL library.
 * 
 */
 
/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "demo.h"
#include "utils.h"
#include "rfal_nfc.h"
#include "stm32l4xx_nucleo.h"

#include "ftm_demo.h"

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
/** @defgroup PTD_Demo_Private_Define
 * @{
 */
/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0     /*!< Demo State:  Not initialized        */
#define DEMO_ST_START_DISCOVERY       1     /*!< Demo State:  Start Discovery        */
#define DEMO_ST_DISCOVERY             2     /*!< Demo State:  Discovery              */

#define DEMO_NFCV_USE_SELECT_MODE     true  /*!< NFCV demonstrate select mode        */

/**
  * @}
  */ 
/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
/** @defgroup PTD_Demo_Private_Variables 
 * @{
 */

  
/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
static rfalNfcDiscoverParam discParam;
static uint8_t              state = DEMO_ST_NOTINIT;

/**
  * @}
  */

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static void demoNfcv( rfalNfcvListenDevice *nfcvDev );
static void demoNotif( rfalNfcState st );

/** @defgroup PTD_Demo_Private_Functions
 * @{
 */
/*!
 *****************************************************************************
 * \brief Demo Notification
 *
 *  This function receives the event notifications from RFAL
 *****************************************************************************
 */
static void demoNotif( rfalNfcState st )
{
    uint8_t       devCnt;
    rfalNfcDevice *dev;
    
    
    if( st == RFAL_NFC_STATE_WAKEUP_MODE )
    {
        platformLog("Wake Up mode started \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_TECHDETECT )
    {
        platformLog("Wake Up mode terminated. Polling for devices \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_SELECT )
    {
        /* Multiple devices were found, activate first of them */
        rfalNfcGetDevicesFound( &dev, &devCnt );
        rfalNfcSelect( 0 );
        
        platformLog("Multiple Tags detected: %d \r\n", devCnt);
    }
}

/*!
 *****************************************************************************
 * \brief Demo Ini
 *
 *  This function Initializes the required layers for the demo
 *
 * \return true  : Initialization ok
 * \return false : Initialization failed
 *****************************************************************************
 */
bool demoIni( void )
{
    ReturnCode err;
    
    err = rfalNfcInitialize();
    if( err == ERR_NONE )
    {
        discParam.compMode      = RFAL_COMPLIANCE_MODE_NFC;
        discParam.devLimit      = 1U;
        discParam.nfcfBR        = RFAL_BR_212;
        discParam.ap2pBR        = RFAL_BR_424;
      
        discParam.notifyCb             = demoNotif;
        discParam.totalDuration        = 1000U;
        discParam.wakeupEnabled        = false;
        discParam.wakeupConfigDefault  = true;
        discParam.techs2Find           = RFAL_NFC_POLL_TECH_V;

        state = DEMO_ST_START_DISCOVERY;
        return true;
    }
    return false;
}

/*!
 *****************************************************************************
 * \brief Demo Cycle
 *
 *  This function executes the demo state machine. 
 *  It must be called periodically
 *****************************************************************************
 */
void demoCycle( void )
{
    static rfalNfcDevice *nfcDevice;
    
    rfalNfcWorker();                                    /* Run RFAL worker periodically */

    /*******************************************************************************/
    /* Check if USER button is pressed */
    if( BSP_PB_GetState(BUTTON_USER) == GPIO_PIN_RESET )
    {
        discParam.wakeupEnabled = !discParam.wakeupEnabled;    /* enable/disable wakeup */
        state = DEMO_ST_START_DISCOVERY;                       /* restart loop          */
        platformLog("Toggling Wake Up mode %s\r\n", discParam.wakeupEnabled ? "ON": "OFF");

        /* Debounce button */
        while( BSP_PB_GetState(BUTTON_USER) == GPIO_PIN_RESET );
    }
  
    
    switch( state )
    {
        /*******************************************************************************/
        case DEMO_ST_START_DISCOVERY:

          platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
          platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
          platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
          platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
          platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
          platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
          
          rfalNfcDeactivate( false );
          rfalNfcDiscover( &discParam );
          
          state = DEMO_ST_DISCOVERY;
          break;

        /*******************************************************************************/
        case DEMO_ST_DISCOVERY:
        
            if( rfalNfcIsDevActivated( rfalNfcGetState() ) )
            {
                rfalNfcGetActiveDevice( &nfcDevice );
                
                switch( nfcDevice->type )
                {
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCA:
                    
                        platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
                        switch( nfcDevice->dev.nfca.type )
                        {
                            case RFAL_NFCA_T1T:
                                platformLog("ISO14443A/Topaz (NFC-A T1T) TAG found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                break;
                            
                            case RFAL_NFCA_T4T:
                                platformLog("NFCA Passive ISO-DEP device found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                break;
 
                            case RFAL_NFCA_T4T_NFCDEP:
                            case RFAL_NFCA_NFCDEP:
                                platformLog("NFCA Passive P2P device found. NFCID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                break;
                            
                            default:
                                platformLog("ISO14443A/NFC-A card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                break;
                        }
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCB:
                        
                        platformLog("ISO14443B/NFC-B card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                        break;
                        
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCF:
                        if( rfalNfcfIsNfcDepSupported( &nfcDevice->dev.nfcf ) )
                        {
                            platformLog("NFCF Passive P2P device found. NFCID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                        }
                        else
                        {
                            platformLog("Felica/NFC-F card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
                        }
                        
                        platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCV:
                        {
                            uint8_t devUID[RFAL_NFCV_UID_LEN];
                            
                            ST_MEMCPY( devUID, nfcDevice->nfcid, nfcDevice->nfcidLen );   /* Copy the UID into local var */
                            REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );                 /* Reverse the UID for display purposes */
                            platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));
                        
                            platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
                            
                            demoNfcv( &nfcDevice->dev.nfcv );
                        }
                        break;
                        
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_ST25TB:
                        
                        platformLog("ST25TB card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_AP2P:
                        
                        platformLog("NFC Active P2P device found. NFCID3: %s\r\n", hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                        platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
                        break;
                    /*******************************************************************************/
                    case RFAL_NFC_POLL_TYPE_NFCA:
                    case RFAL_NFC_POLL_TYPE_NFCF:
                        platformLog("Activated in CE %s mode.\r\n", (nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? "NFC-A" : "NFC-F");
                        platformLedOn( ((nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? PLATFORM_LED_A_PORT : PLATFORM_LED_F_PORT), 
                                       ((nfcDevice->type == RFAL_NFC_POLL_TYPE_NFCA) ? PLATFORM_LED_A_PIN  : PLATFORM_LED_F_PIN)  );
                    
                        break;

                    /*******************************************************************************/
                    default:
                        break;
                }
                
                rfalNfcDeactivate( false );
                platformDelay( 500 );
                state = DEMO_ST_START_DISCOVERY;
            }
            break;

        /*******************************************************************************/
        case DEMO_ST_NOTINIT:
        default:
            break;
    }
}



/*!
 *****************************************************************************
 * \brief Demo NFC-V Exchange
 *
 * Example how to exchange read and write blocks on a NFC-V tag
 * 
 *****************************************************************************
 */
static void demoNfcv( rfalNfcvListenDevice *nfcvDev )
{
    ReturnCode            err;
    
    #if DEMO_NFCV_USE_SELECT_MODE
        /*
        * Activate selected state
        */
        err = rfalNfcvPollerSelect(RFAL_NFCV_REQ_FLAG_DEFAULT, nfcvDev->InvRes.UID );
        platformLog(" Select %s \r\n", (err != ERR_NONE) ? "FAIL (revert to addressed mode)": "OK" );
    #endif    
    ftm_demo(nfcvDev);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
