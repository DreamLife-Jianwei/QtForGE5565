/*
 *--------------------------------------------------------------------------
 *                       COPYRIGHT NOTICE
 *
 *  Copyright (C) 2000-2010 GE Intelligent Platforms Embedded Systems, Inc.
 *  International Copyright Secured.  All Rights Reserved.
 *--------------------------------------------------------------------------
 *
 *  $Workfile: rfm2gdll_stdc.h $
 *  $Revision: 3 $
 *  $Modtime: 3/15/10 1:31p $
 *
 *--------------------------------------------------------------------------
 *	
 *--------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C"  {
#endif  /* __cplusplus */

#include "rfm2g_types.h"
#include "rfm2g_api.h"

/* Define custom timeout values */
#define RFM_INFINITE		INFINITE
#define INFINITE_TIMEOUT	(unsigned int)(0xFFFFFFFF)

/***********************************************************************
*	These are beyond the core API functions listed above and if used may
*	not exist in versions of this driver for other operating systems 
***********************************************************************/
DECL_SPEC(unsigned)		RFM2gRegOffset(	RFM2GHANDLE		pHandle,
										char			*pRegNm	);

DECL_SPEC(unsigned)		RFM2gRegSize(	RFM2GHANDLE		pHandle,
										const char		*pRegNm	);

DECL_SPEC(const char*)	RFM2gRegName(	RFM2GHANDLE		pHandle,
										unsigned		Offset	);

DECL_SPEC(USHORT)		RFM2gEventId(	RFM2GHANDLE		pHandle,
										char			*pEvntNm	);

DECL_SPEC(const char*)	RFM2gEventName(	RFM2GHANDLE		pHandle,
										USHORT			Id	);

DECL_SPEC(RFM2G_STATUS)	RFM2gNevents(	RFM2GHANDLE		pHandle,
										RFMGETNEVENTS	pEvntInfo,
										ULONG32 			bClear	);

DECL_SPEC(UCHAR)		RFM2gPeekReg8(	RFM2GHANDLE	pHandle,
										unsigned	Offset	);

DECL_SPEC(USHORT)		RFM2gPeekReg16( RFM2GHANDLE	pHandle,
										unsigned	Offset	);

DECL_SPEC(ULONG32)		RFM2gPeekReg32(	RFM2GHANDLE	pHandle,
										unsigned	Offset	);

DECL_SPEC(const ULONG32)	RFM2gRegisterSize(RFM2GHANDLE pHandle);

#ifdef  __cplusplus
};
#endif  /* __cplusplus */
