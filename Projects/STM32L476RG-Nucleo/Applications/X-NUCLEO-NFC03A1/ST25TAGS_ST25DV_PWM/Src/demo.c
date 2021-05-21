/******************************************************************************
  * @attention
  *
  * COPYRIGHT 2020 STMicroelectronics, all rights reserved
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
 *  This demo shows how to poll for ST25DV-PWM Type 5 NFC cards/devices and how
 *  to exchange data with these devices, using the ST25Tags and RFAL libraries.
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
#include "st25dv_pwm.h"

/** @addtogroup X-CUBE-NFC3_Applications
 *  @{
 */

/** @addtogroup ST25Tags
 *  @{
 */

/** @addtogroup ST25DV_PWM
 * @{
 */

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
/** @defgroup ST25Tags_Demo_Private_Define
 * @{
 */
/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0     /*!< Demo State:  Not initialized        */
#define DEMO_ST_START_DISCOVERY       1     /*!< Demo State:  Start Discovery        */
#define DEMO_ST_DISCOVERY             2     /*!< Demo State:  Discovery              */

#define DEMO_NFCV_BLOCK_LEN           4     /*!< NFCV Block len                      */
#define DEMO_NFCV_USE_SELECT_MODE     false /*!< NFCV demonstrate select mode        */

#define DEMO_ST25DVPWM_NB_LOOPS       3     /*!< Number of PWM output write loops    */
#define DEMO_LED_BLINK_RATE_MS        100   /*!< Time LEDs are on in milliseconds    */

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
/** @defgroup ST25Tags_Demo_Private_Variables
 * @{
 */
static rfalNfcDiscoverParam discParam;
static uint8_t  state = DEMO_ST_NOTINIT;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static void demoNfcv( rfalNfcvListenDevice *nfcvDev );
static void demoSt25DvPwm( rfalNfcvListenDevice *nfcvDev );

static void demoNotif( rfalNfcState st );
static void reportCommandError(ReturnCode errCode);


/** @defgroup ST25Tags_Demo_Private_Functions
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
    uint8_t  rxBuf[ 1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN ];
    uint16_t rcvLen;

    rfalNfcWorker();    /* Run RFAL worker periodically */

    /*******************************************************************************/
    /* Check if USER button is pressed */
    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
    {
        discParam.wakeupEnabled = !discParam.wakeupEnabled;    /* enable/disable wakeup */
        state = DEMO_ST_START_DISCOVERY;                       /* restart loop          */
        platformLog("Toggling Wake Up mode %s\r\n", discParam.wakeupEnabled ? "ON": "OFF");

        /* Debounce button */
        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
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
                        platformLog("Felica/NFC-F card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
                        platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
                        break;

                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCV:
                        {
                            uint8_t devUID[RFAL_NFCV_UID_LEN];

                            ST_MEMCPY( devUID, nfcDevice->nfcid, nfcDevice->nfcidLen );     /* Copy the UID into local var */
                            REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );                     /* Reverse the UID for display purposes */
                            platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));

                            platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);

                            /* Test if device is a ST25DV-PWM tag */
                            if ( ST25TAGS_DVPWM_isTagSt25DvPwm(nfcDevice->nfcid) )
                            {
                                platformLog(" ST25DV-PWM tag detected.\r\n");
                                demoSt25DvPwm(&nfcDevice->dev.nfcv);
                            }
                            else
                            {
                                platformLog(" -> Not a ST25DV-PWM tag.\r\n");
                                demoNfcv( &nfcDevice->dev.nfcv );
                            }

                            /* Loop until tag is removed from the field */
                            platformLog("Operation completed\r\nTag can be removed from the field\r\n");
                            while ( rfalNfcvPollerReadSingleBlock( RFAL_NFCV_REQ_FLAG_DEFAULT, nfcDevice->nfcid, 0, rxBuf, sizeof(rxBuf), &rcvLen ) == ERR_NONE)
                            {
                                platformDelay(130);
                            }
                            platformLog("Tag %s has left the field\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));
                        }
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
    uint16_t              rcvLen;
    uint8_t               blockNum = 1;
    uint8_t               rxBuf[ 1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN ];                        /* Flags + Block Data + CRC */
    uint8_t *             uid;
#if DEMO_ENABLE_POLL_WRITE_TAG
    uint8_t               wrData[DEMO_NFCV_BLOCK_LEN] = { 0x11, 0x22, 0x33, 0x99 };             /* Write block example */
#endif /* DEMO_ENABLE_POLL_WRITE_TAG */

    uid = nfcvDev->InvRes.UID;

    #if DEMO_NFCV_USE_SELECT_MODE
        /*
        * Activate selected state
        */
        err = rfalNfcvPollerSelect(RFAL_NFCV_REQ_FLAG_DEFAULT, nfcvDev->InvRes.UID );
        platformLog(" Select %s \r\n", (err != ERR_NONE) ? "FAIL (revert to addressed mode)": "OK" );
        if( err == ERR_NONE )
        {
            uid = NULL;
        }
    #endif

    /*
    * Read block using Read Single Block command
    * with addressed mode (uid != NULL) or selected mode (uid == NULL)
    */
    err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
    platformLog(" Read Block: %s %s\r\n", (err != ERR_NONE) ? "FAIL": "OK Data:", (err != ERR_NONE) ? "" : hex2Str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));

#if DEMO_ENABLE_POLL_WRITE_TAG /* Writing example */
        err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, wrData, sizeof(wrData));
        platformLog(" Write Block: %s Data: %s\r\n", (err != ERR_NONE) ? "FAIL": "OK", hex2Str( wrData, DEMO_NFCV_BLOCK_LEN) );
        err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
        platformLog(" Read Block: %s %s\r\n", (err != ERR_NONE) ? "FAIL": "OK Data:", (err != ERR_NONE) ? "" : hex2Str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));
#endif /* DEMO_ENABLE_POLL_WRITE_TAG */
}

/*!
 *****************************************************************************
 * \brief Demo usage of ST25Tags st25dv-pwm's API
 *
 * Demo will first recognize the ST25DV-PWM tag type then perform some commands.
 * Best seen using a ST25DV-PWM-eSET board connected to a USB power supply.
 *
 *****************************************************************************
 */
static void demoSt25DvPwm( rfalNfcvListenDevice *nfcvDev )
{
    ReturnCode      errCode;
    uint16_t        rcvLen;
    uint8_t         blockNum = 1;
    uint8_t         rxBuf[ 1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN ];    /* Flags + Block Data + CRC */
    uint8_t *       uid;
    ST25TAGS_DVPWM_TagType_t tagType;

    uint8_t         configPassword[] = {0, 0, 0, 0};
    uint8_t         dutyCycle = 0;


    uid = nfcvDev->InvRes.UID;

    /* Test if tag is ST25DV-PWM */
    tagType = ST25TAGS_DVPWM_GetSt25PwmTagType( uid );

    if (tagType == UNKNOWN_DVPWM_TAG)
    {
        platformLog("ST25DV PWM Tag not recognized. Exiting demo.\r\n");
        return;
    }

    platformLog("  -> %s tag identified\r\n\r\n", ST25TAGS_ST25DVPWM_tagName[tagType]);

    /*
    * RFAL example use: Read block using Read Single Block command
    * with addressed mode (uid != NULL) or selected mode (uid == NULL)
    */
    errCode = rfalNfcvPollerReadSingleBlock( RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen );
    platformLog( " Read Block %d: %s %s\r\n\r\n", blockNum, (errCode != ERR_NONE) ? "FAIL": "OK - Data = ", (errCode != ERR_NONE) ? "" : hex2Str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN) );

    /* Open ST25DV-PWM Security Session to access configuration commands and registers */
    platformLog( " Calling ST25TAGS_DVPWM_OpenSecuritySession to open a Configuration session...\r\n");
    errCode = ST25TAGS_DVPWM_OpenSecuritySession( uid, CONFIGURATION_SECURITY_SESSION, configPassword );
    reportCommandError( errCode );

    /* Configure memory into a single user area */
    platformLog( " Calling ST25TAGS_DVPWM_SetNumberOfUserAreas for 1 single user area...\r\n" );
    errCode = ST25TAGS_DVPWM_SetNumberOfUserAreas( uid, 1 );
    reportCommandError( errCode );

    /* Set AREA1 protection to Read: Always / Write: Always */
    platformLog( " Calling ST25TAGS_DVPWM_ProtectUserArea for AREA1 to set Read: always / Write: protected by password...\r\n" );
    errCode = ST25TAGS_DVPWM_ProtectUserArea( uid, AREA_USER_1, READABLE_AND_WRITE_PROTECTED_BY_PWD );
    reportCommandError( errCode );

    /* Set PWM output power (optional, default = 100%) */
    platformLog( " Calling ST25TAGS_DVPWM_ConfigurePwmOutputPower for PWM1 at 75%% of Imax level\r\n" );
    errCode =  ST25TAGS_DVPWM_ConfigurePwmOutputPower( uid, DVPWM_OUTPUT_PWM1, THREE_QUARTER_POWER );
    reportCommandError( errCode );

    if ( ST25TAGS_DVPWM_HAS_TWO_OUTPUTS(tagType) )
    {
        platformLog( " Calling ST25TAGS_DVPWM_ConfigurePwmOutputPower for PWM2 at 50%% of Imax level\r\n" );
        errCode =  ST25TAGS_DVPWM_ConfigurePwmOutputPower( uid, DVPWM_OUTPUT_PWM2, HALF_POWER );
        reportCommandError( errCode );
    }

    /* Configure behavior of PWM output during RF communication (optional, default is no change in PWM output) */
    platformLog( " Defining improvements to RF noise immunity for all PWM outputs...\r\n" );
    errCode = ST25TAGS_DVPWM_ConfigurePwmOutputDuringRfCommand(uid, DVPWM_PWM_OUTPUT_QUARTER_POWER_FULL_FREQUENCY);
    reportCommandError(errCode);

    /* Enable PWM outputs */
    platformLog( " Enabling PWM1 output...\r\n" );
    errCode = ST25TAGS_DVPWM_EnablePwmOutputSignal( uid, DVPWM_OUTPUT_PWM1, true );
    reportCommandError(errCode);

    if ( ST25TAGS_DVPWM_HAS_TWO_OUTPUTS(tagType) )
    {
        platformLog( " Enabling PWM2 output...\r\n" );
        errCode = ST25TAGS_DVPWM_EnablePwmOutputSignal( uid, DVPWM_OUTPUT_PWM2, true );
        reportCommandError( errCode );
    }

    /* Configure the PWM outputs */
    platformLog( " Configuring PWM1 output...\r\n" );
    errCode = ST25TAGS_DVPWM_SetPwmOutputSignal( uid, DVPWM_OUTPUT_PWM1, 20000, 0 );
    reportCommandError(errCode);

    /* Modify duty cycle values to light up bars of leds on ST25DV-PWM-eSET board */
    platformLog( " Power on the ST25DV-PWM-eSET board to see the light show!\r\n" );
    for (uint8_t nbLoops = 0, toggle = 0; nbLoops < DEMO_ST25DVPWM_NB_LOOPS; nbLoops++)
    {
        toggle = nbLoops % 2;

        for ( dutyCycle = 0; dutyCycle < 100; dutyCycle += 10 )
        {
            ST25TAGS_DVPWM_SetPwmOutputSignal( uid, DVPWM_OUTPUT_PWM1, 20000, (toggle == 0 ? dutyCycle : 100 - dutyCycle) );

            if ( ST25TAGS_DVPWM_HAS_TWO_OUTPUTS(tagType) )
            {
                ST25TAGS_DVPWM_SetPwmOutputSignal( uid, DVPWM_OUTPUT_PWM2, 20000, (toggle == 0 ? 100 - dutyCycle : dutyCycle) );
            }

            platformDelay( DEMO_LED_BLINK_RATE_MS );
        }
    }
}


void reportCommandError(ReturnCode errCode)
{
    platformLog("   ==> Returned value: %d", (uint8_t)errCode);

    switch (errCode)
    {
        case ERR_NONE:
            platformLog(" - Command was successful\r\n");
            break;
        case ERR_PARAM:
            platformLog(" - Command failed: bad parameter!\r\n");
            break;
        case ERR_REQUEST:
            platformLog(" - Command failed: cannot be executed at the moment! Check that a security session is open.\r\n");
            break;
        case ERR_TIMEOUT:
            platformLog(" - Command failed: tag is not responding! Check that it is still in the RF field.\r\n");
            break;
        default:
            platformLog(" - Command failed!\r\n");
            break;
    }

    platformLog("\r\n");
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
