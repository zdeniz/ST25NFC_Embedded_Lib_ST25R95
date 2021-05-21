/******************************************************************************
  * \attention
  *
  * <h2><center>&copy; COPYRIGHT 2019 STMicroelectronics</center></h2>
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
 *  \brief Provides NDEF methods and definitions to access NFC Forum Tags
 *
 *  This module provides an interface to handle NDEF message
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_poller.h"
#include "ndef_t2t.h"
#include "ndef_t3t.h"
#include "ndef_t4t.h"
#include "ndef_t5t.h"
#include "utils.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
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

/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */

static ndefDeviceType ndefPollerGetDeviceType(const rfalNfcDevice *dev);

/*
 ******************************************************************************
 * GLOBAL VARIABLE DEFINITIONS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


/*******************************************************************************/
ReturnCode ndefPollerContextInitialization(ndefContext *ctx, const rfalNfcDevice *dev)
{
#if RFAL_FEATURE_T1T
    static const ndefPollerWrapper ndefT1TWrapper =
    {
        NULL, /* ndefT1TPollerContextInitialization, */
        NULL, /* ndefT1TPollerNdefDetect,            */
        NULL, /* ndefT1TPollerReadBytes,             */
        NULL, /* ndefT1TPollerReadRawMessage,        */
#if NDEF_FEATURE_FULL_API
        NULL, /* ndefT1TPollerWriteBytes,            */
        NULL, /* ndefT1TPollerWriteRawMessage,       */
        NULL, /* ndefT1TPollerTagFormat,             */
        NULL, /* ndefT1TPollerWriteRawMessageLen     */
        NULL, /* ndefT1TPollerCheckPresence          */
        NULL, /* ndefT1TPollerCheckAvailableSpace    */
        NULL, /* ndefT1TPollerBeginWriteMessage      */
        NULL, /* ndefT1TPollerEndWriteMessage        */
        NULL  /* ndefT1TPollerSetReadOnly            */
#endif /* NDEF_FEATURE_FULL_API */
    };
#endif /* RFAL_FEATURE_T1T */

#if RFAL_FEATURE_T2T
    static const ndefPollerWrapper ndefT2TWrapper =
    {
        ndefT2TPollerContextInitialization,
        ndefT2TPollerNdefDetect,
        ndefT2TPollerReadBytes,
        ndefT2TPollerReadRawMessage,
#if NDEF_FEATURE_FULL_API
        ndefT2TPollerWriteBytes,
        ndefT2TPollerWriteRawMessage,
        ndefT2TPollerTagFormat,
        ndefT2TPollerWriteRawMessageLen,
        ndefT2TPollerCheckPresence,
        ndefT2TPollerCheckAvailableSpace,
        ndefT2TPollerBeginWriteMessage,
        ndefT2TPollerEndWriteMessage,
        ndefT2TPollerSetReadOnly
#endif /* NDEF_FEATURE_FULL_API */
    };
#endif /* RFAL_FEATURE_T2T */

#if RFAL_FEATURE_NFCF
    static const ndefPollerWrapper ndefT3TWrapper =
    {
        ndefT3TPollerContextInitialization,
        ndefT3TPollerNdefDetect,
        ndefT3TPollerReadBytes,
        ndefT3TPollerReadRawMessage,
#if NDEF_FEATURE_FULL_API
        ndefT3TPollerWriteBytes,
        ndefT3TPollerWriteRawMessage,
        ndefT3TPollerTagFormat,
        ndefT3TPollerWriteRawMessageLen,
        ndefT3TPollerCheckPresence,
        ndefT3TPollerCheckAvailableSpace,
        ndefT3TPollerBeginWriteMessage,
        ndefT3TPollerEndWriteMessage,
        ndefT3TPollerSetReadOnly
#endif /* NDEF_FEATURE_FULL_API */
    };
#endif /* RFAL_FEATURE_NFCF */

#if RFAL_FEATURE_T4T
    static const ndefPollerWrapper ndefT4TWrapper =
    {
        ndefT4TPollerContextInitialization,
        ndefT4TPollerNdefDetect,
        ndefT4TPollerReadBytes,
        ndefT4TPollerReadRawMessage,
#if NDEF_FEATURE_FULL_API
        ndefT4TPollerWriteBytes,
        ndefT4TPollerWriteRawMessage,
        ndefT4TPollerTagFormat,
        ndefT4TPollerWriteRawMessageLen,
        ndefT4TPollerCheckPresence,
        ndefT4TPollerCheckAvailableSpace,
        ndefT4TPollerBeginWriteMessage,
        ndefT4TPollerEndWriteMessage,
        ndefT4TPollerSetReadOnly
#endif /* NDEF_FEATURE_FULL_API */
    };
#endif /* RFAL_FEATURE_T4T */

#if RFAL_FEATURE_NFCV
    static const ndefPollerWrapper ndefT5TWrapper =
    {
        ndefT5TPollerContextInitialization,
        ndefT5TPollerNdefDetect,
        ndefT5TPollerReadBytes,
        ndefT5TPollerReadRawMessage,
#if NDEF_FEATURE_FULL_API
        ndefT5TPollerWriteBytes,
        ndefT5TPollerWriteRawMessage,
        ndefT5TPollerTagFormat,
        ndefT5TPollerWriteRawMessageLen,
        ndefT5TPollerCheckPresence,
        ndefT5TPollerCheckAvailableSpace,
        ndefT5TPollerBeginWriteMessage,
        ndefT5TPollerEndWriteMessage,
        ndefT5TPollerSetReadOnly
#endif /* NDEF_FEATURE_FULL_API */
    };
#endif /* RFAL_FEATURE_NFCV */

    static const ndefPollerWrapper *ndefPollerWrappers[] =
    {
        NULL,
#if RFAL_FEATURE_T1T
        &ndefT1TWrapper,
#else
        NULL,
#endif
#if RFAL_FEATURE_T2T
        &ndefT2TWrapper,
#else
        NULL,
#endif
#if RFAL_FEATURE_NFCF
        &ndefT3TWrapper,
#else
        NULL,
#endif
#if RFAL_FEATURE_T4T
        &ndefT4TWrapper,
#else
        NULL,
#endif
#if RFAL_FEATURE_NFCV
        &ndefT5TWrapper,
#else
        NULL,
#endif
    };

    if( (ctx == NULL) || (dev == NULL) )
    {
        return ERR_PARAM;
    }

    ctx->ndefPollWrapper = ndefPollerWrappers[ndefPollerGetDeviceType(dev)];

    /* ndefPollWrapper is NULL when support of a given tag type is not enabled */
    if( (ctx->ndefPollWrapper == NULL) || (ctx->ndefPollWrapper->pollerContextInitialization == NULL) )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerContextInitialization)(ctx, dev);
}

/*******************************************************************************/
ReturnCode ndefPollerNdefDetect(ndefContext *ctx, ndefInfo *info)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerNdefDetect == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerNdefDetect)(ctx, info);
}

/*******************************************************************************/
ReturnCode ndefPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerReadRawMessage == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerReadRawMessage)(ctx, buf, bufLen, rcvdLen);
}

/*******************************************************************************/
ReturnCode ndefPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerReadBytes == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerReadBytes)(ctx, offset, len, buf, rcvdLen);
}

#if NDEF_FEATURE_FULL_API

/*******************************************************************************/
ReturnCode ndefPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerWriteRawMessage == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerWriteRawMessage)(ctx, buf, bufLen);
}

/*******************************************************************************/
ReturnCode ndefPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerTagFormat == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerTagFormat)(ctx, cc, options);
}

/*******************************************************************************/
ReturnCode ndefPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerWriteRawMessageLen == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerWriteRawMessageLen)(ctx, rawMessageLen);
}

/*******************************************************************************/
ReturnCode ndefPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerWriteBytes == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerWriteBytes)(ctx, offset, buf, len);
}

/*******************************************************************************/
ReturnCode ndefPollerCheckPresence(ndefContext *ctx)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerCheckPresence == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerCheckPresence)(ctx);
}

/*******************************************************************************/
ReturnCode ndefPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerCheckAvailableSpace == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerCheckAvailableSpace)(ctx, messageLen);
}

/*******************************************************************************/
ReturnCode ndefPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerBeginWriteMessage == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerBeginWriteMessage)(ctx, messageLen);
}

/*******************************************************************************/
ReturnCode ndefPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerEndWriteMessage == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerEndWriteMessage)(ctx, messageLen);
}

/*******************************************************************************/
ReturnCode ndefPollerSetReadOnly(ndefContext *ctx)
{
    if( ctx == NULL )
    {
        return ERR_PARAM;
    }

    if( ctx->ndefPollWrapper == NULL )
    {
        return ERR_WRONG_STATE;
    }

    if( ctx->ndefPollWrapper->pollerSetReadOnly == NULL )
    {
        return ERR_NOTSUPP;
    }

    return (ctx->ndefPollWrapper->pollerSetReadOnly)(ctx);
}

#endif /* NDEF_FEATURE_FULL_API */

/*******************************************************************************/
static ndefDeviceType ndefPollerGetDeviceType(const rfalNfcDevice *dev)
{
    ndefDeviceType type;
    
    if( dev == NULL )
    {
        type = NDEF_DEV_NONE;
    }
    else
    {
        switch( dev->type )
        {
        case RFAL_NFC_LISTEN_TYPE_NFCA:
            switch( dev->dev.nfca.type )
            {
                case RFAL_NFCA_T1T:
                    type = NDEF_DEV_T1T;
                    break; 
                case RFAL_NFCA_T2T:
                    type = NDEF_DEV_T2T;
                    break;
                case RFAL_NFCA_T4T:
                    type = NDEF_DEV_T4T;
                    break;
                default:
                    type = NDEF_DEV_NONE;
                    break;
            }
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCB:
            type = NDEF_DEV_T4T;
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCF:
            type = NDEF_DEV_T3T;
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCV:
            type = NDEF_DEV_T5T;
            break;
        default:
            type = NDEF_DEV_NONE;
            break;
        }
    }
    return type;
}
