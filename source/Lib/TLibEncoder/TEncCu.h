/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2015, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/** \file     TEncCu.h
\brief    Coding Unit (CU) encoder class (header)
*/

#ifndef __TENCCU__
#define __TENCCU__

// Include files
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComDataCU.h"

#include "TEncEntropy.h"
#include "TEncSearch.h"
#include "TEncRateCtrl.h"

#if PGR_ENABLE

#include "TLibCommon/PixelPrediction.h"

#endif

//! \ingroup TLibEncoder
//! \{
class TEncTop;
class TEncSbac;
class TEncCavlc;
class TEncSlice;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU encoder class
class TEncCu
{
private:

	TComDataCU**            m_ppcBestCU;      ///< Best CUs in each depth
	TComDataCU**            m_ppcTempCU;      ///< Temporary CUs in each depth
	UChar                   m_uhTotalDepth;

	TComYuv**               m_ppcPredYuvBest; ///< Best Prediction Yuv for each depth
	TComYuv**               m_ppcResiYuvBest; ///< Best Residual Yuv for each depth
	TComYuv**               m_ppcRecoYuvBest; ///< Best Reconstruction Yuv for each depth
	TComYuv**               m_ppcPredYuvTemp; ///< Temporary Prediction Yuv for each depth
	TComYuv**               m_ppcResiYuvTemp; ///< Temporary Residual Yuv for each depth
#if PGR_ENABLE
    TComYuv**               m_ppcAbnormalResiYuvTemp; ///< Temporary  abnormatl Residual Yuv for each depth
#endif
	TComYuv**               m_ppcRecoYuvTemp; ///< Temporary Reconstruction Yuv for each depth
	TComYuv**               m_ppcOrigYuv;     ///< Original Yuv for each depth
	TComYuv**               m_ppcNoCorrYuv;

#if PGR_ENABLE
	// ---- Template Match ----
	Pixel*					m_pPixel[MAX_NUM_COMPONENT];						///< pixel data
	
#endif

	//  Data : encoder control
	Bool                    m_bEncodeDQP;
	Bool                    m_CodeChromaQpAdjFlag;
	Int                     m_ChromaQpAdjIdc;

	//  Access channel
	TEncCfg*                m_pcEncCfg;
	TEncSearch*             m_pcPredSearch;
	TComTrQuant*            m_pcTrQuant;
	TComRdCost*             m_pcRdCost;

	TEncEntropy*            m_pcEntropyCoder;
	TEncBinCABAC*           m_pcBinCABAC;

	// SBAC RD
	TEncSbac***             m_pppcRDSbacCoder;
	TEncSbac*               m_pcRDGoOnSbacCoder;
	TEncRateCtrl*           m_pcRateCtrl;

public:
	/// copy parameters from encoder class
	Void  init(TEncTop* pcEncTop);

	/// create internal buffers
	Void  create(UChar uhTotalDepth, UInt iMaxWidth, UInt iMaxHeight, ChromaFormat chromaFormat
		, UInt uiPLTMaxSize, UInt uiPLTMaxPredSize
		);
#if PGR_ENABLE
	// create pgr buffers
	Void  createPGR(UInt uiPicWidth, UInt uiPicHeight, ChromaFormat chromaFormat, UInt uiMaxCuWidth, UInt uiMaxCuHeight, UInt uiTotalCuDepth);

	// init estimation data
	Void  initEstPGR(TComPic* pcPic);

	Pixel** getPixel() { return m_pPixel; };

#endif

	/// destroy internal buffers
	Void  destroy();

	/// CTU analysis function
	Void  compressCtu(TComDataCU* pCtu, UChar* lastPLTSize, UChar* lastPLTUsedSize, Pel lastPLT[][MAX_PLT_PRED_SIZE]);

	/// CTU encoding function
	Void  encodeCtu(TComDataCU*  pCtu);

	Int   updateCtuDataISlice(TComDataCU* pCtu, Int width, Int height);


protected:
	Void  finishCU(TComDataCU*  pcCU, UInt uiAbsPartIdx, UInt uiDepth);
#if AMP_ENC_SPEEDUP
	Void  xCompressCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug), PartSize eParentPartSize = NUMBER_OF_PART_SIZES);

#if PGR_ENABLE
	Void  xCompressCUPGR(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth); // UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug), PartSize eParentPartSize = NUMBER_OF_PART_SIZES);

	Void  reviseAnomalyResidue(TComDataCU*& rpcTempCU,TComYuv*& pcOrgYuv, TComYuv*& pcPreYuv, TComYuv*& pcResiYuv, TComYuv*& pcAnYuv, UInt uiResidueThreshold);

	Void  xCheckPRGResidue(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU,UInt qp, Bool bTransquantBypass);
#endif

#else
	Void  xCompressCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth);
#endif
	Void  xEncodeCU(TComDataCU*  pcCU, UInt uiAbsPartIdx, UInt uiDepth);

	Int   xComputeQP(TComDataCU* pcCU, UInt uiDepth);
	Void  xCheckBestMode(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sParent) DEBUG_STRING_FN_DECLARE(sTest) DEBUG_STRING_PASS_INTO(Bool bAddSizeInfo = true));

	Void  xCheckRDCostMerge2Nx2N(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool *earlyDetectionSkipMode, Bool checkSkipOnly);

#if SCM_T0227_INTRABC_SIG_UNIFICATION
	Void  xCheckRDCostIntraBCMerge2Nx2N(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU);
#endif

#if AMP_MRG
#if SCM_T0227_INTRABC_SIG_UNIFICATION
	Void  xCheckRDCostInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG = false, TComMv *iMVCandList = NULL);
#else
	Void  xCheckRDCostInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG = false);
#endif
#else
#if SCM_T0227_INTRABC_SIG_UNIFICATION
	Void  xCheckRDCostInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize, TComMv *iMVCandList = NULL);
#else
	Void  xCheckRDCostInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize);
#endif
#endif

	Void  xCheckRDCostIntra(TComDataCU *&rpcBestCU,
		TComDataCU *&rpcTempCU,
		Double      &cost,
		PartSize     ePartSize
		DEBUG_STRING_FN_DECLARE(sDebug)
		, Bool         bRGBIntraModeReuse = false
		);

	Void  xCheckRDCostIntraCSC(TComDataCU *&rpcBestCU,
		TComDataCU *&rpcTempCU,
		Double      &cost,
		PartSize    ePartSize
		DEBUG_STRING_FN_DECLARE(sDebug)
		);

	Void  xCheckRDCostIntraBC(TComDataCU*& rpcBestCU,
		TComDataCU*& rpcTempCU,
		Bool         bUse1DSearchFor8x8
		, PartSize     eSize
		, Double&      rdCost
		, Bool         testPredOnly = false
#if SCM_T0227_INTRABC_SIG_UNIFICATION
		, TComMv *iMVCandList = NULL
#endif
		DEBUG_STRING_FN_DECLARE(sDebug)
		);
#if SCM_T0227_INTRABC_SIG_UNIFICATION
	Void  xCheckRDCostIntraBCMixed(TComDataCU*& rpcBestCU,
		TComDataCU*& rpcTempCU,
		PartSize     eSize
		, Double&      rdCost
		DEBUG_STRING_FN_DECLARE(sDebug)
		, TComMv *iMVCandList
		);
#endif

	Void  xCheckDQP(TComDataCU*  pcCU);
	Void  xCheckRDCostHashInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, Bool& isPerfectMatch DEBUG_STRING_FN_DECLARE(sDebug));

	Void  xCheckIntraPCM(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU);
#if SCM_T0064_REMOVE_PLT_SHARING
	Void  xCheckPLTMode(TComDataCU *&rpcBestCU, TComDataCU*& rpcTempCU, Bool forcePLTPrediction);
#else
	Void  xCheckPLTMode(TComDataCU *&rpcBestCU, TComDataCU*& rpcTempCU, Bool bCheckPLTSharingMode);
#endif
	Void  xCopyAMVPInfo(AMVPInfo* pSrc, AMVPInfo* pDst);
	Void  xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth, TComDataCU* pcCU, UInt uiLPelX, UInt uiTPelY);
	Void  xCopyYuv2Tmp(UInt uhPartUnitIdx, UInt uiDepth);

	Bool getdQPFlag()                        { return m_bEncodeDQP; }
	Void setdQPFlag(Bool b)                { m_bEncodeDQP = b; }

	Bool getCodeChromaQpAdjFlag() { return m_CodeChromaQpAdjFlag; }
	Void setCodeChromaQpAdjFlag(Bool b) { m_CodeChromaQpAdjFlag = b; }

#if ADAPTIVE_QP_SELECTION
	// Adaptive reconstruction level (ARL) statistics collection functions
	Void xCtuCollectARLStats(TComDataCU* pCtu);
	Int  xTuCollectARLStats(TCoeff* rpcCoeff, TCoeff* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples);
#endif

#if AMP_ENC_SPEEDUP
#if AMP_MRG
	Void deriveTestModeAMP(TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver);
#else
	Void deriveTestModeAMP(TComDataCU *pcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver);
#endif
#endif

	Void  xFillPCMBuffer(TComDataCU* pCU, TComYuv* pOrgYuv);
};

//! \}

#endif // __TENCMB__
