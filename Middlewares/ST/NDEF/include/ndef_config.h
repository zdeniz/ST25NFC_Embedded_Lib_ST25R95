/******************************************************************************
  * \attention
  *
  * <h2><center>&copy; COPYRIGHT 2020 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        www.st.com/myliberty
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
 *      PROJECT:   NDEF firmware
 *      Revision:
 *      LANGUAGE:  ISO C99
 */

/*! \file
 *
 *  \author
 *
 *  \brief NDEF config header file
 *
 * This file allows to select which NDEF features to use.
 *
 * \addtogroup NDEF
 * @{
 *
 */

#ifndef NDEF_CONFIG_H
#define NDEF_CONFIG_H


/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * NDEF FEATURES CONFIGURATION
 ******************************************************************************
 */


#ifdef NDEF_CONFIG_CUSTOM

#include "ndef_config_custom.h"

/* Define default configuration when not set in ndef_config_custom.h */
#ifndef NDEF_FEATURE_FULL_API
#define NDEF_FEATURE_FULL_API                  false       /*!< Support Write, Format, Check Presence, set Read-only in addition to the Read feature */
#endif

#ifndef NDEF_TYPE_EMPTY_SUPPORT
#define NDEF_TYPE_EMPTY_SUPPORT                false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_FLAT_SUPPORT
#define NDEF_TYPE_FLAT_SUPPORT                 false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_RTD_AAR_SUPPORT
#define NDEF_TYPE_RTD_AAR_SUPPORT              false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT
#define NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT      false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_RTD_URI_SUPPORT
#define NDEF_TYPE_RTD_URI_SUPPORT              false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_RTD_TEXT_SUPPORT
#define NDEF_TYPE_RTD_TEXT_SUPPORT             false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_RTD_WLC_SUPPORT
#define NDEF_TYPE_RTD_WLC_SUPPORT              false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_MEDIA_SUPPORT
#define NDEF_TYPE_MEDIA_SUPPORT                false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_BLUETOOTH_SUPPORT
#define NDEF_TYPE_BLUETOOTH_SUPPORT            false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_VCARD_SUPPORT
#define NDEF_TYPE_VCARD_SUPPORT                false      /* NDEF library configuration missing. Disabled by default */
#endif
#ifndef NDEF_TYPE_WIFI_SUPPORT
#define NDEF_TYPE_WIFI_SUPPORT                 false      /* NDEF library configuration missing. Disabled by default */
#endif

#else

#define NDEF_FEATURE_FULL_API                  true       /*!< Support Write, Format, Check Presence, set Read-only in addition to the Read feature */

#define NDEF_TYPE_EMPTY_SUPPORT                true       /*!< Support Empty type                          */
#define NDEF_TYPE_FLAT_SUPPORT                 true       /*!< Support Flat type                           */
#define NDEF_TYPE_RTD_DEVICE_INFO_SUPPORT      true       /*!< Support RTD Device Information type         */
#define NDEF_TYPE_RTD_TEXT_SUPPORT             true       /*!< Support RTD Text type                       */
#define NDEF_TYPE_RTD_URI_SUPPORT              true       /*!< Support RTD URI type                        */
#define NDEF_TYPE_RTD_AAR_SUPPORT              true       /*!< Support RTD Android Application Record type */
#define NDEF_TYPE_RTD_WLC_SUPPORT              true       /*!< Support WLC Types                           */
#define NDEF_TYPE_MEDIA_SUPPORT                true       /*!< Support Media type                          */
#define NDEF_TYPE_BLUETOOTH_SUPPORT            true       /*!< Support Bluetooth types                     */
#define NDEF_TYPE_VCARD_SUPPORT                true       /*!< Support vCard type                          */
#define NDEF_TYPE_WIFI_SUPPORT                 true       /*!< Support Wifi type                           */

#endif /* NDEF_CONFIG_CUSTOM */

#endif

/**
  * @}
  *
  */
