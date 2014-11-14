/*************************************************************************/
/* module:          Definition of MetaInf DTD prototypefunctions         */
/*                  for the en-/decoder                                  */
/* file:            xltmetinf.h                                          */
/* target system:   all                                                  */
/* target OS:       all                                                  */   
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

#ifndef _XLT_METINF_H
#define _XLT_METINF_H

/* process only if we really use MetInf DTD */
#ifdef __USE_METINF__

/*************************************************************************/
/*  Definitions                                                          */
/*************************************************************************/

#include "xlttagtbl.h"
#include "xltenc.h"

#ifdef __USE_DMTND__
#define META_TYPE_TNDS_XML   "application/vnd.syncml.dmtnds+xml"
#define META_TYPE_TNDS_WBXML "application/vnd.syncml.dmtnds+wbxml"
#endif

Ret_t buildMetInfAnchorCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
Ret_t buildMetInfMetInfCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
Ret_t buildMetInfMemCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
Ret_t metinfEncBlock(XltTagID_t tagId, XltRO_t reqOptFlag, const VoidPtr_t pContent, SmlEncoding_t enc, BufferMgmtPtr_t pBufMgr, SmlPcdataExtension_t attFlag);
#endif    /* __USE_METINF__ */
#endif	  /* _XLT_METINF_H */

