/*************************************************************************/
/* module:         XML/WBXML scanner                                     */
/* file:           XLTDecCom.h                                           */
/* target system:  all                                                   */
/* target OS:      all                                                   */
/*************************************************************************/

/*
 * Copyright Notice
 * Copyright (c) Ericsson, IBM, Lotus, Matsushita Communication 
 * Industrial Co., Ltd., Motorola, Nokia, Openwave Systems, Inc., 
 * Palm, Inc., Psion, Starfish Software, Symbian, Ltd. (2001).
 * All Rights Reserved.
 * Implementation of all or part of any Specification may require 
 * licenses under third party intellectual property rights, 
 * including without limitation, patent rights (such a third party 
 * may or may not be a Supporter). The Sponsors of the Specification 
 * are not responsible and shall not be held responsible in any 
 * manner for identifying or failing to identify any or all such 
 * third party intellectual property rights.
 * 
 * THIS DOCUMENT AND THE INFORMATION CONTAINED HEREIN ARE PROVIDED 
 * ON AN "AS IS" BASIS WITHOUT WARRANTY OF ANY KIND AND ERICSSON, IBM, 
 * LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO. LTD, MOTOROLA, 
 * NOKIA, PALM INC., PSION, STARFISH SOFTWARE AND ALL OTHER SYNCML 
 * SPONSORS DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION 
 * HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF 
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
 * SHALL ERICSSON, IBM, LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO., 
 * LTD, MOTOROLA, NOKIA, PALM INC., PSION, STARFISH SOFTWARE OR ANY 
 * OTHER SYNCML SPONSOR BE LIABLE TO ANY PARTY FOR ANY LOSS OF 
 * PROFITS, LOSS OF BUSINESS, LOSS OF USE OF DATA, INTERRUPTION OF 
 * BUSINESS, OR FOR DIRECT, INDIRECT, SPECIAL OR EXEMPLARY, INCIDENTAL, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES OF ANY KIND IN CONNECTION WITH 
 * THIS DOCUMENT OR THE INFORMATION CONTAINED HEREIN, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH LOSS OR DAMAGE.
 * 
 * The above notice and this paragraph must be included on all copies 
 * of this document that are made.
 * 
 */
/**
 * Common header file for the WBXML and the XML scanner.
 */

/*************************************************************************/
/* Definitions                                                           */
/*************************************************************************/
#ifndef _XLT_DEC_COM_H
#define _XLT_DEC_COM_H

#include <smldef.h>
#include <smldtd.h>
#include "xlttags.h"

#define DEC_ID          XLT_DEC_ID + 0
#define WBXML_ID        XLT_DEC_ID + 200
#define XML_ID          XLT_DEC_ID + 400
#define STACK_ID        XLT_DEC_ID + 600

#ifdef _cplusplus
extern "C" {
#endif


/* Token types returned by the scanner */
typedef enum {
    TOK_UNDEF = 0,
    TOK_TAG_START,
    TOK_TAG_END,
    TOK_TAG_EMPTY,
    TOK_CONT
} XltTokType_t;

/**
 * Struct containing a token returned by the scanner
 */
typedef struct
{
    XltTokType_t type;       /* XLT_TOK_TAG_START, etc. */
    XltTagID_t tagid;        /* TN_SYNCHDR, etc. - valid for token of type
				XLT_TOK_TAG_xxx */
    SmlPcdataExtension_t ext;   /* type of extension this tag belongs to -
				valid for token of type XLT_TOK_TAG_xxx */
    SmlPcdataPtr_t pcdata;      /* valid for token of type XLT_TOK_CONT_xxx */
    MemPtr_t start;          /* pointer to the start of this token within
				the document - needed for pushback */
} XltDecToken_t, *XltDecTokenPtr_t;

/**
 * Public interface for the XML/WBXML scanner components.
 *
 * The struct used for holding the XML and WBXML scanner state information
 * are the same - function pointers are used to map the "objects" public
 * methods to the right functions for scanning XML and WBXML documents.
 * This object-oriented interface enables the parser component to use the
 * two scanners interchangeably.
 * The interface contains serveral public methods and attributes:
 *
 * FUNCTION: nextTok
 *
 * Decodes the next valid token at the current position within the
 * document. Information about this token is placed into the curtok
 * attribute of the scanner object.
 *
 * PRE-Condition:
 * 
 * POST-Condition:
 *                 curtok contains the last valid token.
 *
 * IN:
 *
 * OUT:
 *
 * IN/OUT:         the scanner
 *
 * RETURNS:        SML_ERR_OK or an appropriate error code
 *
 * 
 * FUNCTION: destroy
 *
* Free the memory allocated by the scanner.
*
* PRE-Condition:
* POST-Condition:
*
* IN:
*
* OUT:
*
* IN/OUT:         the scanner
*
* RETURNS:        SML_ERR_OK or an appropriate error code
*
*
* FUNCTION: pushTok
*
* Resets the scanner to the beginning position within the document of the
* last valid token stored in curtok. Only the last found token can be
* pushed back. It is not possible to go back more than one token.
*
* PRE-Condition:
*                 curtok contains a valid token.
*                 
* POST-Condition:
*                 the next call of nextTok will find the token that was
*                 pushed back.
*
* IN/OUT:         the scanner
*
* RETURNS:        SML_ERR_OK or an appropriate error code
*
* FUNCITON: setBuf
*
* Set the buffer the scanner works on.
*
* FUNCTION: getPos
*
* Get the current position of the scanner within the working buffer.
*
* ATTRIBUTE: curtok
*
* Contains the last valid token found by a call to nextTok.
*
* ATTRIBUTE: charset
*
* The charset information as specified in the XML/WBXML document. This is
* the IANA assigned MIBEnum value.

* ATTRIBUTE: charsetStr
*
* String representation of the charset. This attribute is valid only when
* charset equals zero. Otherwise charsetStr will be NULL.
*
* ATTRIBUTE: pubID
*
* The document public identifier as specified in the XML/WBXML document.
* This is the numeric identifier assigned by the WAP Forum. If this value
* is zero, the public ID is instead specified as a string contained in the
* pubIDStr public attribute.
*
* ATTRIBUTE: pubIDStr
*
* The string representation of the document public identifier as specified
* in the XML/WBXML document (e.g. "-//WAPFORUM//DTD WML 1.0//EN"). This
* attribute is valid only when pubID equals zero. Otherwise pubIDStr will
* be NULL.
*
* ATTRIBUTE: finished
*
* This flag is set by the nextTok method when the scanner reaches the end
* of the buffer.
*/
typedef struct XltDecScanner_s XltDecScanner_t, *XltDecScannerPtr_t;
struct XltDecScanner_s
{
    /* public methods */
    Ret_t (*nextTok)(XltDecScannerPtr_t pScanner);
    Ret_t (*destroy)(XltDecScannerPtr_t pScanner);
    Ret_t (*pushTok)(XltDecScannerPtr_t pScanner);
    void (*setBuf)(XltDecScannerPtr_t pScanner, const MemPtr_t pBufStart, const MemPtr_t pBufEnd); 
    MemPtr_t (*getPos)(XltDecScannerPtr_t pScanner); 

    /* public attributes */
    XltDecTokenPtr_t curtok;
    Long_t charset;
    String_t charsetStr;
    Long_t pubID;
    String_t pubIDStr;
    Flag_t finished;
};

/**
 * FUNCTION: xltDecWbxmlInit, xltDecXmlInit
 *
 * Initialize a new WBXML/XML scanner.
 *
 * PRE-Condition:
 *                 ppScanner is NULL
 *
 * POST-Condition:
 *                 ppScanner points to an initialized scanner status object
 *
 * IN:             pBufEnd, buffer containing the WBXML/XML document
 *
 * IN/OUT:         pBufPos, pointer to the current position within the
 *                     buffer
 *
 * OUT:            ppScanner, a new WBXML/XML scanner status object
 *
 * RETURNS:        SML_ERR_OK or an appropriate error code
 */
Ret_t xltDecWbxmlInit(const MemPtr_t pBufEnd, MemPtr_t *ppBufPos, XltDecScannerPtr_t *ppScanner);
Ret_t xltDecXmlInit(const MemPtr_t pBufEnd, MemPtr_t *ppBufPos, XltDecScannerPtr_t *ppScanner);

#ifdef _cplusplus
}
#endif

#endif
