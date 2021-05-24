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
 *  \brief Provides NDEF methods and definitions to access NFC-V Forum T5T
 *
 *  This module provides an interface to perform as a NFC-V Reader/Writer
 *  to handle a Type 5 Tag T5T
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "ndef_poller.h"
#include "ndef_t5t.h"
#include "utils.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef RFAL_FEATURE_NFCV
    #error " RFAL: Module configuration missing. Please enable/disable T5T support by setting: RFAL_FEATURE_NFCV "
#endif

#if RFAL_FEATURE_NFCV

#ifndef NDEF_FEATURE_FULL_API
    #error " NDEF: Module configuration missing. Please enable/disable Full API by setting: NDEF_FEATURE_FULL_API"
#endif

#ifdef TEST_NDEF
#define NDEF_SKIP_T5T_SYS_INFO /* Must not call ndefT5TGetSystemInformation() in test mode */
#endif

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_T5T_UID_MANUFACTURER_ID_POS       6U    /*!< Manufacturer ID Offset in UID buffer (reverse)    */
#define NDEF_T5T_MANUFACTURER_ID_ST         0x02U    /*!< Manufacturer ID for ST                            */

#define NDEF_T5T_SYSINFO_MAX_LEN              22U    /*!< Max length for (Extended) Get System Info response */

#define NDEF_T5T_MLEN_DIVIDER                  8U    /*!<  T5T_area size is measured in bytes is equal to 8 * MLEN */

#define NDEF_T5T_TLV_T_LEN                     1U    /*!< TLV T Length: 1 bytes                             */
#define NDEF_T5T_TLV_L_1_BYTES_LEN             1U    /*!< TLV L Length: 1 bytes                             */
#define NDEF_T5T_TLV_L_3_BYTES_LEN             3U    /*!< TLV L Length: 3 bytes                             */

#define NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR       256U    /*!< Max number of blocks for 1 byte addressing        */
#define NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING    256U    /*!< MLEN max value for 1 byte encoding                */

#define NDEF_T5T_TL_MIN_SIZE  (NDEF_T5T_TLV_T_LEN \
                       + NDEF_T5T_TLV_L_1_BYTES_LEN) /*!< Min TL size                                       */

#define NDEF_T5T_TL_MAX_SIZE  (NDEF_T5T_TLV_T_LEN \
                       + NDEF_T5T_TLV_L_3_BYTES_LEN) /*!< Max TL size                                       */

#define NDEF_T5T_TLV_NDEF                   0x03U    /*!< TLV flag NDEF value                               */
#define NDEF_T5T_TLV_PROPRIETARY            0xFDU    /*!< TLV flag PROPRIETARY value                        */
#define NDEF_T5T_TLV_TERMINATOR             0xFEU    /*!< TLV flag TERMINATOR value                         */
#define NDEF_T5T_TLV_RFU                    0x00U    /*!< TLV flag RFU value                                */

#define NDEF_T5T_WR_ACCESS_ALWAYS           0x0U     /*!< Write Accces. 00b: Always                         */
#define NDEF_T5T_WR_ACCESS_RFU              0x1U     /*!< Write Accces. 01b: RFU                            */
#define NDEF_T5T_WR_ACCESS_PROPRIETARY      0x2U     /*!< Write Accces. 00b: Proprietary                    */
#define NDEF_T5T_WR_ACCESS_NEVER            0x3U     /*!< Write Accces. 00b: Never                          */

#ifndef NDEF_T5T_N_RETRY_ERROR
#define NDEF_T5T_N_RETRY_ERROR                2U     /*!< nT5T,RETRY,ERROR DP 2.2  §B.12                    */
#endif /* NDEF_T5T_N_RETRY_ERROR */

#define NDEF_T5T_FLAG_LEN                     1U     /*!< Flag byte length                                  */

#define NDEF_T5T_MAPPING_VERSION_1_0    (1U << 6)    /*!< T5T Version 1.0                                   */

/*
 *****************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define ndefT5TisT5TDevice(device) ((device)->type == RFAL_NFC_LISTEN_TYPE_NFCV)
#define ndefT5TInvalidateCache(ctx)     { (ctx)->subCtx.t5t.cacheBlock = 0xFFFFFFFFU; }
#define ndefT5TIsValidCache(ctx, block) ( (ctx)->subCtx.t5t.cacheBlock == (block) )

#define ndefT5TIsTransmissionError(err)      ( ((err) == ERR_FRAMING) || ((err) == ERR_CRC) || ((err) == ERR_PAR) || ((err) == ERR_TIMEOUT) )

#define ndefT5TMajorVersion(V)               ((uint8_t)((V) >> 6U))    /*!< Get major version */


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

static ReturnCode ndefT5TPollerReadSingleBlock(ndefContext *ctx, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);
static ReturnCode ndefT5TPollerReadMultipleBlocks(ndefContext *ctx, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

#if !defined NDEF_SKIP_T5T_SYS_INFO
static ReturnCode ndefT5TGetSystemInformation(ndefContext *ctx, bool extended);
#endif /* NDEF_SKIP_T5T_SYS_INFO */

#if NDEF_FEATURE_FULL_API
static ReturnCode ndefT5TWriteCC(ndefContext *ctx);
static ReturnCode ndefT5TPollerWriteSingleBlock(ndefContext *ctx, uint16_t blockNum, const uint8_t* wrData);
static ReturnCode ndefT5TPollerLockSingleBlock(ndefContext *ctx, uint16_t blockNum);
#endif /* NDEF_FEATURE_FULL_API */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

#ifdef TEST_NDEF

/*! Default T5T Access mode */
ndefT5TAccessMode gAccessMode = NDEF_T5T_ACCESS_MODE_SELECTED;

/*******************************************************************************/
/*  Make T5T Access mode accessible for test purpose */
ReturnCode ndefT5TPollerSetAccessMode(ndefT5TAccessMode mode)
{
    gAccessMode = mode;

    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerMultipleBlockRead(ndefContext *ctx, bool enable)
{
    if ( (ctx == NULL) || (ctx->state != NDEF_STATE_INVALID) )
    {
        return ERR_PARAM;
    }

    ctx->subCtx.t5t.useMultipleBlockRead = enable;

    return ERR_NONE;
}
#endif /* TEST_NDEF */


/*******************************************************************************/
static void ndefT5TPollerAccessMode(ndefContext *ctx, ndefT5TAccessMode mode)
{
    ndefT5TAccessMode accessMode = mode;
    ctx->subCtx.t5t.flags = (uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT;

    if (accessMode == NDEF_T5T_ACCESS_MODE_SELECTED)
    {
        if (rfalNfcvPollerSelect(ctx->subCtx.t5t.flags, ctx->device.dev.nfcv.InvRes.UID) == ERR_NONE)
        {
            /* Selected mode (AMS = 0, SMS = 1) */
            ctx->subCtx.t5t.uid    = NULL;
            ctx->subCtx.t5t.flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_SELECT;
        }
        else
        {
            /* Set Addressed mode if Selected mode failed */
            accessMode = NDEF_T5T_ACCESS_MODE_ADDRESSED;
        }
    }
    if (accessMode == NDEF_T5T_ACCESS_MODE_ADDRESSED)
    {
        /* Addressed mode (AMS = 1, SMS = 0) */
        ctx->subCtx.t5t.uid    = ctx->device.dev.nfcv.InvRes.UID;
        ctx->subCtx.t5t.flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_ADDRESS;
    }
    else if (accessMode == NDEF_T5T_ACCESS_MODE_NON_ADDRESSED)
    {
        /* Non-addressed mode (AMS = 0, SMS = 0) */
        ctx->subCtx.t5t.uid = NULL;
    }
    else
    {
        /* MISRA 15.7 - Empty else */
    }
}


/*******************************************************************************/
ReturnCode ndefT5TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
    uint8_t         lastVal;
    uint16_t        res;
    uint16_t        nbRead;
    uint16_t        blockLen;
    uint16_t        startBlock;
    uint16_t        startAddr;
    uint32_t        currentLen = len;
    uint32_t        lvRcvLen   = 0U;

    if ( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    if ( (ctx->subCtx.t5t.blockLen > 0U) && (len > 0U) )
    {
        blockLen = (uint16_t)ctx->subCtx.t5t.blockLen;
        if( blockLen == 0U )
        {
            return ERR_SYSTEM;
        }
        startBlock = (uint16_t) (offset / blockLen);
        startAddr  = (uint16_t) (startBlock * blockLen);

        res = ( (ctx->cc.t5t.multipleBlockRead == true) && (ctx->subCtx.t5t.useMultipleBlockRead == true) ) ?
              /* Read a single block using the ReadMultipleBlock command... */
              ndefT5TPollerReadMultipleBlocks(ctx, startBlock, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead) :
              ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
        if (res != ERR_NONE)
        {
            return res;
        }

        nbRead = (uint16_t) (nbRead  + startAddr - (uint16_t)offset - 1U);
        if ((uint32_t) nbRead > currentLen)
        {
            nbRead = (uint16_t) currentLen;
        }
        if (nbRead > 0U)
        {
            /* Remove the Flag byte */
            (void)ST_MEMCPY(buf, &ctx->subCtx.t5t.txrxBuf[1U - startAddr + (uint16_t)offset], nbRead);
        }
        lvRcvLen   += (uint32_t) nbRead;
        currentLen -= (uint32_t) nbRead;
        /* Process all blocks but not the last one */
        /* Rationale: ndefT5TPollerReadSingleBlock() reads 2 extra CRC bytes and could write after buffer end */
        while (currentLen > (uint32_t)blockLen)
        {
            startBlock++;
            lastVal = buf[lvRcvLen - 1U]; /* Read previous value that is going to be overwritten by status byte (1st byte in response) */

            res = ( (ctx->cc.t5t.multipleBlockRead == true) && (ctx->subCtx.t5t.useMultipleBlockRead == true) ) ?
                  /* Read a single block using the ReadMultipleBlock command... */
                  ndefT5TPollerReadMultipleBlocks(ctx, startBlock, 0U, &buf[lvRcvLen - 1U], blockLen + NDEF_T5T_FLAG_LEN + RFAL_CRC_LEN, &nbRead) :
                  ndefT5TPollerReadSingleBlock(ctx, startBlock, &buf[lvRcvLen - 1U], blockLen + NDEF_T5T_FLAG_LEN + RFAL_CRC_LEN, &nbRead);
            if (res != ERR_NONE)
            {
                return res;
            }

            buf[lvRcvLen - 1U] = lastVal; /* Restore previous value */

            lvRcvLen   += blockLen;
            currentLen -= blockLen;
        }
        if (currentLen > 0U)
        {
            /* Process the last block. Take care of removing status byte and 2 extra CRC bytes that could write after buffer end */
            startBlock++;

            res = ( (ctx->cc.t5t.multipleBlockRead == true) && (ctx->subCtx.t5t.useMultipleBlockRead == true) ) ?
                  /* Read a single block using the ReadMultipleBlock command... */
                  ndefT5TPollerReadMultipleBlocks(ctx, startBlock, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead) :
                  ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
            if (res != ERR_NONE)
            {
                return res;
            }

            nbRead--; /* Remove Flag byte */
            if (nbRead > currentLen)
            {
                nbRead = (uint16_t)currentLen;
            }
            if (nbRead > 0U)
            {
                (void)ST_MEMCPY(&buf[lvRcvLen], & ctx->subCtx.t5t.txrxBuf[1U], nbRead);
            }
            lvRcvLen   += nbRead;
            currentLen -= nbRead;
        }
    }
    if (currentLen != 0U)
    {
        return ERR_SYSTEM;
    }
    if( rcvdLen != NULL )
    {
        *rcvdLen = lvRcvLen;
    }
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerContextInitialization(ndefContext *ctx, const rfalNfcDevice *dev)
{
    ReturnCode    result;
    uint16_t      rcvLen;

    if( (ctx == NULL) || (dev == NULL) || !ndefT5TisT5TDevice(dev) )
    {
        return ERR_PARAM;
    }

    (void)ST_MEMCPY(&ctx->device, dev, sizeof(ctx->device));

    ndefT5TInvalidateCache(ctx);

    /* Reset info about the card */
    ctx->state                    = NDEF_STATE_INVALID;

    /* Initialize CC fields, used in NDEF detect */
    ctx->cc.t5t.ccLen             = 0U;
    ctx->cc.t5t.magicNumber       = 0U;
    ctx->cc.t5t.majorVersion      = 0U;
    ctx->cc.t5t.minorVersion      = 0U;
    ctx->cc.t5t.readAccess        = 0U;
    ctx->cc.t5t.writeAccess       = 0U;
    ctx->cc.t5t.memoryLen         = 0U;
    ctx->cc.t5t.specialFrame      = false;
    ctx->cc.t5t.lockBlock         = false;
    ctx->cc.t5t.mlenOverflow      = false;
    ctx->cc.t5t.multipleBlockRead = false;

    ctx->subCtx.t5t.blockLen      = 0U;
    ctx->subCtx.t5t.TlvNDEFOffset = 0U; /* Offset for TLV */
    ctx->subCtx.t5t.useMultipleBlockRead = false;

#ifdef TEST_NDEF
    ndefT5TPollerAccessMode(ctx, gAccessMode);
#else
    ndefT5TPollerAccessMode(ctx, NDEF_T5T_ACCESS_MODE_SELECTED);
#endif

    /* T5T v1.1 4.1.1.3 Retrieve the Block Length */
    ctx->subCtx.t5t.legacySTHighDensity = false;
    result = ndefT5TPollerReadSingleBlock(ctx, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen);
    if( (result != ERR_NONE) && (ctx->device.dev.nfcv.InvRes.UID[NDEF_T5T_UID_MANUFACTURER_ID_POS] == NDEF_T5T_MANUFACTURER_ID_ST) )
    {
        /* Try High Density Legacy mode */
        result = ndefT5TPollerReadSingleBlock(ctx, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen);
        if( result != ERR_NONE )
        {
            return result;
        }
        ctx->subCtx.t5t.legacySTHighDensity = true;
    }

    if( (rcvLen > 1U) && (ctx->subCtx.t5t.txrxBuf[0U] == (uint8_t) 0U) )
    {
        ctx->subCtx.t5t.blockLen = (uint8_t) (rcvLen - 1U);
    }
    else
    {
        return ERR_PROTO;
    }

    ctx->subCtx.t5t.sysInfoSupported = false;

#if !defined NDEF_SKIP_T5T_SYS_INFO
    if( !ctx->subCtx.t5t.legacySTHighDensity)
    {
        /* Extended Get System Info */
        if( ndefT5TGetSystemInformation(ctx, true) == ERR_NONE )
        {
            ctx->subCtx.t5t.sysInfoSupported = true;
        }
    }
    if( !ctx->subCtx.t5t.sysInfoSupported )
    {
        /* Get System Info */
        if( ndefT5TGetSystemInformation(ctx, false) == ERR_NONE )
        {
            ctx->subCtx.t5t.sysInfoSupported = true;
        }
    }
#endif
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerNdefDetect(ndefContext *ctx, ndefInfo *info)
{
    ReturnCode result;
    uint8_t    tmpBuf[NDEF_T5T_TL_MAX_SIZE];
    ReturnCode returnCode = ERR_REQUEST; /* Default return code */
    uint16_t   offset;
    uint16_t   length;
    uint32_t   TlvOffset;
    bool       exit;
    uint32_t   rcvLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    ctx->state                           = NDEF_STATE_INVALID;
    ctx->cc.t5t.ccLen                    = 0U;
    ctx->cc.t5t.memoryLen                = 0U;
    ctx->cc.t5t.multipleBlockRead        = false;
    ctx->messageLen                      = 0U;
    ctx->messageOffset                   = 0U;
    ctx->areaLen                         = 0U;

    if( info != NULL )
    {
        info->state                = NDEF_STATE_INVALID;
        info->majorVersion         = 0U;
        info->minorVersion         = 0U;
        info->areaLen              = 0U;
        info->areaAvalableSpaceLen = 0U;
        info->messageLen           = 0U;
    }

    result = ndefT5TPollerReadBytes(ctx, 0U, 4U, ctx->ccBuf, &rcvLen);
    if ( (result == ERR_NONE) && (rcvLen == 4U) && ( (ctx->ccBuf[0] == (uint8_t)0xE1U) || (ctx->ccBuf[0] == (uint8_t)0xE2U) ) )
    {
        ctx->cc.t5t.magicNumber           =  ctx->ccBuf[0U];
        ctx->cc.t5t.majorVersion          = (ctx->ccBuf[1U] >> 6U ) & 0x03U;
        ctx->cc.t5t.minorVersion          = (ctx->ccBuf[1U] >> 4U ) & 0x03U;
        ctx->cc.t5t.readAccess            = (ctx->ccBuf[1U] >> 2U ) & 0x03U;
        ctx->cc.t5t.writeAccess           = (ctx->ccBuf[1U] >> 0U ) & 0x03U;
        ctx->cc.t5t.memoryLen             =  ctx->ccBuf[2U];
        ctx->cc.t5t.specialFrame          = (((ctx->ccBuf[3U] >> 4U ) & 0x01U) != 0U);
        ctx->cc.t5t.lockBlock             = (((ctx->ccBuf[3U] >> 3U ) & 0x01U) != 0U);
        ctx->cc.t5t.mlenOverflow          = (((ctx->ccBuf[3U] >> 2U ) & 0x01U) != 0U);
        /* Read the CC with Single Block Read command(s) and update multipleBlockRead flag after */
        ctx->state                        = NDEF_STATE_INITIALIZED;

        /* Check Magic Number TS T5T v1.0 - 7.5.1.2 */
        if( (ctx->cc.t5t.magicNumber != NDEF_T5T_CC_MAGIC_1_BYTE_ADDR_MODE) &&
            (ctx->cc.t5t.magicNumber != NDEF_T5T_CC_MAGIC_2_BYTE_ADDR_MODE) )
        {
            return ERR_REQUEST;
        }

        /* Check version - 7.5.1.2 */
        if( ctx->cc.t5t.majorVersion > ndefT5TMajorVersion(NDEF_T5T_MAPPING_VERSION_1_0) )
        {
            return ERR_REQUEST;
        }

        /* Check read access - 7.5.1.2 */
        if( ctx->cc.t5t.readAccess != 0U )
        {
            return ERR_REQUEST;
        }

        if( ctx->cc.t5t.memoryLen != 0U )
        {
            /* 4-byte CC */
            ctx->cc.t5t.ccLen         = NDEF_T5T_CC_LEN_4_BYTES;
            if( (ctx->cc.t5t.memoryLen == 0xFFU) && ctx->cc.t5t.mlenOverflow )
            {
                if( (ctx->subCtx.t5t.sysInfoSupported == true) && (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) )
                {
                    ctx->cc.t5t.memoryLen = (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER);
                }
            }
        }
        else
        {
            /* 8-byte CC */
            result = ndefT5TPollerReadBytes(ctx, 4U, 4U, &ctx->ccBuf[4U], &rcvLen);
            if ( (result == ERR_NONE) && (rcvLen == 4U) )
            {
                ctx->cc.t5t.ccLen     = NDEF_T5T_CC_LEN_8_BYTES;
                ctx->cc.t5t.memoryLen = ((uint16_t)ctx->ccBuf[6U] << 8U) + (uint16_t)ctx->ccBuf[7U];
            }
        }

        /* Update multipleBlockRead flag after having read the second half of 8-byte CC */
        ctx->cc.t5t.multipleBlockRead     = (((ctx->ccBuf[3U] >> 0U ) & 0x01U) != 0U);

        if( (ctx->subCtx.t5t.sysInfoSupported == true) &&
            (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) &&
            (ctx->cc.t5t.memoryLen == (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER)) &&
            (ctx->cc.t5t.memoryLen > 0U) )
        {
            ctx->cc.t5t.memoryLen--; /* remove CC area from memory length */
        }

        ctx->messageLen     = 0U;
        ctx->messageOffset  = ctx->cc.t5t.ccLen;
        /* TS T5T v1.0 4.3.1.17 T5T_Area size is measured in bytes, is equal to MLEN * 8 */
        ctx->areaLen        = (uint32_t)ctx->cc.t5t.memoryLen * NDEF_T5T_MLEN_DIVIDER;

        TlvOffset = ctx->cc.t5t.ccLen;
        exit      = false;
        while( (exit == false) && (TlvOffset < (ctx->cc.t5t.ccLen + ctx->areaLen)) )
        {
            result = ndefT5TPollerReadBytes(ctx, TlvOffset, NDEF_T5T_TL_MIN_SIZE, tmpBuf, &rcvLen);
            if ( (result != ERR_NONE) || ( rcvLen != NDEF_T5T_TL_MIN_SIZE) )
            {
                return result;
            }
            offset = NDEF_T5T_TLV_T_LEN + NDEF_T5T_TLV_L_1_BYTES_LEN;
            length = tmpBuf[1U];
            if ( length == (NDEF_SHORT_VFIELD_MAX_LEN + 1U) )
            {
                /* Size is encoded in 1 + 2 bytes */
                result = ndefT5TPollerReadBytes(ctx, TlvOffset, NDEF_T5T_TL_MAX_SIZE, tmpBuf, &rcvLen);
                if ( (result != ERR_NONE) || ( rcvLen != NDEF_T5T_TL_MAX_SIZE) )
                {
                    return result;
                }
                length = (((uint16_t)tmpBuf[2U]) << 8U) + (uint16_t)tmpBuf[3U];
                offset += 2U;
            }
            if (tmpBuf[0U] == (uint8_t)NDEF_T5T_TLV_NDEF)
            {
                /* NDEF record return it */
                returnCode                    = ERR_NONE;  /* Default */
                ctx->subCtx.t5t.TlvNDEFOffset = TlvOffset; /* Offset for TLV */
                ctx->messageOffset            = TlvOffset + offset;
                ctx->messageLen               = length;
                if (length == 0U)
                {
                    /* Req 40 7.5.1.6 */
                    if ( (ctx->cc.t5t.readAccess == 0U) && (ctx->cc.t5t.writeAccess == 0U) )
                    {
                        ctx->state = NDEF_STATE_INITIALIZED;
                    }
                    else
                    {
                        ctx->state = NDEF_STATE_INVALID;
                        returnCode = ERR_REQUEST; /* Default */
                    }
                    exit = true;
                }
                else
                {
                    if (ctx->cc.t5t.readAccess == 0U)
                    {
                        if (ctx->cc.t5t.writeAccess == 0U)
                        {
                            ctx->state = NDEF_STATE_READWRITE;
                        }
                        else
                        {
                            ctx->state = NDEF_STATE_READONLY;
                        }
                    }
                    exit = true;
                }
            }
            else if (tmpBuf[0U]== (uint8_t) NDEF_T5T_TLV_TERMINATOR)
            {
                /* NDEF end */
                exit = true;
            }
            else
            {
                /* Skip Proprietary and RFU too */
                TlvOffset += (uint32_t)offset + (uint32_t)length;
            }
        }
    }
    else
    {
        /* No CC File */
        returnCode = ERR_REQUEST;
        if (result != ERR_NONE)
        {
            returnCode = result;
        }
    }

    if( info != NULL )
    {
        info->state                = ctx->state;
        info->majorVersion         = ctx->cc.t5t.majorVersion;
        info->minorVersion         = ctx->cc.t5t.minorVersion;
        info->areaLen              = ctx->areaLen;
        info->areaAvalableSpaceLen = (uint32_t)ctx->cc.t5t.ccLen + ctx->areaLen - ctx->messageOffset;
        info->messageLen           = ctx->messageLen;
    }
    return returnCode;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen)
{
    ReturnCode result;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    if( ctx->messageLen > bufLen )
    {
        return ERR_NOMEM;
    }

    result = ndefT5TPollerReadBytes(ctx, ctx->messageOffset, ctx->messageLen, buf, rcvdLen);
    return result;
}

#if NDEF_FEATURE_FULL_API

/*******************************************************************************/
ReturnCode ndefT5TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len)
{
    ReturnCode      result = ERR_REQUEST;
    ReturnCode      res;
    uint16_t        nbRead;
    uint16_t        blockLen;
    uint16_t        startBlock;
    uint16_t        startAddr;
    const uint8_t*  wrbuf      = buf;
    uint32_t        currentLen = len;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (len == 0U) || (ctx->subCtx.t5t.blockLen == 0U))
    {
        return ERR_PARAM;
    }
    blockLen = (uint16_t)ctx->subCtx.t5t.blockLen;
    if( blockLen == 0U )
    {
        return ERR_SYSTEM;
    }
    startBlock = (uint16_t) (offset     / blockLen);
    startAddr  = (uint16_t) (startBlock * blockLen);

    if (startAddr != offset)
    {
        /* Unaligned start offset must read the first block before */
        res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
        if ( (res == ERR_NONE) && (ctx->subCtx.t5t.txrxBuf[0U] == 0U) && (nbRead > 0U) )
        {
            nbRead = (uint16_t) ((uint32_t)nbRead - 1U  + startAddr - offset);
            if (nbRead > (uint32_t) currentLen)
            {
                nbRead = (uint16_t) currentLen;
            }
            if (nbRead > 0U)
            {
                (void)ST_MEMCPY(&ctx->subCtx.t5t.txrxBuf[1U - startAddr + (uint16_t)offset], wrbuf, nbRead);
            }
            res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, &ctx->subCtx.t5t.txrxBuf[1U]);
            if (res != ERR_NONE)
            {
                return res;
            }
        }
        else
        {
            if (res != ERR_NONE)
            {
                result = res;
            }
            else
            {
                result = ERR_PARAM;
            }
            return result;
        }
        currentLen -= nbRead;
        wrbuf       = &wrbuf[nbRead];
        startBlock++;
    }
    while (currentLen >= blockLen)
    {
        res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, wrbuf);
        if (res == ERR_NONE)
        {
            currentLen -= blockLen;
            wrbuf       = &wrbuf[blockLen];
            startBlock++;
        }
        else
        {
            result = res;
            break;
        }
    }
    if ( (currentLen != 0U) && (currentLen < blockLen) )
    {
        /* Unaligned end, must read the first block before */
        res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &nbRead);
        if ( (res == ERR_NONE) && (ctx->subCtx.t5t.txrxBuf[0U] == 0U) && (nbRead > 0U) )
        {
            /* MISRA: PRQA requires to check the length to copy, IAR doesn't */
            if (currentLen > 0U)
            {
                (void)ST_MEMCPY(&ctx->subCtx.t5t.txrxBuf[1U], wrbuf, currentLen);
            }
            res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, &ctx->subCtx.t5t.txrxBuf[1U]);
            if (res != ERR_NONE)
            {
                result = res;
            }
            else
            {
                currentLen = 0U;
            }
        }
        else
        {
            if (res != ERR_NONE)
            {
                result = res;
            }
            else
            {
                result = ERR_PARAM;
            }
            return result;
        }
    }
    if (currentLen == 0U)
    {
        result = ERR_NONE;
    }
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen)
{
    uint8_t    TLV[8U];
    ReturnCode result;
    uint8_t    len;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device))
    {
        return ERR_PARAM;
    }

    if( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        return ERR_WRONG_STATE;
    }

    if( (rawMessageLen != 0U) && ((ctx->messageOffset + rawMessageLen) < ctx->areaLen) )
    {
        /* Write T5T TLV terminator */
        len = 0U;
        TLV[len] = NDEF_TERMINATOR_TLV_T; /* TLV terminator */
        len++;
        result = ndefT5TPollerWriteBytes(ctx, ctx->messageOffset + rawMessageLen, TLV, len);
        if (result != ERR_NONE)
        {
            return result;
        }
    }

    len = 0U;
    TLV[len] = NDEF_T5T_TLV_NDEF;
    len++;
    if (rawMessageLen <= NDEF_SHORT_VFIELD_MAX_LEN)
    {
        TLV[len] = (uint8_t) rawMessageLen;
        len++;
    }
    else
    {
        TLV[len] = (uint8_t)(NDEF_SHORT_VFIELD_MAX_LEN + 1U);
        len++;        
        TLV[len] = (uint8_t) (rawMessageLen >> 8U);
        len++;
        TLV[len] = (uint8_t) rawMessageLen;
        len++;
    }
    if (rawMessageLen == 0U)
    {
        TLV[len] = NDEF_TERMINATOR_TLV_T; /* TLV terminator */
        len++;
    }

    result = ndefT5TPollerWriteBytes(ctx, ctx->subCtx.t5t.TlvNDEFOffset, TLV, len);

    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen)
{
    uint32_t   len = bufLen;
    ReturnCode result;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    /* TS T5T v1.0 7.5.3.1/2: T5T NDEF Detect should have been called before NDEF write procedure */
    /* Warning: current tag content must not be changed between NDEF Detect procedure and NDEF Write procedure*/

    /* TS T5T v1.0 7.5.3.3: check write access condition */
    if ( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        /* Conclude procedure */
        return ERR_WRONG_STATE;
    }

    /* TS T5T v1.0 7.5.3.3: verify available space */
    result = ndefT5TPollerCheckAvailableSpace(ctx, bufLen);
    if( result != ERR_NONE )
    {
        /* Conclude procedure */
        return ERR_PARAM;
    }
    /* TS T5T v1.0 7.5.3.4: reset L-Field to 0 */
    /* and update ctx->messageOffset according to L-field len */
    result = ndefT5TPollerBeginWriteMessage(ctx, bufLen);
    if (result != ERR_NONE)
    {
        ctx->state = NDEF_STATE_INVALID;
        /* Conclude procedure */
        return result;
    }
    if( bufLen != 0U )
    {
        /* TS T5T v1.0 7.5.3.5: write new NDEF message */
        result = ndefT5TPollerWriteBytes(ctx, ctx->messageOffset, buf, len);
        if (result != ERR_NONE)
        {
            /* Conclude procedure */
            ctx->state = NDEF_STATE_INVALID;
            return result;
        }
        /* TS T5T v1.0 7.5.3.6 & 7.5.3.7: update L-Field and write Terminator TLV */
        result = ndefT5TPollerEndWriteMessage(ctx, len);
        if (result != ERR_NONE)
        {
            /* Conclude procedure */
            ctx->state = NDEF_STATE_INVALID;
            return result;
        }
    }
    return result;
}

/*******************************************************************************/
static ReturnCode ndefT5TWriteCC(ndefContext *ctx)
{
    ReturnCode  ret;
    uint8_t*    buf;
    uint8_t     dataIt;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    buf    = ctx->ccBuf;
    dataIt = 0U;
    /* Encode CC */
    buf[dataIt] = ctx->cc.t5t.magicNumber;                                                                /* Byte 0 */
    dataIt++;
    buf[dataIt] = (uint8_t)(((ctx->cc.t5t.majorVersion  & 0x03U) << 6) |                                  /* Byte 1 */
                            ((ctx->cc.t5t.minorVersion  & 0x03U) << 4) |                                  /*        */
                            ((ctx->cc.t5t.readAccess    & 0x03U) << 2) |                                  /*        */
                            ((ctx->cc.t5t.writeAccess   & 0x03U) << 0));                                  /*        */
    dataIt++;
    buf[dataIt] = (ctx->cc.t5t.ccLen == NDEF_T5T_CC_LEN_8_BYTES) ? 0U : (uint8_t)ctx->cc.t5t.memoryLen;   /* Byte 2 */
    dataIt++;
    buf[dataIt]   = 0U;                                                                                   /* Byte 3 */
    if( ctx->cc.t5t.multipleBlockRead ) { buf[dataIt] |= 0x01U; }                                         /* Byte 3  b0 MBREAD                */
    if( ctx->cc.t5t.mlenOverflow )      { buf[dataIt] |= 0x04U; }                                         /* Byte 3  b2 Android MLEN overflow */
    if( ctx->cc.t5t.lockBlock )         { buf[dataIt] |= 0x08U; }                                         /* Byte 3  b3 Lock Block            */
    if( ctx->cc.t5t.specialFrame )      { buf[dataIt] |= 0x10U; }                                         /* Byte 3  b4 Special Frame         */
    dataIt++;
    if( ctx->cc.t5t.ccLen == NDEF_T5T_CC_LEN_8_BYTES )
    {
        buf[dataIt] = 0U;                                                                                 /* Byte 4 */
        dataIt++;
        buf[dataIt] = 0U;                                                                                 /* Byte 5 */
        dataIt++;
        buf[dataIt] = (uint8_t)(ctx->cc.t5t.memoryLen >> 8);                                              /* Byte 6 */
        dataIt++;
        buf[dataIt] = (uint8_t)(ctx->cc.t5t.memoryLen >> 0);                                              /* Byte 7 */
        dataIt++;
    }

    ret = ndefT5TPollerWriteBytes(ctx, 0U, buf, ctx->cc.t5t.ccLen);
    return ret;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
    uint16_t                 rcvdLen;
    ReturnCode               result;
    static const uint8_t     emptyNDEF[] = { 0x03U, 0x00U, 0xFEU, 0x00U};

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    /* Reset previous potential info about NDEF messages */
    ctx->messageLen               = 0U;
    ctx->messageOffset            = 0U;
    ctx->subCtx.t5t.TlvNDEFOffset = 0U;

    if( cc != NULL )
    {
        if( (cc->t5t.ccLen != NDEF_T5T_CC_LEN_8_BYTES) && (cc->t5t.ccLen != NDEF_T5T_CC_LEN_4_BYTES) )
        {
            return ERR_PARAM;
        }
        (void)ST_MEMCPY(&ctx->cc, cc, sizeof(ndefCapabilityContainer));
    }
    else
    {
        /* Try to find the appropriate cc values */
        ctx->cc.t5t.magicNumber  = NDEF_T5T_CC_MAGIC_1_BYTE_ADDR_MODE; /* E1 */
        ctx->cc.t5t.majorVersion = ndefT5TMajorVersion(NDEF_T5T_MAPPING_VERSION_1_0);
        ctx->cc.t5t.minorVersion = 0U;
        ctx->cc.t5t.readAccess   = 0U;
        ctx->cc.t5t.writeAccess  = 0U;
        ctx->cc.t5t.lockBlock    = false;
        ctx->cc.t5t.specialFrame = false;
        ctx->cc.t5t.memoryLen    = 0U;
        ctx->cc.t5t.mlenOverflow = false;

        /* Autodetect the Multiple Block Read feature (CC Byte 3 b0: MBREAD) */
        result = ndefT5TPollerReadMultipleBlocks(ctx, 0U, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvdLen);
        ctx->cc.t5t.multipleBlockRead = (result == ERR_NONE) ? true : false;

        /* Try to retrieve the tag's size using getSystemInfo and GetExtSystemInfo */

        if( (ctx->subCtx.t5t.sysInfoSupported == true) && (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) )
        {
            ctx->cc.t5t.memoryLen = (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER);

            if( (options & NDEF_T5T_FORMAT_OPTION_NFC_FORUM) == NDEF_T5T_FORMAT_OPTION_NFC_FORUM ) /* NFC Forum format */
            {
                if( ctx->cc.t5t.memoryLen >= NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING )
                {
                    ctx->cc.t5t.ccLen = NDEF_T5T_CC_LEN_8_BYTES;
                }
                if( ctx->cc.t5t.memoryLen > 0U )
                {
                    ctx->cc.t5t.memoryLen--; /* remove CC area from memory length */
                }
            }
            else /* Android format */
            {
                ctx->cc.t5t.ccLen = NDEF_T5T_CC_LEN_4_BYTES;
                 if( ctx->cc.t5t.memoryLen >= NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING )
                {
                    ctx->cc.t5t.mlenOverflow = true;
                    ctx->cc.t5t.memoryLen    = 0xFFU;
                }
            }

            if( !ctx->subCtx.t5t.legacySTHighDensity && (ctx->subCtx.t5t.sysInfo.numberOfBlock > NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR) )
            {
                ctx->cc.t5t.magicNumber = NDEF_T5T_CC_MAGIC_2_BYTE_ADDR_MODE; /* E2 */
            }
        }
        else
        {
            return ERR_REQUEST;
        }
    }

    result = ndefT5TWriteCC(ctx);
    if( result != ERR_NONE )
    {
        /* If write fails, try to use special frame if not yet used */
        if( !ctx->cc.t5t.specialFrame )
        {
            platformDelay(20U); /* Wait to be sure that previous command has ended */
            ctx->cc.t5t.specialFrame = true; /* Add option flag */
            result = ndefT5TWriteCC(ctx);
            if( result != ERR_NONE )
            {
                ctx->cc.t5t.specialFrame = false; /* Add option flag */
                return result;
            }
        }
        else
        {
           return result;
        }
    }

    /* Update info about current NDEF */

    ctx->subCtx.t5t.TlvNDEFOffset = ctx->cc.t5t.ccLen;

    result = ndefT5TPollerWriteBytes(ctx, ctx->subCtx.t5t.TlvNDEFOffset, emptyNDEF, sizeof(emptyNDEF) );
    if (result == ERR_NONE)
    {
        /* Update info about current NDEF */
        ctx->messageOffset = (uint32_t)ctx->cc.t5t.ccLen + NDEF_T5T_TLV_T_LEN + NDEF_T5T_TLV_L_1_BYTES_LEN;
        ctx->state         = NDEF_STATE_INITIALIZED;
    }
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerCheckPresence(ndefContext *ctx)
{
    ReturnCode          ret;
    uint16_t            blockAddr;
    uint16_t            rcvLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    ndefT5TInvalidateCache(ctx);

    blockAddr = 0U;

    ret = ndefT5TPollerReadSingleBlock(ctx, blockAddr, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen);

    return ret;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
    uint32_t lLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if ( ctx->state == NDEF_STATE_INVALID )
    {
        return ERR_WRONG_STATE;
    }

    lLen = ( messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T5T_TLV_L_3_BYTES_LEN : NDEF_T5T_TLV_L_1_BYTES_LEN;

    if( (messageLen + ctx->subCtx.t5t.TlvNDEFOffset + NDEF_T5T_TLV_T_LEN + lLen) > (ctx->areaLen + ctx->cc.t5t.ccLen) )
    {
        return ERR_NOMEM;
    }
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    ReturnCode ret;
    uint32_t   lLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        return ERR_WRONG_STATE;
    }

    /* TS T5T v1.0 7.5.3.4: reset L-Field to 0 */
    ret = ndefT5TPollerWriteRawMessageLen(ctx, 0U);
    if( ret != ERR_NONE )
    {
        /* Conclude procedure */
        ctx->state = NDEF_STATE_INVALID;
        return ret;
    }

    lLen                = ( messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T5T_TLV_L_3_BYTES_LEN : NDEF_T5T_TLV_L_1_BYTES_LEN;
    ctx->messageOffset  = ctx->subCtx.t5t.TlvNDEFOffset;
    ctx->messageOffset += NDEF_T5T_TLV_T_LEN; /* T Length */
    ctx->messageOffset += lLen;               /* L Length */
    ctx->state          = NDEF_STATE_INITIALIZED;

    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerSetReadOnly(ndefContext *ctx)
{
    ReturnCode ret;
    uint32_t   numBlocks;
    uint16_t   i;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( ctx->state != NDEF_STATE_READWRITE )
    {
        return ERR_WRONG_STATE;
    }

    ctx->cc.t5t.writeAccess = NDEF_T5T_WR_ACCESS_NEVER;
    ret = ndefT5TWriteCC(ctx);
    if( ret != ERR_NONE )
    {
        return ret;
    }

    ctx->state = NDEF_STATE_READONLY;
    numBlocks = (ctx->areaLen + (uint32_t)ctx->cc.t5t.ccLen)/(uint32_t)ctx->subCtx.t5t.blockLen;
    if( ctx->cc.t5t.lockBlock && !ctx->subCtx.t5t.legacySTHighDensity )
    {
        for( i = 0; i < numBlocks; i++ )
        {
            ret = ndefT5TPollerLockSingleBlock(ctx, i);
            if( ret != ERR_NONE )
            {
                return ret;
            }
        }
    }
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    ReturnCode ret;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( ctx->state != NDEF_STATE_INITIALIZED )
    {
        return ERR_WRONG_STATE;
    }

    /* TS T5T v1.0 7.5.3.6 & 7.5.3.7: update L-Field and write Terminator TLV */
    ret = ndefT5TPollerWriteRawMessageLen(ctx, messageLen);
    if( ret != ERR_NONE )
    {
        /* Conclude procedure */
        ctx->state = NDEF_STATE_INVALID;
        return ret;
    }
    ctx->messageLen = messageLen;
    ctx->state      = (ctx->messageLen == 0U) ? NDEF_STATE_INITIALIZED : NDEF_STATE_READWRITE;
    return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT5TPollerWriteSingleBlock(ndefContext *ctx, uint16_t blockNum, const uint8_t* wrData)
{
    ReturnCode                ret;
    uint8_t                   flags;
    const uint8_t*            uid;
    uint32_t                  retry;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    uid   = ctx->subCtx.t5t.uid;
    flags = ctx->subCtx.t5t.flags;
    if (ctx->cc.t5t.specialFrame)
    {
        flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_OPTION;
    }

    ndefT5TInvalidateCache(ctx);

    retry = NDEF_T5T_N_RETRY_ERROR;
    do
    {
        if( ctx->subCtx.t5t.legacySTHighDensity )
        {
            ret = rfalST25xVPollerM24LRWriteSingleBlock(flags, uid, blockNum, wrData, ctx->subCtx.t5t.blockLen);
        }
        else
        {
            if( blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR )
            {
                ret = rfalNfcvPollerWriteSingleBlock(flags, uid, (uint8_t)blockNum, wrData, ctx->subCtx.t5t.blockLen);
            }
            else
            {
                ret = rfalNfcvPollerExtendedWriteSingleBlock(flags, uid, blockNum, wrData, ctx->subCtx.t5t.blockLen);
            }
        }
    }
    while( (retry-- != 0U) && ndefT5TIsTransmissionError(ret) );

    return ret;
}

/*******************************************************************************/
static ReturnCode ndefT5TPollerLockSingleBlock(ndefContext *ctx, uint16_t blockNum)
{
    ReturnCode                ret;
    uint8_t                   flags;
    const uint8_t*            uid;
    uint32_t                  retry;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    uid   = ctx->subCtx.t5t.uid;
    flags = ctx->subCtx.t5t.flags;
    if (ctx->cc.t5t.specialFrame)
    {
        flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_OPTION;
    }

    retry = NDEF_T5T_N_RETRY_ERROR;
    do
    {
        if( blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR )
        {
            ret = rfalNfcvPollerLockBlock(flags, uid, (uint8_t)blockNum);
        }
        else
        {
            ret = rfalNfcvPollerExtendedLockSingleBlock(flags, uid, blockNum);
        }
    }
    while( (retry-- != 0U) && ndefT5TIsTransmissionError(ret) );

    return ret;
}

#endif /* NDEF_FEATURE_FULL_API */

/*******************************************************************************/
static ReturnCode ndefT5TPollerReadSingleBlock(ndefContext *ctx, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
    ReturnCode                ret;
    uint8_t                   flags;
    const uint8_t*            uid;
    uint32_t                  retry;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (rxBuf == NULL) || (rcvLen == NULL) )
    {
        return ERR_PARAM;
    }

    if( ndefT5TIsValidCache(ctx, blockNum) )
    {
        /* Retrieve data from cache */
        (void)ST_MEMCPY(rxBuf, ctx->subCtx.t5t.cacheBuf, NDEF_T5T_TxRx_BUFF_HEADER_SIZE + (uint32_t)ctx->subCtx.t5t.blockLen);
        *rcvLen = (uint16_t)NDEF_T5T_TxRx_BUFF_HEADER_SIZE + ctx->subCtx.t5t.blockLen;

        return ERR_NONE;
    }

    flags = ctx->subCtx.t5t.flags;
    uid   = ctx->subCtx.t5t.uid;

    retry = NDEF_T5T_N_RETRY_ERROR;
    do
    {
        if( ctx->subCtx.t5t.legacySTHighDensity )
        {
            ret = rfalST25xVPollerM24LRReadSingleBlock(flags, uid, blockNum, rxBuf, rxBufLen, rcvLen);
        }
        else
        {
            if( blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR )
            {
                ret = rfalNfcvPollerReadSingleBlock(flags, uid, (uint8_t)blockNum, rxBuf, rxBufLen, rcvLen);
            }
            else
            {
                ret = rfalNfcvPollerExtendedReadSingleBlock(flags, uid, blockNum, rxBuf, rxBufLen, rcvLen);
            }
        }
    }
    while( (retry-- != 0U) && ndefT5TIsTransmissionError(ret) );

    if( ret == ERR_NONE )
    {
        /* Update cache */
        if( *rcvLen > 0U )
        {
            (void)ST_MEMCPY(ctx->subCtx.t5t.cacheBuf, rxBuf, *rcvLen);
        }
        ctx->subCtx.t5t.cacheBlock = blockNum;
    }

    return ret;
}

/*******************************************************************************/
static ReturnCode ndefT5TPollerReadMultipleBlocks(ndefContext *ctx, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
    ReturnCode                ret;
    uint8_t                   flags;
    const uint8_t*            uid;
    uint32_t                  retry;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    uid   = ctx->subCtx.t5t.uid;
    flags = ctx->subCtx.t5t.flags;

    /* 5.5 The number of data blocks returned by the Type 5 Tag in its response is (NB +1)
       e.g. NumOfBlocks = 0 means reading 1 block */

    retry = NDEF_T5T_N_RETRY_ERROR;
    do
    {
        if( ctx->subCtx.t5t.legacySTHighDensity )
        {
            ret = rfalST25xVPollerM24LRReadMultipleBlocks(flags, uid, firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
        }
        else
        {
            if( firstBlockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR )
            {
                ret = rfalNfcvPollerReadMultipleBlocks(flags, uid, (uint8_t)firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
            }
            else
            {
                ret = rfalNfcvPollerExtendedReadMultipleBlocks(flags, uid, firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
            }
        }
    }
    while( (retry-- != 0U) && ndefT5TIsTransmissionError(ret) );

    return ret;
}

#if !defined NDEF_SKIP_T5T_SYS_INFO
/*******************************************************************************/
static ReturnCode ndefT5TGetSystemInformation(ndefContext *ctx, bool extended)
{
    ReturnCode                ret;
    uint8_t                   rxBuf[NDEF_T5T_SYSINFO_MAX_LEN];
    uint16_t                  rcvLen;
    uint8_t*                  resp;
    uint8_t                   flags;
    const uint8_t*            uid;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    flags = ctx->subCtx.t5t.flags;
    uid   = ctx->subCtx.t5t.uid;

    if( extended )
    {
        ret = rfalNfcvPollerExtendedGetSystemInformation(flags, uid, (uint8_t)RFAL_NFCV_SYSINFO_REQ_ALL, rxBuf, (uint16_t)sizeof(rxBuf), &rcvLen);
    }
    else
    {
        if (ctx->subCtx.t5t.legacySTHighDensity)
        {
            flags |= (uint8_t)RFAL_NFCV_REQ_FLAG_PROTOCOL_EXT;
        }
        ret = rfalNfcvPollerGetSystemInformation(flags, uid, rxBuf, (uint16_t)sizeof(rxBuf), &rcvLen);
    }

    if( ret != ERR_NONE )
    {
        return ret;
    }

    resp = &rxBuf[0U];
    /* skip Flags */
    resp++;
    /* get Info flags */
    ctx->subCtx.t5t.sysInfo.infoFlags = *resp;
    resp++;
    if( extended && (ndefT5TSysInfoLenValue(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) )
    {
        return ERR_PROTO;
    }
    /* get UID */
    (void)ST_MEMCPY(ctx->subCtx.t5t.sysInfo.UID, resp, RFAL_NFCV_UID_LEN);
    resp = &resp[RFAL_NFCV_UID_LEN];
    if( ndefT5TSysInfoDFSIDPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U )
    {
        ctx->subCtx.t5t.sysInfo.DFSID = *resp;
        resp++;
    }
    if( ndefT5TSysInfoAFIPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U )
    {
        ctx->subCtx.t5t.sysInfo.AFI = *resp;
        resp++;
    }
    if( ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U )
    {
        if ( ctx->subCtx.t5t.legacySTHighDensity || extended )
        {
            /* LRIS64K/M24LR16/M24LR64 */
            ctx->subCtx.t5t.sysInfo.numberOfBlock = *resp;
            resp++;
            ctx->subCtx.t5t.sysInfo.numberOfBlock |= (((uint16_t)*resp) << 8U);
            resp++;
        }
        else
        {
            ctx->subCtx.t5t.sysInfo.numberOfBlock = *resp;
            resp++;
        }
        ctx->subCtx.t5t.sysInfo.blockSize = *resp;
        resp++;
        /* Add 1 to get real values*/
        ctx->subCtx.t5t.sysInfo.numberOfBlock++;
        ctx->subCtx.t5t.sysInfo.blockSize++;
    }
    if( ndefT5TSysInfoICRefPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U )
    {
        ctx->subCtx.t5t.sysInfo.ICRef = *resp;
        resp++;
    }
    if( extended && (ndefT5TSysInfoCmdListPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) )
    {
        ctx->subCtx.t5t.sysInfo.supportedCmd[0U] = *resp;
        resp++;
        ctx->subCtx.t5t.sysInfo.supportedCmd[1U] = *resp;
        resp++;
        ctx->subCtx.t5t.sysInfo.supportedCmd[2U] = *resp;
        resp++;
        ctx->subCtx.t5t.sysInfo.supportedCmd[3U] = *resp;
        resp++;
    }
    return ERR_NONE;
}
#endif /* NDEF_SKIP_T5T_SYS_INFO */

#endif /* RFAL_FEATURE_NFCV */
