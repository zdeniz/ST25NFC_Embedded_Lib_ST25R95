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
 *  \brief NDEF Empty type
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_record.h"
#include "ndef_types.h"
#include "ndef_type_flat.h"
#include "st_errno.h"
#include "utils.h"


#if NDEF_TYPE_FLAT_SUPPORT


/*
 ******************************************************************************
 * GLOBAL DEFINES
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


/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


/*
 * Flat payload record
 */


/*****************************************************************************/
static uint32_t ndefFlatPayloadTypePayloadGetLength(const ndefType* type)
{
    if ( (type == NULL) || (type->id != NDEF_TYPE_ID_FLAT) )
    {
        return 0;
    }

    return type->data.bufPayload.length;
}


/*****************************************************************************/
static const uint8_t* ndefFlatPayloadTypePayloadItem(const ndefType* type, ndefConstBuffer* bufItem, bool begin)
{
    if ( (type == NULL) || (type->id != NDEF_TYPE_ID_FLAT) )
    {
        return NULL;
    }

    if ( (begin == true) && (bufItem != NULL) )
    {
        bufItem->buffer = type->data.bufPayload.buffer;
        bufItem->length = type->data.bufPayload.length;

        return bufItem->buffer;
    }

    return NULL;
}


/*****************************************************************************/
ReturnCode ndefFlatPayloadTypeInit(ndefType* type, const ndefConstBuffer* bufPayload)
{
    if ( (type == NULL) || (bufPayload == NULL) )
    {
        return ERR_PARAM;
    }

    type->id               = NDEF_TYPE_ID_FLAT;
    type->getPayloadLength = ndefFlatPayloadTypePayloadGetLength;
    type->getPayloadItem   = ndefFlatPayloadTypePayloadItem;
    type->typeToRecord     = ndefFlatPayloadTypeToRecord;

    type->data.bufPayload.buffer = bufPayload->buffer;
    type->data.bufPayload.length = bufPayload->length;

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefGetFlatPayloadType(const ndefType* type, ndefConstBuffer* bufPayload)
{
    if ( (type     == NULL) || (type->id != NDEF_TYPE_ID_FLAT) ||
         (bufPayload == NULL) )
    {
        return ERR_PARAM;
    }

    bufPayload->buffer = type->data.bufPayload.buffer ;
    bufPayload->length = type->data.bufPayload.length ;

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordToFlatPayloadType(const ndefRecord* record, ndefType* type)
{
    const ndefType* ndefData;

    if ( (record == NULL) || (type == NULL) )
    {
        return ERR_PARAM;
    }

    ndefData = ndefRecordGetNdefType(record);
    if ( (ndefData != NULL) && (ndefData->id == NDEF_TYPE_ID_FLAT) )
    {
        (void)ST_MEMCPY(type, ndefData, sizeof(ndefType));
        return ERR_NONE;
    }

    ndefConstBuffer bufPayload;
    ReturnCode err = ndefRecordGetPayload(record, &bufPayload);
    if (err != ERR_NONE)
    {
        return err;
    }

    return ndefFlatPayloadTypeInit(type, &bufPayload);
}


/*****************************************************************************/
ReturnCode ndefFlatPayloadTypeToRecord(const ndefType* type, ndefRecord* record)
{
    if ( (type   == NULL) || (type->id != NDEF_TYPE_ID_FLAT) ||
         (record == NULL) )
    {
        return ERR_PARAM;
    }

    (void)ndefRecordReset(record);

    /* Do not initialize Type string */

    if (ndefRecordSetNdefType(record, type) != ERR_NONE)
    {
        return ERR_PARAM;
    }

    return ERR_NONE;
}

#endif
