/*************************************************************************/
/* module:          Definition of WBXML/XML tags for the en-/decoder     */
/* file:            XLTTags.c                                            */
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


#include "xlttags.h"

#include <libstr.h>
#include <smlerr.h>
#include <libmem.h>
#include <libutil.h>
#include <mgr.h>

#include "xltmetinf.h"
#include "xltdevinf.h"
#include "xlttagtbl.h"


// %%% luz:2003-07-31: added SyncML namespace tables
const char * const SyncMLNamespaces[SML_NUM_VERS] = {
  "???",
  "SYNCML:SYNCML1.0",
  "SYNCML:SYNCML1.1",
  "SYNCML:SYNCML1.2"
};

/* local prototypes */
#ifdef NOWSM
const // without WSM, the tag table is a global read-only constant
#endif
TagPtr_t getTagTable(SmlPcdataExtension_t ext);

//SmlPcdataExtension_t   getByName(String_t ns);
void freeDtdTable(DtdPtr_t tbl);

#ifdef NOWSM
const // without WSM, the DTD table is a global read-only constant
#endif
DtdPtr_t getDtdTable();


// free table obtained with getDtdTable()
void freeDtdTable(DtdPtr_t tbl)
{
  #ifndef NOWSM
  // only with WSM this is an allocated table
  smlLibFree(tbl);
  #endif
}

/**
 * FUNCTION: getDtdTable
 *
 * Returns a copy of the table containing all known (sub) dtd's
 * On error a NULL pointer is returned
 */
#ifdef NOWSM
const // without WSM, the DTD table is a global read-only constant
#endif
DtdPtr_t getDtdTable() {
  #ifdef NOWSM
  // NOWSM method, table is const, just return a pointer
	static const Dtd_t XltDtdTbl[] = {
		{ "SYNCML:SYNCML1.0", SML_EXT_UNDEFINED}, // %%% note that this is the default, will be override by syncml version specific string from 
		{ "syncml:metinf",    SML_EXT_METINF},
    { "syncml:devinf",    SML_EXT_DEVINF},
    { "syncml:dmddf1.2",    SML_EXT_DMTND},
		{ NULL,               SML_EXT_LAST}
	};
	return (DtdPtr_t)XltDtdTbl;
  #else
  // WSM method wasting a lot of memory
	DtdPtr_t _tmpPtr;

	Dtd_t XltDtdTbl[] = {
		{ "SYNCML:SYNCML1.0", SML_EXT_UNDEFINED},
		{ "syncml:metinf",    SML_EXT_METINF},
    { "syncml:devinf",    SML_EXT_DEVINF},
    { "syncml:dmddf1.2",    SML_EXT_DMTND},
		{ NULL,               SML_EXT_LAST}
	};
	_tmpPtr = NULL; 
	_tmpPtr = (DtdPtr_t)smlLibMalloc(sizeof(XltDtdTbl));
	if (_tmpPtr == NULL) return NULL;
	smlLibMemcpy(_tmpPtr, &XltDtdTbl, sizeof(XltDtdTbl));
	return _tmpPtr;
	#endif
}


/**
 * FUNCTION: getExtName
 *
 * Returns the official name for a given extention/sub-DTD
 * and stored it in 'name'. If not found name isn't modified
 */
// %%% luz:2003-04-24: added syncmlvers parameter
// %%% luz:2003-07-31: changed to vers enum
Ret_t getExtName(SmlPcdataExtension_t ext, String_t *name, SmlVersion_t vers) {
	DtdPtr_t dtdhead = getDtdTable();
	DtdPtr_t dtd = dtdhead;
	const char *dtdname;
	if (!dtdhead) return -1;
	for (;dtd->ext != SML_EXT_LAST; dtd++) {
		if (!dtd->name) continue; /* skip empty names (should not appear but better be on the safe side) */
		if (dtd->ext == ext) {
		  String_t _tmp;
		  // this is the default
		  dtdname=dtd->name;
		  // %%% luz:2003-04-24: added dynamic generation of namespace according to SyncML version
		  if (ext==SML_EXT_UNDEFINED && vers!=SML_VERS_UNDEF) {
		    // this requests SyncML namespace
                    if (vers >= SML_NUM_VERS )
                    {
                       freeDtdTable(dtdhead);
                       return SML_ERR_INVALID_SIZE;
                    }
		    dtdname=SyncMLNamespaces[vers];
		  }
			_tmp = smlLibMalloc(smlLibStrlen(dtdname)+1);
			if (!_tmp) {
				freeDtdTable(dtdhead);
				return SML_ERR_NOT_ENOUGH_SPACE;
			}
			smlLibStrcpy(_tmp, dtdname);
			freeDtdTable(dtdhead);
			*name = _tmp;			
			return SML_ERR_OK;
		}
	}
	freeDtdTable(dtdhead);
	return -1;
}

/**
 * FUNCTION: getCpByName
 *
 * Returns the codepage constant assoziated with the name stored in 'ns'
 *
 * RETURN:             a SmlPcdataExtension_t representing the corresponding codepage id.
 *                     If no corresponding codepage is found -1 is returned.
 */
SmlPcdataExtension_t getExtByName(String_t ns) {
	DtdPtr_t dtdhead = getDtdTable();
	DtdPtr_t dtd = dtdhead;
	SmlPcdataExtension_t ext = (SmlPcdataExtension_t) 255;
	if (!dtdhead) return SML_EXT_UNDEFINED;
	for (;dtd->ext != SML_EXT_LAST; dtd++) {
	  const char *dtdname=dtd->name;
		if (!dtdname) continue; /* skip empty names (should not appear but better be on the safe side) */
		if (dtd->ext==SML_EXT_UNDEFINED && smlLibStrncmp("SYNCML:SYNCML",ns,13)==0) {
		  // SyncML namespace is ok without checking version!
			ext = SML_EXT_UNDEFINED;
			break;
		}
		else if (smlLibStrcmp(dtdname,ns) == 0) {
			ext = dtd->ext;
			break;
		}
	}
	freeDtdTable(dtdhead);
	return ext;
}



/* if the commands are not defined we let the functions point to NULL */
#ifndef RESULT_RECEIVE
#define buildResults NULL
#endif

#ifndef MAP_RECEIVE
#define buildMap NULL
#endif

#ifndef EXEC_RECEIVE
#define buildExec NULL
#endif

#if !defined(ATOM_RECEIVE) && !defined(SEQUENCE_RECEIVE)
#define buildAtomOrSeq NULL
#endif

#ifndef SEARCH_RECEIVE
#define buildSearch NULL
#endif


/**
 * FUNCTION: getTagTable
 *
 * Returns the tag table - this function is used to avoid a global
 * tag table variable
 *
 * RETURN:             a pointer to the tag table containing tag ids, 
 *                     codepages, wbxml tags and xml tags
 */
/* T.K. initialized the structure via _TOKEN Macro, to take
 * out the XML name tags when not compiled with XML support.
 * In addtion removed the (unused) pointer for the build functions 
 */
#ifdef __SML_XML__
#define _TOKEN(id, wbxml, xml) (id), (wbxml), (xml)
#else
#define _TOKEN(id, wbxml, xml) (id), (wbxml), ""
#endif

#ifdef NOWSM
const // without WSM, the tag table is a global read-only constant
#endif
TagPtr_t getTagTable(SmlPcdataExtension_t ext)
{
  #ifndef NOWSM
  int mySize = 0;
  TagPtr_t _tmpTagPtr;
  SyncMLInfoPtr_t pGA = NULL;
  #else
  TagPtr_t _tmpTagPtr=NULL;
  #endif
  /* standard SyncML codepage */
  static const Tag_t syncml[] =
  {
    { _TOKEN(TN_ADD,            0x05,    "Add")},
    { _TOKEN(TN_ALERT,          0x06,    "Alert")},
    { _TOKEN(TN_ARCHIVE,        0x07,    "Archive")},
    { _TOKEN(TN_ATOMIC,         0x08,    "Atomic")},
    { _TOKEN(TN_CHAL,           0x09,    "Chal")},
    { _TOKEN(TN_CMD,            0x0A,    "Cmd")},
    { _TOKEN(TN_CMDID,          0x0B,    "CmdID")},
    { _TOKEN(TN_CMDREF,         0x0C,    "CmdRef")},
    { _TOKEN(TN_COPY,           0x0D,    "Copy")},
    { _TOKEN(TN_CRED,           0x0E,    "Cred")},
    { _TOKEN(TN_DATA,           0x0F,    "Data")},
    { _TOKEN(TN_DELETE,         0x10,    "Delete")},
    { _TOKEN(TN_EXEC,           0x11,    "Exec")},
    { _TOKEN(TN_FINAL,          0x12,    "Final")},
    { _TOKEN(TN_GET,            0x13,    "Get")},
    { _TOKEN(TN_ITEM,           0x14,    "Item")},
    { _TOKEN(TN_LANG,           0x15,    "Lang")},
    { _TOKEN(TN_LOCNAME,        0x16,    "LocName")},
    { _TOKEN(TN_LOCURI,         0x17,    "LocURI")},
    { _TOKEN(TN_MAP,            0x18,    "Map")},
    { _TOKEN(TN_MAPITEM,        0x19,    "MapItem")},
    { _TOKEN(TN_META,           0x1A,    "Meta")},
    { _TOKEN(TN_MSGID,          0x1B,    "MsgID")},
    { _TOKEN(TN_MSGREF,         0x1C,    "MsgRef")},
    { _TOKEN(TN_NORESP,         0x1D,    "NoResp")},
    { _TOKEN(TN_NORESULTS,      0x1E,    "NoResults")},
    { _TOKEN(TN_PUT,            0x1F,    "Put")},
    { _TOKEN(TN_REPLACE,        0x20,    "Replace")},
    { _TOKEN(TN_RESPURI,        0x21,    "RespURI")},
    { _TOKEN(TN_RESULTS,        0x22,    "Results")},
    { _TOKEN(TN_SEARCH,         0x23,    "Search")},
    { _TOKEN(TN_SEQUENCE,       0x24,    "Sequence")},
    { _TOKEN(TN_SESSIONID,      0x25,    "SessionID")},
    { _TOKEN(TN_SFTDEL,         0x26,    "SftDel")},
    { _TOKEN(TN_SOURCE,         0x27,    "Source")},
    { _TOKEN(TN_SOURCEREF,      0x28,    "SourceRef")},
    { _TOKEN(TN_STATUS,         0x29,    "Status")},
    { _TOKEN(TN_SYNC,           0x2A,    "Sync")},
    { _TOKEN(TN_SYNCBODY,       0x2B,    "SyncBody")},
    { _TOKEN(TN_SYNCHDR,        0x2C,    "SyncHdr")},
    { _TOKEN(TN_SYNCML,         0x2D,    "SyncML")},
    { _TOKEN(TN_TARGET,         0x2E,    "Target")},
    { _TOKEN(TN_TARGETREF,      0x2F,    "TargetRef")},
    { _TOKEN(TN_VERSION,        0x31,    "VerDTD")},
    { _TOKEN(TN_PROTO,          0x32,    "VerProto")},
    { _TOKEN(TN_NUMBEROFCHANGES,0x33,    "NumberOfChanges")},
		{ _TOKEN(TN_MOREDATA,       0x34,    "MoreData")},
	/* from version 1.2 */
    { _TOKEN(TN_CORRELATOR,          0x3C,    "Correlator")},
		
    { _TOKEN(TN_UNDEF,          0x00,    NULL)}
  };

  #ifdef __USE_METINF__
	static const Tag_t metinf[] =  {
    { _TOKEN(TN_METINF_ANCHOR,      0x05,	"Anchor")},
    { _TOKEN(TN_METINF_EMI,		      0x06,   "EMI")},
    { _TOKEN(TN_METINF_FORMAT,		  0x07,	"Format")},
    { _TOKEN(TN_METINF_FREEID,		  0x08,	"FreeID")},
    { _TOKEN(TN_METINF_FREEMEM,	    0x09,	"FreeMem")},
    { _TOKEN(TN_METINF_LAST,     	  0x0A,	"Last")},
    { _TOKEN(TN_METINF_MARK,		    0x0B,	"Mark")},
    { _TOKEN(TN_METINF_MAXMSGSIZE,  0x0C,	"MaxMsgSize")},
    { _TOKEN(TN_METINF_MEM,		      0x0D,	"Mem")},
    { _TOKEN(TN_METINF_METINF,		  0x0E,	"MetInf")},
    { _TOKEN(TN_METINF_NEXT,		    0x0F,	"Next")},
    { _TOKEN(TN_METINF_NEXTNONCE,	  0x10,	"NextNonce")},
    { _TOKEN(TN_METINF_SHAREDMEM,	  0x11,	"SharedMem")},
    { _TOKEN(TN_METINF_SIZE,		    0x12,	"Size")},
    { _TOKEN(TN_METINF_TYPE,		    0x13,	"Type")},
    { _TOKEN(TN_METINF_VERSION,	    0x14,	"Version")},
		/* SCTSTK - 18/03/2002, S.H. 2002-04-05 : SyncML 1.1 */
    { _TOKEN(TN_METINF_MAXOBJSIZE,	0x15,	"MaxObjSize")},
    { _TOKEN(TN_UNDEF,				      0x00,	NULL)}
	};
  #endif


  #ifdef __USE_DEVINF__
  static const Tag_t devinf[] = {
    {_TOKEN(TN_DEVINF_CTCAP,       0x05,   "CTCap")},
    {_TOKEN(TN_DEVINF_CTTYPE,      0x06,   "CTType")},
    {_TOKEN(TN_DEVINF_DATASTORE,   0x07,   "DataStore")},
    {_TOKEN(TN_DEVINF_DATATYPE,    0x08,   "DataType")},
    {_TOKEN(TN_DEVINF_DEVID,       0x09,   "DevID")},
    {_TOKEN(TN_DEVINF_DEVINF,      0x0A,   "DevInf")},
    {_TOKEN(TN_DEVINF_DEVTYP,      0x0B,   "DevTyp")},
    {_TOKEN(TN_DEVINF_DISPLAYNAME, 0x0C,   "DisplayName")},
    {_TOKEN(TN_DEVINF_DSMEM,       0x0D,   "DSMem")},
    {_TOKEN(TN_DEVINF_EXT,         0x0E,   "Ext")},
    {_TOKEN(TN_DEVINF_FWV,         0x0F,   "FwV")},
    {_TOKEN(TN_DEVINF_HWV,         0x10,   "HwV")},
    {_TOKEN(TN_DEVINF_MAN,         0x11,   "Man")},
    {_TOKEN(TN_DEVINF_MAXGUIDSIZE, 0x12,   "MaxGUIDSize")},
    {_TOKEN(TN_DEVINF_MAXID,       0x13,   "MaxID")},
    {_TOKEN(TN_DEVINF_MAXMEM,      0x14,   "MaxMem")},
    {_TOKEN(TN_DEVINF_MOD,         0x15,   "Mod")},
    {_TOKEN(TN_DEVINF_OEM,         0x16,   "OEM")},
    {_TOKEN(TN_DEVINF_PARAMNAME,   0x17,   "ParamName")},
    {_TOKEN(TN_DEVINF_PROPNAME,    0x18,   "PropName")},
    {_TOKEN(TN_DEVINF_RX,          0x19,   "Rx")},
    {_TOKEN(TN_DEVINF_RXPREF,      0x1A,   "Rx-Pref")},
    {_TOKEN(TN_DEVINF_SHAREDMEM,   0x1B,   "SharedMem")},
    {_TOKEN(TN_DEVINF_SIZE,        0x1C,   "Size")},
    {_TOKEN(TN_DEVINF_SOURCEREF,   0x1D,   "SourceRef")},
    {_TOKEN(TN_DEVINF_SWV,         0x1E,   "SwV")},
    {_TOKEN(TN_DEVINF_SYNCCAP,     0x1F,   "SyncCap")},
    {_TOKEN(TN_DEVINF_SYNCTYPE,    0x20,   "SyncType")},
    {_TOKEN(TN_DEVINF_TX,          0x21,   "Tx")},
    {_TOKEN(TN_DEVINF_TXPREF,      0x22,   "Tx-Pref")},
    {_TOKEN(TN_DEVINF_VALENUM,     0x23,   "ValEnum")},
    {_TOKEN(TN_DEVINF_VERCT,       0x24,   "VerCT")},
    {_TOKEN(TN_DEVINF_VERDTD,      0x25,   "VerDTD")},
    {_TOKEN(TN_DEVINF_XNAM,        0x26,   "XNam")},
    {_TOKEN(TN_DEVINF_XVAL,        0x27,   "XVal")},
    // %%% luz:2003-04-28 : added these, they were missing
    {_TOKEN(TN_DEVINF_UTC,         0x28,   "UTC")},
    {_TOKEN(TN_DEVINF_NOFM,        0x29,   "SupportNumberOfChanges")},
    {_TOKEN(TN_DEVINF_LARGEOBJECT, 0x2A,   "SupportLargeObjs")},
    // %%% end luz    
    { _TOKEN(TN_UNDEF,			       0x00,	NULL)}
  };
  #endif

  #ifdef __USE_DMTND__
  static const Tag_t dmtnd[] =  {
    { _TOKEN(TN_DMTND_AccessType,   0x05,     "AccessType")}, 
    { _TOKEN(TN_DMTND_ACL,          0x06,     "ACL")},
    { _TOKEN(TN_DMTND_Add,          0x07,     "Add")},
    { _TOKEN(TN_DMTND_b64,          0x08,     "b64")},
    { _TOKEN(TN_DMTND_bin,          0x09,     "bin")},
    { _TOKEN(TN_DMTND_bool,         0x0A,     "bool")},
    { _TOKEN(TN_DMTND_chr,          0x0B,     "chr")},
    { _TOKEN(TN_DMTND_CaseSense,    0x0C,     "CaseSense")},
    { _TOKEN(TN_DMTND_CIS,          0x0D,     "CIS")},
    { _TOKEN(TN_DMTND_Copy,         0x0E,     "Copy")},
    { _TOKEN(TN_DMTND_CS,           0x0F,     "CS")},
    { _TOKEN(TN_DMTND_date,         0x10,     "date")},
    { _TOKEN(TN_DMTND_DDFName,      0x11,     "DDFName")},
    { _TOKEN(TN_DMTND_DefaultValue, 0x12,     "DefaultValue")},
    { _TOKEN(TN_DMTND_Delete,       0x13,     "Delete")},
    { _TOKEN(TN_DMTND_Description,  0x14,     "Description")},
    { _TOKEN(TN_DMTND_DFFormat,     0x15,     "DFFormat")},
    { _TOKEN(TN_DMTND_DFProperties, 0x16,     "DFProperties")},  
    { _TOKEN(TN_DMTND_DFTitle,      0x17,     "DFTitle")},
    { _TOKEN(TN_DMTND_DFType,       0x18,     "DFType")},
    { _TOKEN(TN_DMTND_Dynamic,      0x19,     "Dynamic")},
    { _TOKEN(TN_DMTND_Exec,         0x1A,     "Exec")},
    { _TOKEN(TN_DMTND_float,        0x1B,     "float")},
    { _TOKEN(TN_DMTND_Format,       0x1C,     "Format")},
    { _TOKEN(TN_DMTND_Get,          0x1D,     "Get")},
    { _TOKEN(TN_DMTND_int,          0x1E,     "int")},
    { _TOKEN(TN_DMTND_Man,          0x1F,     "Man")},
    { _TOKEN(TN_DMTND_MgmtTree,     0x20,     "MgmtTree")},
    { _TOKEN(TN_DMTND_MIME,         0x21,     "MIME")},
    { _TOKEN(TN_DMTND_Mod,          0x22,     "Mod")},
    { _TOKEN(TN_DMTND_Name,         0x23,     "Name")},
    { _TOKEN(TN_DMTND_Node,         0x24,     "Node")},
    { _TOKEN(TN_DMTND_node,         0x25,     "node")},
    { _TOKEN(TN_DMTND_NodeName,     0x26,     "NodeName")},
    { _TOKEN(TN_DMTND_null,         0x27,     "null")},
    { _TOKEN(TN_DMTND_Occurrence,   0x28,     "Occurrence")},
    { _TOKEN(TN_DMTND_One,          0x29,     "One")},
    { _TOKEN(TN_DMTND_OneOrMore,    0x2A,     "OneOrMore")},
    { _TOKEN(TN_DMTND_OneOrN,       0x2B,     "OneOrN")},
    { _TOKEN(TN_DMTND_Path,         0x2C,     "Path")},
    { _TOKEN(TN_DMTND_Permanent,    0x2D,     "Permanent")},
    { _TOKEN(TN_DMTND_Replace,      0x2E,     "Replace")},
    { _TOKEN(TN_DMTND_RTProperties, 0x2F,     "RTProperties")},
    { _TOKEN(TN_DMTND_Scope,        0x30,     "Scope")},
    { _TOKEN(TN_DMTND_Size,         0x31,     "Size")},
    { _TOKEN(TN_DMTND_time,         0x32,     "time")},
    { _TOKEN(TN_DMTND_Title,        0x33,     "Title")},
    { _TOKEN(TN_DMTND_TStamp,       0x34,     "TStamp")},
    { _TOKEN(TN_DMTND_Type,         0x35,     "Type")},
    { _TOKEN(TN_DMTND_Value,        0x36,     "Value")},
    { _TOKEN(TN_DMTND_VerDTD,       0x37,     "VerDTD")},
    { _TOKEN(TN_DMTND_VerNo,        0x38,     "VerNo")},
    { _TOKEN(TN_DMTND_xml,          0x39,     "xml")},
    { _TOKEN(TN_DMTND_ZeroOrMore,   0x3A,     "ZeroOrMore")},
    { _TOKEN(TN_DMTND_ZeroOrN,      0x3B,     "ZeroOrN")},
    { _TOKEN(TN_DMTND_ZeroOrOne,    0x3C,     "ZeroOrOne") }
  };
  #endif

  #ifndef NOWSM
  _tmpTagPtr = NULL; 
  pGA = mgrGetSyncMLAnchor();
  if (pGA == NULL) return NULL;
  #endif

  /* get the correct codepage */
  if (ext == SML_EXT_UNDEFINED) {
    #ifndef NOWSM
    _tmpTagPtr = pGA->tokTbl->SyncML;
    if (_tmpTagPtr == NULL) {
		  mySize = sizeof(syncml);
		  pGA->tokTbl->SyncML = (TagPtr_t)smlLibMalloc(mySize);
		  if (pGA->tokTbl->SyncML == NULL) return NULL;
		  smlLibMemcpy(pGA->tokTbl->SyncML, &syncml, mySize);
      _tmpTagPtr = pGA->tokTbl->SyncML;
    }
    #else
    _tmpTagPtr=(TagPtr_t)syncml;
    #endif
  }

  #ifdef __USE_METINF__
	else if (ext == SML_EXT_METINF) {
    #ifndef NOWSM
    _tmpTagPtr = pGA->tokTbl->MetInf;
    if (_tmpTagPtr == NULL) {
		  mySize = sizeof(metinf);
		  pGA->tokTbl->MetInf = (TagPtr_t)smlLibMalloc(mySize);
		  if (pGA->tokTbl->MetInf == NULL) return NULL;
		  smlLibMemcpy(pGA->tokTbl->MetInf, &metinf, mySize);
      _tmpTagPtr = pGA->tokTbl->MetInf;
    }
    #else
    _tmpTagPtr=(TagPtr_t)metinf;
    #endif
  }
  #endif
  
  #ifdef __USE_DEVINF__
	else if (ext == SML_EXT_DEVINF) {
    #ifndef NOWSM
    _tmpTagPtr = pGA->tokTbl->DevInf;
    if (_tmpTagPtr == NULL) {
  		mySize = sizeof(devinf);
  		pGA->tokTbl->DevInf = (TagPtr_t)smlLibMalloc(mySize);
  		if (pGA->tokTbl->DevInf == NULL) return NULL;
  		smlLibMemcpy(pGA->tokTbl->DevInf, &devinf, mySize);
      _tmpTagPtr = pGA->tokTbl->DevInf;
    }
    #else
    _tmpTagPtr=(TagPtr_t)devinf;
    #endif
  }
  #endif

  #ifdef __USE_DMTND__
  else if (ext == SML_EXT_DMTND) {
    #ifndef NOWSM
    _tmpTagPtr = pGA->tokTbl->DmTnd;
    if (_tmpTagPtr == NULL) {
  		mySize = sizeof(dmtnd);
  		pGA->tokTbl->DmTnd = (TagPtr_t)smlLibMalloc(mySize);
  		if (pGA->tokTbl->DmTnd == NULL) return NULL;
  		smlLibMemcpy(pGA->tokTbl->DmTnd, &dmtnd, mySize);
      _tmpTagPtr = pGA->tokTbl->DmTnd;
    }
    #else
    _tmpTagPtr=(TagPtr_t)dmtnd;
    #endif
  }
  #endif

  return _tmpTagPtr;  
}

#undef _TOKEN // we don't need that macro any longer 

/**
 * FUNCTION: getTagString
 *
 * Returns a tag string which belongs to a tag ID. 
 * This function is needed for the XML encoding
 *
 * PRE-Condition:   valid tag ID, the tagSring has to be allocated 
 *
 * POST-Condition:  tag string is returned
 *
 * IN:              tagId, the ID for the tag 
 *
 * IN/OUT:          tagString, allocated string into which the XML 
 *                             tag string will be written
 * 
 * RETURN:          0,if OK
 */
#ifdef __SML_XML__ 
Ret_t getTagString(XltTagID_t tagID, String_t tagString, SmlPcdataExtension_t ext)
{
  int i = 0;
  TagPtr_t pTags = getTagTable(ext);
  if (pTags == NULL) {
    tagString[0] = '\0';
    return SML_ERR_NOT_ENOUGH_SPACE;
  }
  while (((pTags+i)->id) != TN_UNDEF) {
    if ((((pTags+i)->id) == tagID)) {
      String_t _tmp = (pTags+i)->xml;
      smlLibStrcpy(tagString, _tmp); 
      return SML_ERR_OK;
    }    
    i++;
  }        
  //smlLibStrcpy(tagString, '\0'); 
  tagString[0] = '\0';
  return SML_ERR_XLT_INVAL_PROTO_ELEM;
}
#endif

/**
 * FUNCTION: getTagByte
 *
 * Returns a WBXML byte which belongs to a tag ID in a defined codepage. 
 * This function is needed for the WBXML encoding
 *
 * PRE-Condition:   valid tag ID, valid code page
 *
 * POST-Condition:  tag byte is returned
 *
 * IN:              tagId, the ID for the tag 
 *                  cp, code page group for the tag 
 *                  pTagByte, the byte representation of the tag
 * 
 * RETURN:          0, if OK
 */
Ret_t getTagByte(XltTagID_t tagID, SmlPcdataExtension_t ext, Byte_t *pTagByte)
{    
  int i = 0; 
  TagPtr_t pTags = getTagTable(ext);
  if (pTags == NULL)
  {
    return SML_ERR_NOT_ENOUGH_SPACE;
  }
  while (((pTags+i)->id) != TN_UNDEF)
  {
    if (((pTags+i)->id) == tagID)
    {
    	*pTagByte = (pTags+i)->wbxml;
      return SML_ERR_OK;
    }    
    i++;
  }        
  *pTagByte = 0;
  return SML_ERR_XLT_INVAL_PROTO_ELEM;
}

/**
 * FUNCTION: getCodePage
 *
 * Returns the code page which belongs to a certain PCDATA extension type. 
 *
 * PRE-Condition:   valid PCDATA extension type
 *
 * POST-Condition:  the code page is returned
 *
 * IN:              ext, the PCDATA extension type
 * 
 * RETURN:          the code page
 */
Byte_t getCodePage(SmlPcdataExtension_t ext)
{
  #ifdef __USE_DMTND__
  if (ext == SML_EXT_DMTND)
    return 2;
  #endif        
  #ifdef __USE_METINF__
  if (ext == SML_EXT_METINF)
    return 1;
  #endif        
  #ifdef __USE_DEVINF__
  if (ext == SML_EXT_DEVINF)
    return 0;
  #endif        
    return 0;
}

/**
 * FUNCTION: getCodePageById
 *
 * Returns the codepage which belongs to a certain tag ID
 *
 * PRE-Condition:   valid tag ID
 *
 * POST-Condition:  the code page is returned
 *
 * IN:              tagID, the ID of the tag 
 *                  pCp, the codepage/extention of the tag
 *
 * RETURN:          0, if OK
 */
Ret_t getExtById(XltTagID_t tagID,  SmlPcdataExtension_t *pExt)
{
    int i = 0; 
	SmlPcdataExtension_t ext;
	/* Iterate over all defined extensions to find the corresponding TAG.
	 * Empty extensions, e.g. not defined numbers will be skipped.
	 */
	for (ext = SML_EXT_UNDEFINED; ext < SML_EXT_LAST; ext++) {
		TagPtr_t pTags = getTagTable(ext);
		if (pTags == NULL) {
			continue; /* skip empty codepage */
		}
		i = 0;
		while (((pTags+i)->id) != TN_UNDEF) {
			if ((((pTags+i)->id) == tagID)){
       			*pExt = ext;
    			return SML_ERR_OK;
    	    }    
    		i++;
        }        
    }
	/* tag not found in any extension */
  *pExt = (SmlPcdataExtension_t)255;
  return SML_ERR_XLT_INVAL_PROTO_ELEM;
}

/**
 * FUNCTION: getTagIDByStringAndCodepage
 *
 * Returns the tag ID which belongs to a tag string in a certain codepage
 *
 * PRE-Condition:   valid tag string, valid code page
 *
 * POST-Condition:  tag id is returned
 *
 * IN:              tag, the string representation of the tag 
 *                  cp, code page group for the tag 
 *                  pTagID, the tag id of the tag
 * 
 * RETURN:          0, if OK
 */

Ret_t getTagIDByStringAndExt(String_t tag, SmlPcdataExtension_t ext, XltTagID_t *pTagID)
{
    int i = 0; 
    TagPtr_t pTags = getTagTable(ext);
    if (pTags == NULL) {
      return SML_ERR_NOT_ENOUGH_SPACE;
    }
    for (i=0;((pTags+i)->id) != TN_UNDEF; i++) {
	    if (*(pTags+i)->xml != *tag) continue; // if the first char doesn't match we skip the strcmp to speed things up
        if (smlLibStrcmp(((pTags+i)->xml), tag) == 0) {
            *pTagID = (pTags+i)->id;
            return SML_ERR_OK;
        }         
    }        
    *pTagID = TN_UNDEF;
    return SML_ERR_XLT_INVAL_PROTO_ELEM;
}

/**
 * FUNCTION: getTagIDByByteAndCodepage
 *
 * Returns the tag ID which belongs to a tag byte in a certain codepage
 *
 * PRE-Condition:   valid tag byte, valid code page
 *
 * POST-Condition:  tag id is returned
 *
 * IN:              tag, the byte representation of the tag 
 *                  cp, code page group for the tag  
 *                  pTagID, the tag id of the tag
 * 
 * RETURN:          0, if OK
 */
Ret_t getTagIDByByteAndExt(Byte_t tag, SmlPcdataExtension_t ext, XltTagID_t *pTagID)
{

    int i = 0; 
    TagPtr_t pTags = getTagTable(ext);
    if (pTags == NULL)
    {
      return SML_ERR_NOT_ENOUGH_SPACE;
    }
    while (((pTags+i)->id) != TN_UNDEF)
    {
      if (((pTags+i)->wbxml) == tag)
      {
        *pTagID = (pTags+i)->id;
        return SML_ERR_OK;
      }    
      i++;
    }        
    *pTagID = TN_UNDEF;
    return SML_ERR_XLT_INVAL_PROTO_ELEM;
}

/**
 * FUNCTION: getTagIDByStringAndNamespace
 *
 * Returns the tag ID which belongs to a tag string in a certain namespace
 *
 * PRE-Condition:   valid tag string, valid namespace
 *
 * POST-Condition:  tag id is returned
 *
 * IN:              tag, the string representation of the tag 
 *                  ns, namespace group for the tag  
 *                  pTagID, the tag id of the tag
 * 
 * RETURN:          0, if OK
 */
#ifdef __SML_XML__ 
Ret_t getTagIDByStringAndNamespace(String_t tag, String_t ns, XltTagID_t *pTagID)
{
    int i = 0; 
    TagPtr_t pTags = getTagTable(getExtByName(ns));
    if (pTags == NULL)
    {
      return SML_ERR_NOT_ENOUGH_SPACE;
    }
    while (((pTags+i)->id) != TN_UNDEF)
    {
      if ((smlLibStrcmp(((pTags+i)->xml), tag) == 0))
      {
        *pTagID = (pTags+i)->id;
        return SML_ERR_OK;
      }    
      i++;
    }        
    *pTagID = TN_UNDEF;
    return SML_ERR_XLT_INVAL_PROTO_ELEM;
}

#endif
