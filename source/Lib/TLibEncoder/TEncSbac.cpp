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

/** \file     TEncSbac.cpp
    \brief    SBAC encoder class
*/

#include "TEncTop.h"
#include "TEncSbac.h"
#include "TLibCommon/TComTU.h"

#include <map>
#include <algorithm>

#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
#include "../TLibCommon/Debug.h"
#endif


//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSbac::TEncSbac()
// new structure here
: m_pcBitIf                            ( NULL )
, m_pcBinIf                            ( NULL )
, m_numContextModels                   ( 0 )
, m_cCUSplitFlagSCModel                ( 1,             1,                      NUM_SPLIT_FLAG_CTX                   , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSkipFlagSCModel                 ( 1,             1,                      NUM_SKIP_FLAG_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeFlagExtSCModel             ( 1,             1,                      NUM_MERGE_FLAG_EXT_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMergeIdxExtSCModel              ( 1,             1,                      NUM_MERGE_IDX_EXT_CTX                , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPartSizeSCModel                 ( 1,             1,                      NUM_PART_SIZE_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUPredModeSCModel                 ( 1,             1,                      NUM_PRED_MODE_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUIntraPredSCModel                ( 1,             1,                      NUM_ADI_CTX                          , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUChromaPredSCModel               ( 1,             1,                      NUM_CHROMA_PRED_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUDeltaQpSCModel                  ( 1,             1,                      NUM_DELTA_QP_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUInterDirSCModel                 ( 1,             1,                      NUM_INTER_DIR_CTX                    , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCURefPicSCModel                   ( 1,             1,                      NUM_REF_NO_CTX                       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUMvdSCModel                      ( 1,             1,                      NUM_MV_RES_CTX                       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtCbfSCModel                    ( 1,             NUM_QT_CBF_CTX_SETS,    NUM_QT_CBF_CTX_PER_SET               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUTransSubdivFlagSCModel          ( 1,             1,                      NUM_TRANS_SUBDIV_FLAG_CTX            , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUQtRootCbfSCModel                ( 1,             1,                      NUM_QT_ROOT_CBF_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigCoeffGroupSCModel            ( 1,             2,                      NUM_SIG_CG_FLAG_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUSigSCModel                      ( 1,             1,                      NUM_SIG_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastX                        ( 1,             NUM_CTX_LAST_FLAG_SETS, NUM_CTX_LAST_FLAG_XY                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCuCtxLastY                        ( 1,             NUM_CTX_LAST_FLAG_SETS, NUM_CTX_LAST_FLAG_XY                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUOneSCModel                      ( 1,             1,                      NUM_ONE_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUAbsSCModel                      ( 1,             1,                      NUM_ABS_FLAG_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
, m_cMVPIdxSCModel                     ( 1,             1,                      NUM_MVP_IDX_CTX                      , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoMergeSCModel                   ( 1,             1,                      NUM_SAO_MERGE_FLAG_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
, m_cSaoTypeIdxSCModel                 ( 1,             1,                      NUM_SAO_TYPE_IDX_CTX                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_cTransformSkipSCModel              ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_TRANSFORMSKIP_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_CUTransquantBypassFlagSCModel      ( 1,             1,                      NUM_CU_TRANSQUANT_BYPASS_FLAG_CTX    , m_contextModels + m_numContextModels, m_numContextModels)
, m_explicitRdpcmFlagSCModel           ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_EXPLICIT_RDPCM_FLAG_CTX          , m_contextModels + m_numContextModels, m_numContextModels)
, m_explicitRdpcmDirSCModel            ( 1,             MAX_NUM_CHANNEL_TYPE,   NUM_EXPLICIT_RDPCM_DIR_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
, m_cIntraBCPredFlagSCModel            ( 1,             1,                      NUM_INTRABC_PRED_CTX                 , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_cCrossComponentPredictionSCModel   ( 1,             1,                      NUM_CROSS_COMPONENT_PREDICTION_CTX   , m_contextModels + m_numContextModels, m_numContextModels)
, m_PLTModeFlagSCModel                 ( 1,             1,                      NUM_PLTMODE_FLAG_CTX                 , m_contextModels + m_numContextModels, m_numContextModels)
, m_SPointSCModel                      ( 1,             1,                      NUM_SPOINT_CTX                       , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCopyTopRunSCModel                 ( 1,             1,                      NUM_TOP_RUN_CTX                      , m_contextModels + m_numContextModels, m_numContextModels)
, m_cRunSCModel                        ( 1,             1,                      NUM_LEFT_RUN_CTX                     , m_contextModels + m_numContextModels, m_numContextModels)
#if !SCM_T0064_REMOVE_PLT_SHARING
, m_PLTSharingModeFlagSCModel          ( 1,             1,                      NUM_PLT_REUSE_FLAG_CTX               , m_contextModels + m_numContextModels, m_numContextModels)
#endif
#if SCM_T0065_PLT_IDX_GROUP
, m_PLTLastRunTypeSCModel              ( 1,             1,                      NUM_PLT_LAST_RUN_TYPE_CTX            , m_contextModels + m_numContextModels, m_numContextModels)
#endif
, m_PLTScanRotationModeFlagSCModel     ( 1,             1,                      NUM_SCAN_ROTATION_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_ChromaQpAdjFlagSCModel             ( 1,             1,                      NUM_CHROMA_QP_ADJ_FLAG_CTX           , m_contextModels + m_numContextModels, m_numContextModels)
, m_ChromaQpAdjIdcSCModel              ( 1,             1,                      NUM_CHROMA_QP_ADJ_IDC_CTX            , m_contextModels + m_numContextModels, m_numContextModels)
, m_cCUColourTransformFlagSCModel      ( 1,             1,                      NUM_COLOUR_TRANS_CTX                 , m_contextModels + m_numContextModels, m_numContextModels)
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
, m_cIntraBCBVDSCModel                 (1,              1,                      NUM_INTRABC_BVD_CTX                  , m_contextModels + m_numContextModels, m_numContextModels)
#endif
{
  assert( m_numContextModels <= MAX_NUM_CTX_MOD );
}

TEncSbac::~TEncSbac()
{
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSbac::resetEntropy           (const TComSlice *pSlice)
{
  Int  iQp              = pSlice->getSliceQp();
  SliceType eSliceType  = pSlice->getSliceType();

  SliceType encCABACTableIdx = pSlice->getEncCABACTableIdx();
  if (!pSlice->isIntra() && (encCABACTableIdx==B_SLICE || encCABACTableIdx==P_SLICE) && pSlice->getPPS()->getCabacInitPresentFlag())
  {
    eSliceType = encCABACTableIdx;
  }

#if SCM_T0227_INTRABC_SIG_UNIFICATION
  if ( eSliceType == I_SLICE && pSlice->getSPS()->getUseIntraBlockCopy() )
  {
    eSliceType = P_SLICE;
  }
#endif

  m_cCUSplitFlagSCModel.initBuffer                ( eSliceType, iQp, (UChar*)INIT_SPLIT_FLAG );
  m_cCUSkipFlagSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_SKIP_FLAG );
  m_cCUMergeFlagExtSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_MERGE_FLAG_EXT);
  m_cCUMergeIdxExtSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_MERGE_IDX_EXT);
  m_cCUPartSizeSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_PART_SIZE );
  m_cCUPredModeSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_PRED_MODE );
  m_cCUIntraPredSCModel.initBuffer                ( eSliceType, iQp, (UChar*)INIT_INTRA_PRED_MODE );
  m_cCUChromaPredSCModel.initBuffer               ( eSliceType, iQp, (UChar*)INIT_CHROMA_PRED_MODE );
  m_cCUInterDirSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_INTER_DIR );
  m_cCUMvdSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_MVD );
  m_cCURefPicSCModel.initBuffer                   ( eSliceType, iQp, (UChar*)INIT_REF_PIC );
  m_cCUDeltaQpSCModel.initBuffer                  ( eSliceType, iQp, (UChar*)INIT_DQP );
  m_cCUQtCbfSCModel.initBuffer                    ( eSliceType, iQp, (UChar*)INIT_QT_CBF );
  m_cCUQtRootCbfSCModel.initBuffer                ( eSliceType, iQp, (UChar*)INIT_QT_ROOT_CBF );
  m_cCUSigCoeffGroupSCModel.initBuffer            ( eSliceType, iQp, (UChar*)INIT_SIG_CG_FLAG );
  m_cCUSigSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_SIG_FLAG );
  m_cCuCtxLastX.initBuffer                        ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCuCtxLastY.initBuffer                        ( eSliceType, iQp, (UChar*)INIT_LAST );
  m_cCUOneSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_ONE_FLAG );
  m_cCUAbsSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_ABS_FLAG );
  m_cMVPIdxSCModel.initBuffer                     ( eSliceType, iQp, (UChar*)INIT_MVP_IDX );
  m_cCUTransSubdivFlagSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
  m_cSaoMergeSCModel.initBuffer                   ( eSliceType, iQp, (UChar*)INIT_SAO_MERGE_FLAG );
  m_cSaoTypeIdxSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_SAO_TYPE_IDX );
  m_cTransformSkipSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_TRANSFORMSKIP_FLAG );
  m_CUTransquantBypassFlagSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG );
  m_explicitRdpcmFlagSCModel.initBuffer           ( eSliceType, iQp, (UChar*)INIT_EXPLICIT_RDPCM_FLAG);
  m_explicitRdpcmDirSCModel.initBuffer            ( eSliceType, iQp, (UChar*)INIT_EXPLICIT_RDPCM_DIR);
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
  m_cIntraBCPredFlagSCModel.initBuffer            ( eSliceType, iQp, (UChar*)INIT_INTRABC_PRED_FLAG );
#endif
  m_cCrossComponentPredictionSCModel.initBuffer   ( eSliceType, iQp, (UChar*)INIT_CROSS_COMPONENT_PREDICTION  );
  m_PLTModeFlagSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_PLTMODE_FLAG );
  m_SPointSCModel.initBuffer                      ( eSliceType, iQp, (UChar*)INIT_SPOINT );
  m_cCopyTopRunSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_TOP_RUN);
  m_cRunSCModel.initBuffer                        ( eSliceType, iQp, (UChar*)INIT_RUN);
#if !SCM_T0064_REMOVE_PLT_SHARING
  m_PLTSharingModeFlagSCModel.initBuffer          ( eSliceType, iQp, (UChar*)INIT_PLT_REUSE_FLAG);
#endif
#if SCM_T0065_PLT_IDX_GROUP
  m_PLTLastRunTypeSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_PLT_LAST_RUN_TYPE);
#endif
  m_PLTScanRotationModeFlagSCModel.initBuffer     ( eSliceType, iQp, (UChar*)INIT_SCAN_ROTATION_FLAG );
  m_ChromaQpAdjFlagSCModel.initBuffer             ( eSliceType, iQp, (UChar*)INIT_CHROMA_QP_ADJ_FLAG );
  m_ChromaQpAdjIdcSCModel.initBuffer              ( eSliceType, iQp, (UChar*)INIT_CHROMA_QP_ADJ_IDC );
  m_cCUColourTransformFlagSCModel.initBuffer      ( eSliceType, iQp, (UChar*)INIT_COLOUR_TRANS );
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
  m_cIntraBCBVDSCModel.initBuffer                 ( eSliceType, iQp, (UChar*)INIT_INTRABC_BVD);
#endif

  for (UInt statisticIndex = 0; statisticIndex < RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS ; statisticIndex++)
  {
    m_golombRiceAdaptationStatistics[statisticIndex] = 0;
  }

  m_pcBinIf->start();

  return;
}

/** The function does the following:
 * If current slice type is P/B then it determines the distance of initialisation type 1 and 2 from the current CABAC states and
 * stores the index of the closest table.  This index is used for the next P/B slice when cabac_init_present_flag is true.
 */
SliceType TEncSbac::determineCabacInitIdx(const TComSlice *pSlice)
{
  Int  qp              = pSlice->getSliceQp();

  if (!pSlice->isIntra())
  {
    SliceType aSliceTypeChoices[] = {B_SLICE, P_SLICE};

    UInt bestCost             = MAX_UINT;
    SliceType bestSliceType   = aSliceTypeChoices[0];
    for (UInt idx=0; idx<2; idx++)
    {
      UInt curCost          = 0;
      SliceType curSliceType  = aSliceTypeChoices[idx];

      curCost  = m_cCUSplitFlagSCModel.calcCost                ( curSliceType, qp, (UChar*)INIT_SPLIT_FLAG );
      curCost += m_cCUSkipFlagSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_SKIP_FLAG );
      curCost += m_cCUMergeFlagExtSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_MERGE_FLAG_EXT);
      curCost += m_cCUMergeIdxExtSCModel.calcCost              ( curSliceType, qp, (UChar*)INIT_MERGE_IDX_EXT);
      curCost += m_cCUPartSizeSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_PART_SIZE );
      curCost += m_cCUPredModeSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_PRED_MODE );
      curCost += m_cCUIntraPredSCModel.calcCost                ( curSliceType, qp, (UChar*)INIT_INTRA_PRED_MODE );
      curCost += m_cCUChromaPredSCModel.calcCost               ( curSliceType, qp, (UChar*)INIT_CHROMA_PRED_MODE );
      curCost += m_cCUInterDirSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_INTER_DIR );
      curCost += m_cCUMvdSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_MVD );
      curCost += m_cCURefPicSCModel.calcCost                   ( curSliceType, qp, (UChar*)INIT_REF_PIC );
      curCost += m_cCUDeltaQpSCModel.calcCost                  ( curSliceType, qp, (UChar*)INIT_DQP );
      curCost += m_cCUQtCbfSCModel.calcCost                    ( curSliceType, qp, (UChar*)INIT_QT_CBF );
      curCost += m_cCUQtRootCbfSCModel.calcCost                ( curSliceType, qp, (UChar*)INIT_QT_ROOT_CBF );
      curCost += m_cCUSigCoeffGroupSCModel.calcCost            ( curSliceType, qp, (UChar*)INIT_SIG_CG_FLAG );
      curCost += m_cCUSigSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_SIG_FLAG );
      curCost += m_cCuCtxLastX.calcCost                        ( curSliceType, qp, (UChar*)INIT_LAST );
      curCost += m_cCuCtxLastY.calcCost                        ( curSliceType, qp, (UChar*)INIT_LAST );
      curCost += m_cCUOneSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_ONE_FLAG );
      curCost += m_cCUAbsSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_ABS_FLAG );
      curCost += m_cMVPIdxSCModel.calcCost                     ( curSliceType, qp, (UChar*)INIT_MVP_IDX );
      curCost += m_cCUTransSubdivFlagSCModel.calcCost          ( curSliceType, qp, (UChar*)INIT_TRANS_SUBDIV_FLAG );
      curCost += m_cSaoMergeSCModel.calcCost                   ( curSliceType, qp, (UChar*)INIT_SAO_MERGE_FLAG );
      curCost += m_cSaoTypeIdxSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_SAO_TYPE_IDX );
      curCost += m_cTransformSkipSCModel.calcCost              ( curSliceType, qp, (UChar*)INIT_TRANSFORMSKIP_FLAG );
      curCost += m_CUTransquantBypassFlagSCModel.calcCost      ( curSliceType, qp, (UChar*)INIT_CU_TRANSQUANT_BYPASS_FLAG );
      curCost += m_explicitRdpcmFlagSCModel.calcCost           ( curSliceType, qp, (UChar*)INIT_EXPLICIT_RDPCM_FLAG);
      curCost += m_explicitRdpcmDirSCModel.calcCost            ( curSliceType, qp, (UChar*)INIT_EXPLICIT_RDPCM_DIR);
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
      curCost += m_cIntraBCPredFlagSCModel.calcCost            ( curSliceType, qp, (UChar*)INIT_INTRABC_PRED_FLAG );
#endif
      curCost += m_cCrossComponentPredictionSCModel.calcCost   ( curSliceType, qp, (UChar*)INIT_CROSS_COMPONENT_PREDICTION );
      curCost += m_PLTModeFlagSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_PLTMODE_FLAG );
      curCost += m_SPointSCModel.calcCost                      ( curSliceType, qp, (UChar*)INIT_SPOINT );
      curCost += m_cCopyTopRunSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_TOP_RUN );
      curCost += m_cRunSCModel.calcCost                        ( curSliceType, qp, (UChar*)INIT_RUN );
#if !SCM_T0064_REMOVE_PLT_SHARING
      curCost += m_PLTSharingModeFlagSCModel.calcCost          ( curSliceType, qp, (UChar*)INIT_PLT_REUSE_FLAG );
#endif
#if SCM_T0065_PLT_IDX_GROUP
      curCost += m_PLTLastRunTypeSCModel.calcCost              (curSliceType, qp, (UChar*)INIT_PLT_LAST_RUN_TYPE);
#endif
      curCost += m_PLTScanRotationModeFlagSCModel.calcCost     ( curSliceType, qp, (UChar*)INIT_SCAN_ROTATION_FLAG );
      curCost += m_ChromaQpAdjFlagSCModel.calcCost             ( curSliceType, qp, (UChar*)INIT_CHROMA_QP_ADJ_FLAG );
      curCost += m_ChromaQpAdjIdcSCModel.calcCost              ( curSliceType, qp, (UChar*)INIT_CHROMA_QP_ADJ_IDC );
      curCost += m_cCUColourTransformFlagSCModel.calcCost      ( curSliceType, qp, (UChar*)INIT_COLOUR_TRANS );
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
      curCost += m_cIntraBCBVDSCModel.calcCost                 ( curSliceType, qp, (UChar*)INIT_INTRABC_BVD );
#endif

      if (curCost < bestCost)
      {
        bestSliceType = curSliceType;
        bestCost      = curCost;
      }
    }
    return bestSliceType;
  }
  else
  {
    return I_SLICE;
  }
}

Void TEncSbac::codeVPS( const TComVPS* pcVPS )
{
  assert (0);
  return;
}

Void TEncSbac::codeSPS( const TComSPS* pcSPS )
{
  assert (0);
  return;
}

Void TEncSbac::codePPS( const TComPPS* pcPPS )
{
  assert (0);
  return;
}

Void TEncSbac::codeSliceHeader( TComSlice* pcSlice )
{
  assert (0);
  return;
}

Void TEncSbac::codeTilesWPPEntryPoint( TComSlice* pSlice )
{
  assert (0);
  return;
}

Void TEncSbac::codeTerminatingBit( UInt uilsLast )
{
  m_pcBinIf->encodeBinTrm( uilsLast );
}

Void TEncSbac::codeSliceFinish()
{
  m_pcBinIf->finish();
}

Void TEncSbac::xWriteUnarySymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset )
{
  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[0] );

  if( 0 == uiSymbol)
  {
    return;
  }

  while( uiSymbol-- )
  {
    m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ iOffset ] );
  }

  return;
}

Void TEncSbac::xWriteUnaryMaxSymbol( UInt uiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    return;
  }

  m_pcBinIf->encodeBin( uiSymbol ? 1 : 0, pcSCModel[ 0 ] );

  if ( uiSymbol == 0 )
  {
    return;
  }

  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );

  while( --uiSymbol )
  {
    m_pcBinIf->encodeBin( 1, pcSCModel[ iOffset ] );
  }
  if( bCodeLast )
  {
    m_pcBinIf->encodeBin( 0, pcSCModel[ iOffset ] );
  }

  return;
}

Void TEncSbac::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  UInt bins = 0;
  Int numBins = 0;

  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    bins = 2 * bins + 1;
    numBins++;
    uiSymbol -= 1 << uiCount;
    uiCount  ++;
  }
  bins = 2 * bins + 0;
  numBins++;

  bins = (bins << uiCount) | uiSymbol;
  numBins += uiCount;

  assert( numBins <= 32 );
  m_pcBinIf->encodeBinsEP( bins, numBins );
}

#if PGR_ENABLE
Void TEncSbac::xWriteEPExpGolombK0(UInt uiSymbol)
{
	UInt uiTmp = uiSymbol + 1;
	UInt uiNumBins = 0;
	while (uiTmp > 0)
	{
		uiTmp = uiTmp >> 1;
		uiNumBins++;
	}

	m_pcBinIf->encodeBinsEP(uiSymbol + 1, uiNumBins);
}
Void TEncSbac::xWriteEPEXPGolombK(UInt uiSymbol, UInt uiK)
{
	UInt uiTmp = uiSymbol - 1 + (1 << uiK);
	xWriteEPExpGolombK0(uiTmp);
}
#endif


Void TEncSbac::xWriteTruncBinCode(UInt uiSymbol, UInt uiMaxSymbol)
{
  UInt uiThresh;
  if (uiMaxSymbol > 256)
  {
    UInt uiThreshVal = 1 << 8;
    uiThresh = 8;
    while (uiThreshVal <= uiMaxSymbol)
    {
      uiThresh++;
      uiThreshVal <<= 1;
    }
    uiThresh--;
  }
  else
  {
    uiThresh = g_uhPLTTBC[uiMaxSymbol];
  }

  UInt uiVal = 1 << uiThresh;
  assert(uiVal <= uiMaxSymbol);
  assert((uiVal << 1) > uiMaxSymbol);
  assert(uiSymbol < uiMaxSymbol);
  UInt b = uiMaxSymbol - uiVal;
  assert(b < uiVal);
  if (uiSymbol < uiVal - b)
  {
    m_pcBinIf->encodeBinsEP(uiSymbol, uiThresh);
  }
  else
  {
    uiSymbol += uiVal - b;
    assert(uiSymbol < (uiVal << 1));
    assert((uiSymbol >> 1) >= uiVal - b);
    m_pcBinIf->encodeBinsEP(uiSymbol, uiThresh + 1);
  }
}

Pel TEncSbac::writePLTIndex(UInt uiIdx, Pel *pLevel, Int iMaxSymbol, UChar *pSPoint, Int iWidth, UChar *pEscapeFlag)
{
  UInt uiTraIdx = m_puiScanOrder[uiIdx];  //unified position variable (raster scan)
#if SCM_T0065_PLT_IDX_GROUP
  Pel siCurLevel = pEscapeFlag[uiTraIdx] ? (iMaxSymbol - 1) : pLevel[uiTraIdx];
#else
  Pel siCurLevel = pLevel[uiTraIdx];
#endif
  if( pEscapeFlag[uiTraIdx] )
  {
    assert(siCurLevel == (iMaxSymbol-1));
  }

  if( uiIdx )
  {
    UInt uiTraIdxLeft = m_puiScanOrder[uiIdx - 1];
    if (pSPoint[uiTraIdxLeft] == PLT_RUN_LEFT)  ///< copy left
    {
      Pel siLeftLevel = pLevel[uiTraIdxLeft];
      if( pEscapeFlag[uiTraIdxLeft] )
      {
        siLeftLevel = iMaxSymbol - 1;
      }

      assert(siCurLevel != siLeftLevel);

      if (siCurLevel > siLeftLevel)
      {
        siCurLevel--;
      }
    }
    else
    {
      assert(uiTraIdxLeft >= iWidth);
      Pel siAboveLevel = pLevel[uiTraIdx - iWidth];
      if( pEscapeFlag[uiTraIdx - iWidth] )
      {
        siAboveLevel = iMaxSymbol - 1;
      }

      assert(siCurLevel != siAboveLevel);

      if (siCurLevel > siAboveLevel)
      {
        siCurLevel--;
      }
    }
    iMaxSymbol--;
  }
  assert(iMaxSymbol > 0);
  assert(siCurLevel >= 0);
  assert(iMaxSymbol > siCurLevel);
  if (iMaxSymbol > 1)
  {
    xWriteTruncBinCode((UInt)siCurLevel, iMaxSymbol);
  }
  return siCurLevel;
}

Void TEncSbac::xEncodePLTPredIndicator(UChar *bReusedPrev, UInt uiPLTSizePrev, UInt &uiNumPLTPredicted, UInt uiMaxPLTSize)
{
  Int lastPredIdx = -1;
  UInt run = 0;
  uiNumPLTPredicted = 0;

  for( UInt idx = 0; idx < uiPLTSizePrev; idx++ )
  {
    if( bReusedPrev[idx] )
    {
      uiNumPLTPredicted++;
      lastPredIdx = idx;
    }
  }

  Int idx = 0;
  while( idx <= lastPredIdx )
  {
    if( bReusedPrev[idx] )
    {
      xWriteEpExGolomb( run ? run + 1 : run, 0 );
      run = 0;
    }
    else
    {
      run++;
    }
    idx++;
  }
  if ((uiNumPLTPredicted < uiMaxPLTSize && lastPredIdx + 1 < uiPLTSizePrev) || !uiNumPLTPredicted)
  {
    xWriteEpExGolomb( 1, 0 );
  }
}

Void TEncSbac::encodeRun ( UInt uiRun, Bool bCopyTopMode, const UInt uiPltIdx, const UInt uiMaxRun )
{
  ContextModel *pcModel;
  UChar *ucCtxLut;
  if ( bCopyTopMode )
  {
    pcModel = m_cCopyTopRunSCModel.get(0);
    ucCtxLut = g_ucRunTopLut;
  }
  else
  {
    pcModel = m_cRunSCModel.get(0);
    ucCtxLut = g_ucRunLeftLut;
    g_ucRunLeftLut[0] = (uiPltIdx < SCM__S0269_PLT_RUN_MSB_IDX_CTX_T1 ? 0: (uiPltIdx < SCM__S0269_PLT_RUN_MSB_IDX_CTX_T2 ? 1 : 2));
  }
  xWriteTruncMsbP1RefinementBits( uiRun, pcModel, uiMaxRun, SCM__S0269_PLT_RUN_MSB_IDX_CABAC_BYPASS_THRE, ucCtxLut );
}

Void TEncSbac::encodeSPoint( TComDataCU *pcCU, UInt uiAbsPartIdx, UInt uiIdx, UInt uiWidth, UChar *pSPoint, UInt *uiRefScanOrder )
{
  if( uiRefScanOrder )
  {
    m_puiScanOrder = uiRefScanOrder;
  }

  UInt uiTraIdx = m_puiScanOrder[uiIdx];
#if !SCM_T0078_REMOVE_PLT_RUN_MODE_CTX
  UInt uiCtx = pcCU->getCtxSPoint( uiAbsPartIdx, uiTraIdx, pSPoint );
#endif

  if( uiTraIdx >= uiWidth && pSPoint[m_puiScanOrder[uiIdx - 1]] != PLT_RUN_ABOVE )
  {
    UInt mode = pSPoint[uiTraIdx];
#if SCM_T0078_REMOVE_PLT_RUN_MODE_CTX
    m_pcBinIf->encodeBin( mode, m_SPointSCModel.get( 0, 0, 0 ) );
#else
    m_pcBinIf->encodeBin( mode, m_SPointSCModel.get( 0, 0, uiCtx ) );
#endif
  }
}

Void TEncSbac::codePLTModeFlag(TComDataCU *pcCU, UInt uiAbsPartIdx)
{
  UInt uiSymbol = pcCU->getPLTModeFlag(uiAbsPartIdx);
  m_pcBinIf->encodeBin(uiSymbol, m_PLTModeFlagSCModel.get(0, 0, 0));
}

#if !SCM_T0064_REMOVE_PLT_SHARING
Void TEncSbac:: codePLTSharingModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getPLTSharingModeFlag(uiAbsPartIdx);
  m_pcBinIf->encodeBin( uiSymbol, m_PLTSharingModeFlagSCModel.get(0,0,0) );
}
#endif

Void TEncSbac::codePLTModeSyntax(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiNumComp)
{
  UInt uiIdx, uiDictMaxSize, uiDictIdxBits;
  UInt uiSampleBits[3];
  Pel *pLevel, *pPalette;
  TCoeff *pRun;
  Pel *pPixelValue[3];
#if SCM_T0072_T0109_T0120_PLT_NON444
  ComponentID compBegin = COMPONENT_Y;
#else
  ComponentID compBegin = ComponentID(uiNumComp == 2 ? 1 : 0);
#endif
  const UInt minCoeffSizeY = pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight();
  const UInt offsetY = minCoeffSizeY * uiAbsPartIdx;
  const UInt offset = offsetY >> (pcCU->getPic()->getComponentScaleX(compBegin) + pcCU->getPic()->getComponentScaleY(compBegin));
  UInt width = pcCU->getWidth(uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleX(compBegin);
  UInt height = pcCU->getHeight(uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleY(compBegin);
  UInt uiTotal = width * height;
#if SCM_T0072_T0109_T0120_PLT_NON444
  UInt uiScaleX = pcCU->getPic()->getComponentScaleX(COMPONENT_Cb);
  UInt uiScaleY = pcCU->getPic()->getComponentScaleY(COMPONENT_Cb);
  const UInt offsetC = offsetY >> (uiScaleX + uiScaleY);
#endif

  UInt uiRun = 0;
  uiIdx = 0;
  pLevel = pcCU->getLevel(compBegin) + offset;
  pRun = pcCU->getRun(compBegin) + offset;
  UChar* pSPoint = pcCU->getSPoint(compBegin) + offset;
  UChar* pEscapeFlag = pcCU->getEscapeFlag(compBegin) + offset;

  for (UInt comp = compBegin; comp < compBegin + uiNumComp; comp++)
  {
    uiSampleBits[comp] = pcCU->getSlice()->getSPS()->getBitDepth(toChannelType(ComponentID(comp)));
#if SCM_T0072_T0109_T0120_PLT_NON444
    if ( comp == compBegin )
    {
      pPixelValue[comp] = pcCU->getLevel( ComponentID( comp ) ) + offset;
    }
    else
    {
      pPixelValue[comp] = pcCU->getLevel( ComponentID( comp ) ) + offsetC;
    }
#else
    pPixelValue[comp] = pcCU->getLevel(ComponentID(comp)) + offset;
#endif
  }

  uiDictMaxSize = pcCU->getPLTSize(compBegin, uiAbsPartIdx);
  UInt uiMaxPLTSize = pcCU->getSlice()->getSPS()->getPLTMaxSize();
  assert(uiDictMaxSize <= uiMaxPLTSize);
  uiDictIdxBits = 0;
  while ((1 << uiDictIdxBits) < uiDictMaxSize)
  {
    uiDictIdxBits++;
  }
  UInt uiIndexMaxSize = uiDictMaxSize;
  UInt uiSignalEscape = pcCU->getPLTEscape(compBegin, uiAbsPartIdx);
  UInt uiDictIdxBitsExteneded = uiDictIdxBits;
  if (uiSignalEscape)
  {
    while ((1 << uiDictIdxBitsExteneded) <= uiDictMaxSize)
    {
      uiDictIdxBitsExteneded++;
    }
    uiIndexMaxSize++;
  }

#if !SCM_T0064_REMOVE_PLT_SHARING
  UInt uiPLTUsedSizePrev;
#endif
  UInt uiPLTSizePrev;
  //the bit depth depends on QP
  //calculate the bitLen needed to represent the quantized escape values
  UInt uiMaxVal[3];
  for (Int comp = compBegin; comp < compBegin + uiNumComp; comp++)
  {
    uiMaxVal[comp] = pcCU->xCalcMaxVals(pcCU, ComponentID(comp));
  }

#if !SCM_T0064_REMOVE_PLT_SHARING
  codePLTSharingModeFlag(pcCU, uiAbsPartIdx);
  Bool bUsePLTSharingMode = pcCU->getPLTSharingModeFlag(uiAbsPartIdx);
  if ( !bUsePLTSharingMode )
#endif
  {
#if SCM_T0064_REMOVE_PLT_SHARING
    pcCU->getPLTPred( pcCU, uiAbsPartIdx, compBegin, uiPLTSizePrev );
#else
    pcCU->getPLTPred( pcCU, uiAbsPartIdx, compBegin, uiPLTSizePrev, uiPLTUsedSizePrev );
#endif
    UChar *bReusedPrev;
    UInt uiNumPLTRceived = uiDictMaxSize, uiNumPLTPredicted = 0;

    bReusedPrev = pcCU->getPrevPLTReusedFlag( compBegin, uiAbsPartIdx );

    if ( uiPLTSizePrev )
    {
      xEncodePLTPredIndicator( bReusedPrev, uiPLTSizePrev, uiNumPLTPredicted, uiMaxPLTSize);
    }

    assert( uiDictMaxSize >= uiNumPLTPredicted );
    if ( uiNumPLTPredicted < uiMaxPLTSize)
    {
      uiNumPLTRceived = uiDictMaxSize - uiNumPLTPredicted;
#if SCM_T0063_NUM_PLT_ENTRY
      xWriteEpExGolomb(uiNumPLTRceived, 0);
#else
      for ( UInt uiPLTIdx = 0; uiPLTIdx <= uiNumPLTRceived; uiPLTIdx++ )
      {
        if ( uiNumPLTPredicted + uiPLTIdx < uiMaxPLTSize)
        {
          m_pcBinIf->encodeBinEP( uiPLTIdx == (uiNumPLTRceived) );
        }
      }
#endif
    }
    for ( UInt comp = compBegin; comp < compBegin + uiNumComp; comp++ )
    {
      pPalette = pcCU->getPLT( comp, uiAbsPartIdx );
      for ( UInt uiPLTIdx = uiNumPLTPredicted; uiPLTIdx < uiDictMaxSize; uiPLTIdx++ )
      {
        m_pcBinIf->encodeBinsEP( (UInt)pPalette[uiPLTIdx], uiSampleBits[comp] );
      }
    }
  }

  m_puiScanOrder = g_scanOrder[SCAN_UNGROUPED][SCAN_TRAV][g_aucConvertToBit[width]+2][g_aucConvertToBit[height]+2];
  if (uiDictMaxSize > 0)
  {
    m_pcBinIf->encodeBinEP(uiSignalEscape);
    if (uiDictMaxSize + uiSignalEscape > 1)
    {
      codeScanRotationModeFlag(pcCU, uiAbsPartIdx);
    }
    else
    {
      assert(!pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx));
      assert(!uiSignalEscape);
    }
  }
  else
  {
    assert(!pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx));
  }
#if SCM_T0065_PLT_IDX_GROUP
  Int iLastRunPos = -1;
  UInt lastRunType = 0;
  UInt uiNumIndices = 0;
  std::list<Int> lIdxPosList, lParsedIdxList;
  if (uiIndexMaxSize > 1)
  {
    uiIdx = 0;
    while (uiIdx < uiTotal)
    {
      UInt uiTraIdx = m_puiScanOrder[uiIdx];
      if (pSPoint[uiTraIdx] == PLT_RUN_LEFT)
      {
        lIdxPosList.push_back(uiIdx);
        uiNumIndices++;
      }
      lastRunType = pSPoint[uiTraIdx];
      iLastRunPos = uiIdx;
      uiRun = pRun[uiTraIdx];
      uiIdx += uiRun;
      uiIdx++;
    }

    UInt uiCurrParam = 2 + uiIndexMaxSize / 6;
    UInt uiMappedValue;
#if SCM_T0064_REMOVE_PLT_SHARING
    Bool bUsePLTSharingMode = false;
#endif
    assert(uiNumIndices);
    UInt uiInterval = bUsePLTSharingMode ? 8 : 32;
    UInt uiZeroPosition = bUsePLTSharingMode ? 3 : uiIndexMaxSize;

    if (uiNumIndices >= uiZeroPosition)
    {
      Int iNumResPos = uiZeroPosition - 1;
      UInt uiValue = uiNumIndices - uiZeroPosition;
      uiMappedValue = uiValue < iNumResPos * (uiInterval - 1) ?
        uiValue / (uiInterval - 1) * uiInterval + uiValue % (uiInterval - 1) : uiValue + iNumResPos;
    }
    else
    {
      UInt uiValue = uiZeroPosition - uiNumIndices;
      uiMappedValue = uiValue * uiInterval - 1;
    }
    xWriteCoefRemainExGolomb(uiMappedValue, uiCurrParam, false, MAX_NUM_CHANNEL_TYPE);

    while (!lIdxPosList.empty())
    {
      uiIdx = lIdxPosList.front();
      lIdxPosList.pop_front();
      lParsedIdxList.push_back(writePLTIndex(uiIdx, pLevel, uiIndexMaxSize, pSPoint, width, pEscapeFlag));
    }
    m_pcBinIf->encodeBin(lastRunType, m_PLTLastRunTypeSCModel.get(0, 0, 0));
  }

  uiIdx = 0;
#endif
  while ( uiIdx < uiTotal )
  {
#if !SCM_T0078_REMOVE_PLT_RUN_MODE_CTX
    UInt uiCtx = 0;
#endif
    UInt uiTraIdx = m_puiScanOrder[uiIdx];  //unified position variable (raster scan)
    if (uiIndexMaxSize > 1)
    {
#if !SCM_T0078_REMOVE_PLT_RUN_MODE_CTX
      uiCtx = pcCU->getCtxSPoint( uiAbsPartIdx, uiTraIdx, pSPoint );
#endif
      if ( uiTraIdx >= width && pSPoint[m_puiScanOrder[uiIdx - 1]] != PLT_RUN_ABOVE )
      {
        UInt mode = pSPoint[uiTraIdx];
#if SCM_T0065_PLT_IDX_GROUP
        if ( uiNumIndices && uiIdx < uiTotal - 1 )
        {
#endif
#if SCM_T0078_REMOVE_PLT_RUN_MODE_CTX
          m_pcBinIf->encodeBin( mode, m_SPointSCModel.get( 0, 0, 0 ) );
#else
          m_pcBinIf->encodeBin( mode, m_SPointSCModel.get( 0, 0, uiCtx ) );
#endif
#if SCM_T0065_PLT_IDX_GROUP
        }
#endif
      }
    }
    Pel siCurLevel = 0;
    {
      if ( pSPoint[uiTraIdx] == PLT_RUN_LEFT )
      {
        UInt uiRealLevel = pLevel[uiTraIdx];
        if( pEscapeFlag[uiTraIdx] )
        {
          pLevel[uiTraIdx] = uiIndexMaxSize - 1;
        }
#if SCM_T0065_PLT_IDX_GROUP
        if (!lParsedIdxList.empty())
        {
          siCurLevel = lParsedIdxList.front();
          lParsedIdxList.pop_front();
        }
        else
        {
          siCurLevel = 0;
        }
#else
        siCurLevel = writePLTIndex( uiIdx, pLevel, uiIndexMaxSize, pSPoint, width, pEscapeFlag );
#endif
        if( pEscapeFlag[uiTraIdx] )
        {
          pLevel[uiTraIdx] = uiRealLevel;
        }
      }
#if SCM_HIGH_BIT_DEPTH_BUG_FIX
      uiRun = (UInt)pRun[uiTraIdx];
#else
      uiRun = pRun[uiTraIdx];
#endif 
      if ( uiIndexMaxSize > 1 )
      {
#if SCM_T0065_PLT_IDX_GROUP
        if (iLastRunPos != uiIdx)
        {
          uiNumIndices -= (pSPoint[uiTraIdx] == PLT_RUN_LEFT);
          encodeRun(uiRun, pSPoint[uiTraIdx], siCurLevel, uiTotal - uiNumIndices - uiIdx - 1);
        }
#else
        encodeRun( uiRun, pSPoint[uiTraIdx], siCurLevel, uiTotal - uiIdx - 1 );
#endif
      }

#if !SCM_S0181_S0150_GROUP_ESCAPE_COLOR_AT_END
      for(UInt uiRunIdx = 0; uiRunIdx <= uiRun; uiRunIdx++, uiIdx++)
      {
        if( pEscapeFlag[m_puiScanOrder[uiIdx]] )
        {
#if SCM_T0072_T0109_T0120_PLT_NON444
          UInt uiY, uiX;
          uiTraIdx = m_puiScanOrder[uiIdx];
          uiY = uiTraIdx/width;
          uiX = uiTraIdx%width;
          UInt uiXC, uiYC, uiTraIdxC;
          if(!pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx))
          {
            uiXC = (uiX>>uiScaleX);
            uiYC = (uiY>>uiScaleY);
            uiTraIdxC = uiYC * (width>>uiScaleX) + uiXC;
          }
          else
          {
            uiXC = (uiX>>uiScaleY);
            uiYC = (uiY>>uiScaleX);
            uiTraIdxC = uiYC * (height>>uiScaleY) + uiXC;  
          }
          
          if(
              pcCU->getPic()->getChromaFormat() == CHROMA_444 ||
              ( pcCU->getPic()->getChromaFormat() == CHROMA_420 && ((uiX&1) == 0) && ((uiY&1) == 0)) ||
              ( pcCU->getPic()->getChromaFormat() == CHROMA_422 && ((!pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx) && ((uiX&1) == 0)) || (pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx) && ((uiY&1) == 0))) )
            )
          {
            for ( UInt comp = compBegin; comp < compBegin + uiNumComp; comp++ )
            {
              if(comp == compBegin)
                xWriteTruncBinCode( (UInt)pPixelValue[comp][uiTraIdx], uiMaxVal[comp] + 1 );
              else
                xWriteTruncBinCode( (UInt)pPixelValue[comp][uiTraIdxC], uiMaxVal[comp] + 1 );
            }
          }
          else
          {
            xWriteTruncBinCode( (UInt)pPixelValue[compBegin][uiTraIdx], uiMaxVal[compBegin] + 1 );
          }
#else
          for ( UInt comp = compBegin; comp < compBegin + uiNumComp; comp++ )
          {
            xWriteTruncBinCode( (UInt)pPixelValue[comp][m_puiScanOrder[uiIdx]], uiMaxVal[comp] + 1 );
          }
#endif
        }
      }
#else
      uiIdx += (uiRun + 1);    
#endif
    }
  }
  assert(uiIdx == uiTotal);

#if SCM_S0181_S0150_GROUP_ESCAPE_COLOR_AT_END
  for( uiIdx = 0; uiIdx < uiTotal; uiIdx ++ )
  {
    UInt uiTraIdx = m_puiScanOrder[uiIdx];
    if( pEscapeFlag[uiTraIdx] )
    {
#if SCM_T0072_T0109_T0120_PLT_NON444
      UInt uiY, uiX;
      uiY = uiTraIdx/width;
      uiX = uiTraIdx%width;
      UInt uiXC, uiYC, uiTraIdxC;
      if(!pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx))
      {
        uiXC = (uiX>>uiScaleX);
        uiYC = (uiY>>uiScaleY);
        uiTraIdxC = uiYC * (width>>uiScaleX) + uiXC;
      }
      else
      {
        uiXC = (uiX>>uiScaleY);
        uiYC = (uiY>>uiScaleX);
        uiTraIdxC = uiYC * (height>>uiScaleY) + uiXC;
      }

      if(   pcCU->getPic()->getChromaFormat() == CHROMA_444 ||
          ( pcCU->getPic()->getChromaFormat() == CHROMA_420 && ((uiX&1) == 0) && ((uiY&1) == 0)) ||
          ( pcCU->getPic()->getChromaFormat() == CHROMA_422 && ((!pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx) && ((uiX&1) == 0)) || (pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx) && ((uiY&1) == 0))) )
        )
      {
        for ( UInt comp = compBegin; comp < compBegin + uiNumComp; comp++ )
        {
          if ( comp == compBegin )
          {
            xWriteTruncBinCode( (UInt)pPixelValue[comp][uiTraIdx], uiMaxVal[comp] + 1 );
          }
          else
          {
            xWriteTruncBinCode( (UInt)pPixelValue[comp][uiTraIdxC], uiMaxVal[comp] + 1 );
          }
        }
      }
      else
      {
        xWriteTruncBinCode( (UInt)pPixelValue[compBegin][uiTraIdx], uiMaxVal[compBegin] + 1 );
      }
#else
      for ( UInt comp = compBegin; comp < compBegin + uiNumComp; comp++ )
      {
        xWriteTruncBinCode( (UInt)pPixelValue[comp][m_puiScanOrder[uiIdx]], uiMaxVal[comp] + 1 );
      }
#endif
    }
  }
#endif
}

Void TEncSbac::codeScanRotationModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getPLTScanRotationModeFlag(uiAbsPartIdx);
  UInt uiCtx = 0;
  m_pcBinIf->encodeBin( uiSymbol, m_PLTScanRotationModeFlagSCModel.get( 0, 0, uiCtx ) );
}


/** Coding of coeff_abs_level_minus3
 * \param symbol                  value of coeff_abs_level_minus3
 * \param rParam                  reference to Rice parameter
 * \param useLimitedPrefixLength
 * \param maxLog2TrDynamicRange 
 */
Void TEncSbac::xWriteCoefRemainExGolomb ( UInt symbol, UInt &rParam, const Bool useLimitedPrefixLength, const Int maxLog2TrDynamicRange )
{
  Int codeNumber  = (Int)symbol;
  UInt length;

  if (codeNumber < (COEF_REMAIN_BIN_REDUCTION << rParam))
  {
    length = codeNumber>>rParam;
    m_pcBinIf->encodeBinsEP( (1<<(length+1))-2 , length+1);
    m_pcBinIf->encodeBinsEP((codeNumber%(1<<rParam)),rParam);
  }
  else if (useLimitedPrefixLength)
  {
    const UInt maximumPrefixLength = (32 - (COEF_REMAIN_BIN_REDUCTION + maxLog2TrDynamicRange));

    UInt prefixLength = 0;
    UInt suffixLength = MAX_UINT;
    UInt codeValue    = (symbol >> rParam) - COEF_REMAIN_BIN_REDUCTION;

    if (codeValue >= ((1 << maximumPrefixLength) - 1))
    {
      prefixLength = maximumPrefixLength;
      suffixLength = maxLog2TrDynamicRange - rParam;
    }
    else
    {
      while (codeValue > ((2 << prefixLength) - 2))
      {
        prefixLength++;
      }

      suffixLength = prefixLength + 1; //+1 for the separator bit
    }

    const UInt suffix = codeValue - ((1 << prefixLength) - 1);

    const UInt totalPrefixLength = prefixLength + COEF_REMAIN_BIN_REDUCTION;
    const UInt prefix            = (1 << totalPrefixLength) - 1;
    const UInt rParamBitMask     = (1 << rParam) - 1;

    m_pcBinIf->encodeBinsEP(  prefix,                                        totalPrefixLength      ); //prefix
    m_pcBinIf->encodeBinsEP(((suffix << rParam) | (symbol & rParamBitMask)), (suffixLength + rParam)); //separator, suffix, and rParam bits
  }
  else
  {
    length = rParam;
    codeNumber  = codeNumber - ( COEF_REMAIN_BIN_REDUCTION << rParam);

    while (codeNumber >= (1<<length))
    {
      codeNumber -=  (1<<(length++));
    }

    m_pcBinIf->encodeBinsEP((1<<(COEF_REMAIN_BIN_REDUCTION+length+1-rParam))-2,COEF_REMAIN_BIN_REDUCTION+length+1-rParam);
    m_pcBinIf->encodeBinsEP(codeNumber,length);
  }
}

// SBAC RD
Void  TEncSbac::load ( const TEncSbac* pSrc)
{
  this->xCopyFrom(pSrc);
}

Void  TEncSbac::loadIntraDirMode( const TEncSbac* pSrc, const ChannelType chType )
{
  m_pcBinIf->copyState( pSrc->m_pcBinIf );
  if (isLuma(chType))
  {
    this->m_cCUIntraPredSCModel      .copyFrom( &pSrc->m_cCUIntraPredSCModel       );
  }
  else
  {
    this->m_cCUChromaPredSCModel     .copyFrom( &pSrc->m_cCUChromaPredSCModel      );
  }
}


Void  TEncSbac::store( TEncSbac* pDest) const
{
  pDest->xCopyFrom( this );
}


Void TEncSbac::xCopyFrom( const TEncSbac* pSrc )
{
  m_pcBinIf->copyState( pSrc->m_pcBinIf );
  xCopyContextsFrom(pSrc);
}

Void TEncSbac::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iSymbol = pcCU->getMVPIdx(eRefList, uiAbsPartIdx);
  Int iNum = AMVP_MAX_NUM_CANDS;

  xWriteUnaryMaxSymbol(iSymbol, m_cMVPIdxSCModel.get(0), 1, iNum-1);
}

Void TEncSbac::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
  assert( !pcCU->isIntraBC( uiAbsPartIdx ) );
#endif

  PartSize eSize         = pcCU->getPartitionSize( uiAbsPartIdx );
  const UInt log2DiffMaxMinCodingBlockSize = pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize();

  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
    if( uiDepth == log2DiffMaxMinCodingBlockSize )
    {
      m_pcBinIf->encodeBin( eSize == SIZE_2Nx2N? 1 : 0, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
    }
    return;
  }

  switch(eSize)
  {
    case SIZE_2Nx2N:
    {
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      break;
    }
    case SIZE_2NxN:
    case SIZE_2NxnU:
    case SIZE_2NxnD:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 1) );
      if ( pcCU->getSlice()->getSPS()->getUseAMP() && uiDepth < log2DiffMaxMinCodingBlockSize )
      {
        if (eSize == SIZE_2NxN)
        {
          m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
        }
        else
        {
          m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
          m_pcBinIf->encodeBinEP((eSize == SIZE_2NxnU? 0: 1));
        }
      }
      break;
    }
    case SIZE_Nx2N:
    case SIZE_nLx2N:
    case SIZE_nRx2N:
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );

      if( uiDepth == log2DiffMaxMinCodingBlockSize && !( pcCU->getWidth(uiAbsPartIdx) == 8 && pcCU->getHeight(uiAbsPartIdx) == 8 ) )
      {
        m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 2) );
      }

      if ( pcCU->getSlice()->getSPS()->getUseAMP() && uiDepth < log2DiffMaxMinCodingBlockSize )
      {
        if (eSize == SIZE_Nx2N)
        {
          m_pcBinIf->encodeBin(1, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
        }
        else
        {
          m_pcBinIf->encodeBin(0, m_cCUPartSizeSCModel.get( 0, 0, 3 ));
          m_pcBinIf->encodeBinEP((eSize == SIZE_nLx2N? 0: 1));
        }
      }
      break;
    }
    case SIZE_NxN:
    {
      if( uiDepth == log2DiffMaxMinCodingBlockSize && !( pcCU->getWidth(uiAbsPartIdx) == 8 && pcCU->getHeight(uiAbsPartIdx) == 8 ) )
      {
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0) );
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 1) );
        m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 2) );
      }
      break;
    }
    default:
    {
      assert(0);
      break;
    }
  }
}

#if !SCM_T0227_INTRABC_SIG_UNIFICATION
Void TEncSbac::codePartSizeIntraBC( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const UInt uiSymbol = pcCU->getPartitionSize(uiAbsPartIdx);
  if( pcCU->getDepth(uiAbsPartIdx) == pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize() )
  {
    // size can be 2Nx2N, NxN, 2NxN, Nx2N
    if( uiSymbol == SIZE_2Nx2N )
    {
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
    }
    else
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
      m_pcBinIf->encodeBin( ( uiSymbol == SIZE_2NxN ? 1 : 0 ), m_cCUPartSizeSCModel.get( 0, 0, 1 ) );
      if( uiSymbol != SIZE_2NxN )
      {
        m_pcBinIf->encodeBin( ( uiSymbol == SIZE_Nx2N ? 1 : 0 ), m_cCUPartSizeSCModel.get( 0, 0, 2 ) );
      }
    }
  }
  else
  {
    // size can be 2Nx2N, 2NxN, Nx2N
    if( uiSymbol == SIZE_2Nx2N )
    {
      m_pcBinIf->encodeBin( 1, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
    }
    else
    {
      m_pcBinIf->encodeBin( 0, m_cCUPartSizeSCModel.get( 0, 0, 0 ) );
      m_pcBinIf->encodeBin( ( uiSymbol == SIZE_Nx2N ? 0 : 1 ), m_cCUPartSizeSCModel.get( 0, 0, 1 ) );
    }
  }
}
#endif

/** code prediction mode
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
#if !SCM_T0227_INTRABC_SIG_UNIFICATION  
  assert(!pcCU->isIntraBC(uiAbsPartIdx));
#endif
  m_pcBinIf->encodeBin( pcCU->isIntra( uiAbsPartIdx ) ? 1 : 0, m_cCUPredModeSCModel.get( 0, 0, 0 ) );
}

Void TEncSbac::codeCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiSymbol = pcCU->getCUTransquantBypass(uiAbsPartIdx);
  m_pcBinIf->encodeBin( uiSymbol, m_CUTransquantBypassFlagSCModel.get( 0, 0, 0 ) );
}

/** code skip flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  UInt uiSymbol = pcCU->isSkipped( uiAbsPartIdx ) ? 1 : 0;
  UInt uiCtxSkip = pcCU->getCtxSkipFlag( uiAbsPartIdx ) ;
  m_pcBinIf->encodeBin( uiSymbol, m_cCUSkipFlagSCModel.get( 0, 0, uiCtxSkip ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tSkipFlag" );
  DTRACE_CABAC_T( "\tuiCtxSkip: ");
  DTRACE_CABAC_V( uiCtxSkip );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
}

/** code merge flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeMergeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const UInt uiSymbol = pcCU->getMergeFlag( uiAbsPartIdx ) ? 1 : 0;
  m_pcBinIf->encodeBin( uiSymbol, *m_cCUMergeFlagExtSCModel.get( 0 ) );

  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tMergeFlag: " );
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\tAddress: " );
  DTRACE_CABAC_V( pcCU->getCtuRsAddr() );
  DTRACE_CABAC_T( "\tuiAbsPartIdx: " );
  DTRACE_CABAC_V( uiAbsPartIdx );
  DTRACE_CABAC_T( "\n" );
}

/** code merge index
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeMergeIndex( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiUnaryIdx = pcCU->getMergeIndex( uiAbsPartIdx );
  UInt uiNumCand = pcCU->getSlice()->getMaxNumMergeCand();
  if ( uiNumCand > 1 )
  {
    for( UInt ui = 0; ui < uiNumCand - 1; ++ui )
    {
      const UInt uiSymbol = ui == uiUnaryIdx ? 0 : 1;
      if ( ui==0 )
      {
        m_pcBinIf->encodeBin( uiSymbol, m_cCUMergeIdxExtSCModel.get( 0, 0, 0 ) );
      }
      else
      {
        m_pcBinIf->encodeBinEP( uiSymbol );
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
  }
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tparseMergeIndex()" );
  DTRACE_CABAC_T( "\tuiMRGIdx= " );
  DTRACE_CABAC_V( pcCU->getMergeIndex( uiAbsPartIdx ) );
  DTRACE_CABAC_T( "\n" );
}

Void TEncSbac::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == pcCU->getSlice()->getSPS()->getLog2DiffMaxMinCodingBlockSize() )
  {
    return;
  }

  UInt uiCtx           = pcCU->getCtxSplitFlag( uiAbsPartIdx, uiDepth );
  UInt uiCurrSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;

  assert( uiCtx < 3 );
  m_pcBinIf->encodeBin( uiCurrSplitFlag, m_cCUSplitFlagSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tSplitFlag\n" )
  return;
}

Void TEncSbac::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  m_pcBinIf->encodeBin( uiSymbol, m_cCUTransSubdivFlagSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseTransformSubdivFlag()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiSymbol )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\n" )
}


Void TEncSbac::codeIntraDirLumaAng( TComDataCU* pcCU, UInt absPartIdx, Bool isMultiple)
{
  UInt dir[4],j;
  Int preds[4][NUM_MOST_PROBABLE_MODES] = {{-1, -1, -1},{-1, -1, -1},{-1, -1, -1},{-1, -1, -1}};
  Int predIdx[4] ={ -1,-1,-1,-1};
  PartSize mode = pcCU->getPartitionSize( absPartIdx );
  UInt partNum = isMultiple?(mode==SIZE_NxN?4:1):1;
  UInt partOffset = ( pcCU->getPic()->getNumPartitionsInCtu() >> ( pcCU->getDepth(absPartIdx) << 1 ) ) >> 2;
  for (j=0;j<partNum;j++)
  {
    dir[j] = pcCU->getIntraDir( CHANNEL_TYPE_LUMA, absPartIdx+partOffset*j );
    pcCU->getIntraDirPredictor(absPartIdx+partOffset*j, preds[j], COMPONENT_Y);
    for(UInt i = 0; i < NUM_MOST_PROBABLE_MODES; i++)
    {
      if(dir[j] == preds[j][i])
      {
        predIdx[j] = i;
      }
    }
    m_pcBinIf->encodeBin((predIdx[j] != -1)? 1 : 0, m_cCUIntraPredSCModel.get( 0, 0, 0 ) );
  }
  for (j=0;j<partNum;j++)
  {
    if(predIdx[j] != -1)
    {
      m_pcBinIf->encodeBinEP( predIdx[j] ? 1 : 0 );
      if (predIdx[j])
      {
        m_pcBinIf->encodeBinEP( predIdx[j]-1 );
      }
    }
    else
    {
      if (preds[j][0] > preds[j][1])
      {
        std::swap(preds[j][0], preds[j][1]);
      }
      if (preds[j][0] > preds[j][2])
      {
        std::swap(preds[j][0], preds[j][2]);
      }
      if (preds[j][1] > preds[j][2])
      {
        std::swap(preds[j][1], preds[j][2]);
      }
      for(Int i = (Int(NUM_MOST_PROBABLE_MODES) - 1); i >= 0; i--)
      {
        dir[j] = dir[j] > preds[j][i] ? dir[j] - 1 : dir[j];
      }
      m_pcBinIf->encodeBinsEP( dir[j], 5 );
    }
  }
  return;
}

Void TEncSbac::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIntraDirChroma = pcCU->getIntraDir( CHANNEL_TYPE_CHROMA, uiAbsPartIdx );

  if( uiIntraDirChroma == DM_CHROMA_IDX )
  {
    m_pcBinIf->encodeBin( 0, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cCUChromaPredSCModel.get( 0, 0, 0 ) );

    UInt uiAllowedChromaDir[ NUM_CHROMA_MODE ];
    pcCU->getAllowedChromaDir( uiAbsPartIdx, uiAllowedChromaDir );

    for( Int i = 0; i < NUM_CHROMA_MODE - 1; i++ )
    {
      if( uiIntraDirChroma == uiAllowedChromaDir[i] )
      {
        uiIntraDirChroma = i;
        break;
      }
    }

    m_pcBinIf->encodeBinsEP( uiIntraDirChroma, 2 );
  }

  return;
}

#if !SCM_T0227_INTRABC_SIG_UNIFICATION
/** code intraBC flag
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeIntraBCFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  // get context function is here
  const UInt uiSymbol = pcCU->isIntraBC( uiAbsPartIdx ) ? 1 : 0;

  m_pcBinIf->encodeBin(uiSymbol, m_cIntraBCPredFlagSCModel.get( 0, 0, 0 ));

  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tuiSymbol: ");
  DTRACE_CABAC_V( uiSymbol );
  DTRACE_CABAC_T( "\n");
}

/** code intraBC
 * \param pcCU
 * \param uiAbsPartIdx
 * \returns Void
 */
Void TEncSbac::codeIntraBC( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const PartSize ePartSize = pcCU->getPartitionSize( uiAbsPartIdx );
  const UInt iNumPart = pcCU->getNumPartitions( uiAbsPartIdx );
  const UInt uiPUOffset = ( g_auiPUOffset[UInt( ePartSize )] << ( ( pcCU->getSlice()->getSPS()->getMaxTotalCUDepth() - pcCU->getDepth(uiAbsPartIdx) ) << 1 ) ) >> 4;

  for(UInt iPartIdx = 0; iPartIdx < iNumPart; iPartIdx ++)
  {
    codeIntraBCBvd(pcCU, uiAbsPartIdx + iPartIdx * uiPUOffset, REF_PIC_LIST_INTRABC);
    Int iSymbol = pcCU->getMVPIdx(REF_PIC_LIST_INTRABC, uiAbsPartIdx + iPartIdx * uiPUOffset);
    xWriteUnaryMaxSymbol(iSymbol, m_cMVPIdxSCModel.get(0), 1, AMVP_MAX_NUM_CANDS-1);
  }
}

Void TEncSbac::codeIntraBCBvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{ 
  const TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  
  const Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  const Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();

  ContextModel* pCtx = m_cIntraBCBVDSCModel.get( 0 );
  
  m_pcBinIf->encodeBin( iHor != 0 ? 1 : 0, *pCtx );
  
  pCtx++;
  m_pcBinIf->encodeBin( iVer != 0 ? 1 : 0, *pCtx );
  
  const Bool bHorAbsGr0 = iHor != 0;
  const Bool bVerAbsGr0 = iVer != 0;
  const UInt uiHorAbs   = 0 > iHor ? -iHor : iHor;
  const UInt uiVerAbs   = 0 > iVer ? -iVer : iVer;
  
  if( bHorAbsGr0 )
  {
    xWriteEpExGolomb( uiHorAbs-1, INTRABC_BVD_CODING_EGORDER );
    m_pcBinIf->encodeBinEP( 0 > iHor ? 1 : 0 );
  }
  
  if( bVerAbsGr0 )
  {
    xWriteEpExGolomb( uiVerAbs-1, INTRABC_BVD_CODING_EGORDER );
    m_pcBinIf->encodeBinEP( 0 > iVer ? 1 : 0 );
  }
  return;
}

Void  TEncSbac::estBvdBin0Cost(Int *Bin0Cost)
{
  ContextModel* pCtx = m_cIntraBCBVDSCModel.get( 0 );
  Bin0Cost[0] = pCtx->getEntropyBits(0);
  Bin0Cost[1] = pCtx->getEntropyBits(1);
  pCtx++;
  Bin0Cost[2] = pCtx->getEntropyBits(0);
  Bin0Cost[3] = pCtx->getEntropyBits(1);
}
#endif

Void TEncSbac::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  const UInt uiInterDir = pcCU->getInterDir( uiAbsPartIdx ) - 1;
  const UInt uiCtx      = pcCU->getCtxInterDir( uiAbsPartIdx );
  ContextModel *pCtx    = m_cCUInterDirSCModel.get( 0 );

  if (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N || pcCU->getHeight(uiAbsPartIdx) != 8 )
  {
    m_pcBinIf->encodeBin( uiInterDir == 2 ? 1 : 0, *( pCtx + uiCtx ) );
  }

  if (uiInterDir < 2)
  {
    m_pcBinIf->encodeBin( uiInterDir, *( pCtx + 4 ) );
  }

  return;
}

Void TEncSbac::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
  ContextModel *pCtx = m_cCURefPicSCModel.get( 0 );
  m_pcBinIf->encodeBin( ( iRefFrame == 0 ? 0 : 1 ), *pCtx );

  if( iRefFrame > 0 )
  {
    UInt uiRefNum = pcCU->getSlice()->getNumRefIdx( eRefList ) - 2;
    pCtx++;
    iRefFrame--;
    for( UInt ui = 0; ui < uiRefNum; ++ui )
    {
      const UInt uiSymbol = ui == iRefFrame ? 0 : 1;
      if( ui == 0 )
      {
        m_pcBinIf->encodeBin( uiSymbol, *pCtx );
      }
      else
      {
        m_pcBinIf->encodeBinEP( uiSymbol );
      }
      if( uiSymbol == 0 )
      {
        break;
      }
    }
  }
  return;
}

Void TEncSbac::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  if(pcCU->getSlice()->getMvdL1ZeroFlag() && eRefList == REF_PIC_LIST_1 && pcCU->getInterDir(uiAbsPartIdx)==3)
  {
    return;
  }

  const TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  const Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  const Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();
  ContextModel* pCtx = m_cCUMvdSCModel.get( 0 );

  m_pcBinIf->encodeBin( iHor != 0 ? 1 : 0, *pCtx );
  m_pcBinIf->encodeBin( iVer != 0 ? 1 : 0, *pCtx );

  const Bool bHorAbsGr0 = iHor != 0;
  const Bool bVerAbsGr0 = iVer != 0;
  const UInt uiHorAbs   = 0 > iHor ? -iHor : iHor;
  const UInt uiVerAbs   = 0 > iVer ? -iVer : iVer;
  pCtx++;

  if( bHorAbsGr0 )
  {
    m_pcBinIf->encodeBin( uiHorAbs > 1 ? 1 : 0, *pCtx );
  }

  if( bVerAbsGr0 )
  {
    m_pcBinIf->encodeBin( uiVerAbs > 1 ? 1 : 0, *pCtx );
  }

  if( bHorAbsGr0 )
  {
    if( uiHorAbs > 1 )
    {
      xWriteEpExGolomb( uiHorAbs-2, 1 );
    }

    m_pcBinIf->encodeBinEP( 0 > iHor ? 1 : 0 );
  }

  if( bVerAbsGr0 )
  {
    if( uiVerAbs > 1 )
    {
      xWriteEpExGolomb( uiVerAbs-2, 1 );
    }

    m_pcBinIf->encodeBinEP( 0 > iVer ? 1 : 0 );
  }

  return;
}

Void TEncSbac::codeCrossComponentPrediction( TComTU &rTu, ComponentID compID )
{
  TComDataCU *pcCU = rTu.getCU();

  if( isLuma(compID) || !pcCU->getSlice()->getPPS()->getUseCrossComponentPrediction() )
  {
    return;
  }

  const UInt uiAbsPartIdx = rTu.GetAbsPartIdxTU();

  if (!pcCU->isIntra(uiAbsPartIdx) || (pcCU->getIntraDir( CHANNEL_TYPE_CHROMA, uiAbsPartIdx ) == DM_CHROMA_IDX))
  {
    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T("\tparseCrossComponentPrediction()")
    DTRACE_CABAC_T( "\tAddr=" )
    DTRACE_CABAC_V( compID )
    DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
    DTRACE_CABAC_V( uiAbsPartIdx )

    Int alpha = pcCU->getCrossComponentPredictionAlpha( uiAbsPartIdx, compID );
    ContextModel *pCtx = m_cCrossComponentPredictionSCModel.get(0, 0) + ((compID == COMPONENT_Cr) ? (NUM_CROSS_COMPONENT_PREDICTION_CTX >> 1) : 0);
    m_pcBinIf->encodeBin(((alpha != 0) ? 1 : 0), pCtx[0]);

    if (alpha != 0)
    {
      static const Int log2AbsAlphaMinus1Table[8] = { 0, 1, 1, 2, 2, 2, 3, 3 };
      assert(abs(alpha) <= 8);

      if (abs(alpha)>1)
      {
        m_pcBinIf->encodeBin(1, pCtx[1]);
        xWriteUnaryMaxSymbol( log2AbsAlphaMinus1Table[abs(alpha) - 1] - 1, (pCtx + 2), 1, 2 );
      }
      else
      {
        m_pcBinIf->encodeBin(0, pCtx[1]);
      }
      m_pcBinIf->encodeBin( ((alpha < 0) ? 1 : 0), pCtx[4] );
    }
    DTRACE_CABAC_T( "\tAlpha=" )
    DTRACE_CABAC_V( pcCU->getCrossComponentPredictionAlpha( uiAbsPartIdx, compID ) )
    DTRACE_CABAC_T( "\n" )
  }
}

Void TEncSbac::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getRefQP( uiAbsPartIdx );

  Int qpBdOffsetY =  pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA);
  iDQp = (iDQp + 78 + qpBdOffsetY + (qpBdOffsetY/2)) % (52 + qpBdOffsetY) - 26 - (qpBdOffsetY/2);

  UInt uiAbsDQp = (UInt)((iDQp > 0)? iDQp  : (-iDQp));
  UInt TUValue = min((Int)uiAbsDQp, CU_DQP_TU_CMAX);
  xWriteUnaryMaxSymbol( TUValue, &m_cCUDeltaQpSCModel.get( 0, 0, 0 ), 1, CU_DQP_TU_CMAX);
  if( uiAbsDQp >= CU_DQP_TU_CMAX )
  {
    xWriteEpExGolomb( uiAbsDQp - CU_DQP_TU_CMAX, CU_DQP_EG_k );
  }

  if ( uiAbsDQp > 0)
  {
    UInt uiSign = (iDQp > 0 ? 0 : 1);
    m_pcBinIf->encodeBinEP(uiSign);
  }

  return;
}

/** code chroma qp adjustment, converting from the internal table representation
 * \returns Void
 */
Void TEncSbac::codeChromaQpAdjustment( TComDataCU* cu, UInt absPartIdx )
{
  Int internalIdc = cu->getChromaQpAdj( absPartIdx );
  Int tableSize = cu->getSlice()->getPPS()->getChromaQpAdjTableSize();
  /* internal_idc == 0 => flag = 0
   * internal_idc > 1 => code idc value (if table size warrents) */
  m_pcBinIf->encodeBin( internalIdc > 0, m_ChromaQpAdjFlagSCModel.get( 0, 0, 0 ) );

  if (internalIdc > 0 && tableSize > 1)
  {
    xWriteUnaryMaxSymbol( internalIdc - 1, &m_ChromaQpAdjIdcSCModel.get( 0, 0, 0 ), 0, tableSize - 1 );
  }
}

Void TEncSbac::codeQtCbf( TComTU &rTu, const ComponentID compID, const Bool lowestLevel )
{
  TComDataCU* pcCU = rTu.getCU();

  const UInt absPartIdx   = rTu.GetAbsPartIdxTU(compID);
  const UInt TUDepth      = rTu.GetTransformDepthRel();
        UInt uiCtx        = pcCU->getCtxQtCbf( rTu, toChannelType(compID) );
  const UInt contextSet   = toChannelType(compID);

  const UInt width        = rTu.getRect(compID).width;
  const UInt height       = rTu.getRect(compID).height;
  const Bool canQuadSplit = (width >= (MIN_TU_SIZE * 2)) && (height >= (MIN_TU_SIZE * 2));

  //             Since the CBF for chroma is coded at the highest level possible, if sub-TUs are
  //             to be coded for a 4x8 chroma TU, their CBFs must be coded at the highest 4x8 level
  //             (i.e. where luma TUs are 8x8 rather than 4x4)
  //    ___ ___
  //   |   |   | <- 4 x (8x8 luma + 4x8 4:2:2 chroma)
  //   |___|___|    each quadrant has its own chroma CBF
  //   |   |   | _ _ _ _
  //   |___|___|        |
  //   <--16--->        V
  //                   _ _
  //                  |_|_| <- 4 x 4x4 luma + 1 x 4x8 4:2:2 chroma
  //                  |_|_|    no chroma CBF is coded - instead the parent CBF is inherited
  //                  <-8->    if sub-TUs are present, their CBFs had to be coded at the parent level

  const UInt lowestTUDepth = TUDepth + ((!lowestLevel && !canQuadSplit) ? 1 : 0); //unsplittable TUs inherit their parent's CBF

  if ((width != height) && (lowestLevel || !canQuadSplit)) //if sub-TUs are present
  {
    const UInt subTUDepth        = lowestTUDepth + 1;                      //if this is the lowest level of the TU-tree, the sub-TUs are directly below. Otherwise, this must be the level above the lowest level (as specified above)
    const UInt partIdxesPerSubTU = rTu.GetAbsPartIdxNumParts(compID) >> 1;

    for (UInt subTU = 0; subTU < 2; subTU++)
    {
      const UInt subTUAbsPartIdx = absPartIdx + (subTU * partIdxesPerSubTU);
      const UInt uiCbf           = pcCU->getCbf(subTUAbsPartIdx, compID, subTUDepth);

      m_pcBinIf->encodeBin(uiCbf, m_cCUQtCbfSCModel.get(0, contextSet, uiCtx));

      DTRACE_CABAC_VL( g_nSymbolCounter++ )
      DTRACE_CABAC_T( "\tparseQtCbf()" )
      DTRACE_CABAC_T( "\tsub-TU=" )
      DTRACE_CABAC_V( subTU )
      DTRACE_CABAC_T( "\tsymbol=" )
      DTRACE_CABAC_V( uiCbf )
      DTRACE_CABAC_T( "\tctx=" )
      DTRACE_CABAC_V( uiCtx )
      DTRACE_CABAC_T( "\tetype=" )
      DTRACE_CABAC_V( compID )
      DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
      DTRACE_CABAC_V( subTUAbsPartIdx )
      DTRACE_CABAC_T( "\n" )
    }
  }
  else
  {
    const UInt uiCbf = pcCU->getCbf( absPartIdx, compID, lowestTUDepth );
    m_pcBinIf->encodeBin( uiCbf , m_cCUQtCbfSCModel.get( 0, contextSet, uiCtx ) );


    DTRACE_CABAC_VL( g_nSymbolCounter++ )
    DTRACE_CABAC_T( "\tparseQtCbf()" )
    DTRACE_CABAC_T( "\tsymbol=" )
    DTRACE_CABAC_V( uiCbf )
    DTRACE_CABAC_T( "\tctx=" )
    DTRACE_CABAC_V( uiCtx )
    DTRACE_CABAC_T( "\tetype=" )
    DTRACE_CABAC_V( compID )
    DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
    DTRACE_CABAC_V( rTu.GetAbsPartIdxTU(compID) )
    DTRACE_CABAC_T( "\n" )
  }
}


Void TEncSbac::codeTransformSkipFlags (TComTU &rTu, ComponentID component )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU();

  if (pcCU->getCUTransquantBypass(uiAbsPartIdx))
  {
    return;
  }

  if (!TUCompRectHasAssociatedTransformSkipFlag(rTu.getRect(component), pcCU->getSlice()->getPPS()->getTransformSkipLog2MaxSize()))
  {
    return;
  }

  UInt useTransformSkip = pcCU->getTransformSkip( uiAbsPartIdx,component);
  m_pcBinIf->encodeBin( useTransformSkip, m_cTransformSkipSCModel.get( 0, toChannelType(component), 0 ) );

  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T("\tparseTransformSkip()");
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( useTransformSkip )
  DTRACE_CABAC_T( "\tAddr=" )
  DTRACE_CABAC_V( pcCU->getCtuRsAddr() )
  DTRACE_CABAC_T( "\tetype=" )
  DTRACE_CABAC_V( component )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( rTu.GetAbsPartIdxTU() )
  DTRACE_CABAC_T( "\n" )
}


/** Code I_PCM information.
 * \param pcCU pointer to CU
 * \param uiAbsPartIdx CU index
 * \returns Void
 */
Void TEncSbac::codeIPCMInfo( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIPCM = (pcCU->getIPCMFlag(uiAbsPartIdx) == true)? 1 : 0;

  Bool writePCMSampleFlag = pcCU->getIPCMFlag(uiAbsPartIdx);

  m_pcBinIf->encodeBinTrm (uiIPCM);

  if (writePCMSampleFlag)
  {
    m_pcBinIf->encodePCMAlignBits();

    const UInt minCoeffSizeY = pcCU->getPic()->getMinCUWidth() * pcCU->getPic()->getMinCUHeight();
    const UInt offsetY       = minCoeffSizeY * uiAbsPartIdx;
    for (UInt ch=0; ch < pcCU->getPic()->getNumberValidComponents(); ch++)
    {
      const ComponentID compID = ComponentID(ch);
      const UInt offset = offsetY >> (pcCU->getPic()->getComponentScaleX(compID) + pcCU->getPic()->getComponentScaleY(compID));
      Pel * pPCMSample  = pcCU->getPCMSample(compID) + offset;
      const UInt width  = pcCU->getWidth (uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleX(compID);
      const UInt height = pcCU->getHeight(uiAbsPartIdx) >> pcCU->getPic()->getComponentScaleY(compID);
      const UInt sampleBits = pcCU->getSlice()->getSPS()->getPCMBitDepth(toChannelType(compID));
      for (UInt y=0; y<height; y++)
      {
        for (UInt x=0; x<width; x++)
        {
          UInt sample = pPCMSample[x];
          m_pcBinIf->xWritePCMCode(sample, sampleBits);
        }
        pPCMSample += width;
      }
    }

    m_pcBinIf->resetBac();
  }
}

Void TEncSbac::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCbf = pcCU->getQtRootCbf( uiAbsPartIdx );
  UInt uiCtx = 0;
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseQtRootCbf()" )
  DTRACE_CABAC_T( "\tsymbol=" )
  DTRACE_CABAC_V( uiCbf )
  DTRACE_CABAC_T( "\tctx=" )
  DTRACE_CABAC_V( uiCtx )
  DTRACE_CABAC_T( "\tuiAbsPartIdx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\n" )
}

Void TEncSbac::codeQtCbfZero( TComTU & rTu, const ChannelType chType )
{
  // this function is only used to estimate the bits when cbf is 0
  // and will never be called when writing the bistream. do not need to write log
  UInt uiCbf = 0;
  UInt uiCtx = rTu.getCU()->getCtxQtCbf( rTu, chType );

  m_pcBinIf->encodeBin( uiCbf , m_cCUQtCbfSCModel.get( 0, chType, uiCtx ) );
}

Void TEncSbac::codeColourTransformFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Bool uiFlag = pcCU->getColourTransform(uiAbsPartIdx)== true? 1: 0;
  UInt uiCtx = 0;
  if(pcCU->getSlice()->getPPS()->getUseColourTrans())
  {
    m_pcBinIf->encodeBin( uiFlag , m_cCUColourTransformFlagSCModel.get( 0, 0, uiCtx ) );
  }
}

Void TEncSbac::codeQtRootCbfZero( TComDataCU* pcCU )
{
  // this function is only used to estimate the bits when cbf is 0
  // and will never be called when writing the bistream. do not need to write log
  UInt uiCbf = 0;
  UInt uiCtx = 0;
  m_pcBinIf->encodeBin( uiCbf , m_cCUQtRootCbfSCModel.get( 0, 0, uiCtx ) );
}

/** Encode (X,Y) position of the last significant coefficient
 * \param uiPosX     X component of last coefficient
 * \param uiPosY     Y component of last coefficient
 * \param width      Block width
 * \param height     Block height
 * \param component  chroma component ID
 * \param uiScanIdx  scan type (zig-zag, hor, ver)
 * This method encodes the X and Y component within a block of the last significant coefficient.
 */
Void TEncSbac::codeLastSignificantXY( UInt uiPosX, UInt uiPosY, Int width, Int height, ComponentID component, UInt uiScanIdx )
{
  // swap
  if( uiScanIdx == SCAN_VER )
  {
    swap( uiPosX, uiPosY );
    swap( width,  height );
  }

  UInt uiCtxLast;
  UInt uiGroupIdxX    = g_uiGroupIdx[ uiPosX ];
  UInt uiGroupIdxY    = g_uiGroupIdx[ uiPosY ];

  ContextModel *pCtxX = m_cCuCtxLastX.get( 0, toChannelType(component) );
  ContextModel *pCtxY = m_cCuCtxLastY.get( 0, toChannelType(component) );

  Int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
  getLastSignificantContextParameters(component, width, height, blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY);

  //------------------

  // posX

  for( uiCtxLast = 0; uiCtxLast < uiGroupIdxX; uiCtxLast++ )
  {
    m_pcBinIf->encodeBin( 1, *( pCtxX + blkSizeOffsetX + (uiCtxLast >>shiftX) ) );
  }
  if( uiGroupIdxX < g_uiGroupIdx[ width - 1 ])
  {
    m_pcBinIf->encodeBin( 0, *( pCtxX + blkSizeOffsetX + (uiCtxLast >>shiftX) ) );
  }

  // posY

  for( uiCtxLast = 0; uiCtxLast < uiGroupIdxY; uiCtxLast++ )
  {
    m_pcBinIf->encodeBin( 1, *( pCtxY + blkSizeOffsetY + (uiCtxLast >>shiftY) ) );
  }
  if( uiGroupIdxY < g_uiGroupIdx[ height - 1 ])
  {
    m_pcBinIf->encodeBin( 0, *( pCtxY + blkSizeOffsetY + (uiCtxLast >>shiftY) ) );
  }

  // EP-coded part

  if ( uiGroupIdxX > 3 )
  {
    UInt uiCount = ( uiGroupIdxX - 2 ) >> 1;
    uiPosX       = uiPosX - g_uiMinInGroup[ uiGroupIdxX ];
    for (Int i = uiCount - 1 ; i >= 0; i-- )
    {
      m_pcBinIf->encodeBinEP( ( uiPosX >> i ) & 1 );
    }
  }
  if ( uiGroupIdxY > 3 )
  {
    UInt uiCount = ( uiGroupIdxY - 2 ) >> 1;
    uiPosY       = uiPosY - g_uiMinInGroup[ uiGroupIdxY ];
    for ( Int i = uiCount - 1 ; i >= 0; i-- )
    {
      m_pcBinIf->encodeBinEP( ( uiPosY >> i ) & 1 );
    }
  }
}
#if PGR_ENABLE
Void TEncSbac::codePaletteIndex(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth)
{
    UInt uiNumValidComponents = pcCU->getPic()->getNumberValidComponents();
    TCoeff buffer[MAX_BUFFER] ;
    UInt pos;
    UInt   uiSymbol;
	for (UInt ch = 0; ch < uiNumValidComponents; ch++)
    {
        ComponentID comp = ComponentID( ch ) ;
        Int  iNum =  pcCU->getPaletteIndexNum()[comp];
        Int  *piIndex = pcCU->getPaletteIndex(comp);
        pos = 0;
        ::ExpGolombEncode(iNum,buffer,pos);
        for(Int i=0 ; i< iNum; i++)
        {
            ::ExpGolombEncode(piIndex[i],buffer,pos);
        }
		UInt n = 0;
        while ( n< (pos) )
        {
            uiSymbol =ReadBit(buffer,n);
            m_pcBinIf->encodeBinEP(uiSymbol);               
        }
    }
}
Void TEncSbac::codePosition(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{

    UInt   uiSymbol;
    UInt   uiBitsLength;
    
    UInt  uiCUAddr = pcCU->getCtuRsAddr() ;
    UInt  uiFrameWidthInCU =  pcCU->getPic()->getFrameWidthInCtus() ;
    UInt  uiCUAboveAddr = uiCUAddr - uiFrameWidthInCU;
    UInt  uiCULeftAddr = uiCUAddr - 1 ;
    UInt  uiCUAboveLeftAddr   = uiCUAddr -1 - uiFrameWidthInCU;
    UInt  uiCUAboveRightAddr = uiCUAddr +1 - uiFrameWidthInCU;
    UInt  uiCUArray[4] = {uiCULeftAddr,uiCUAboveAddr,uiCUAboveLeftAddr,uiCUAboveRightAddr};
    UInt  uiCUHandleAddr ; 
    const ChromaFormat chFmt = pcCU->getPic()->getChromaFormat();   

     UInt n; 
     UInt start;
     int GroupID,index;
     int  iBuffer[MAX_BUFFER*2];
     bool  bEnd ;
     int Base , Index ;   
     UInt uiNumValidComponents = pcCU->getPic()->getNumberValidComponents();
     const UInt  uiZOrder      = pcCU->getZorderIdxInCtu() +uiAbsPartIdx;
     TComPicYuv* pcPicYuv  = pcCU->getPic()->getPicYuvResi();
     UInt  CUPelX,CUPelY;
     CUPelX= pcCU->getCUPelX() +   g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
     CUPelY= pcCU->getCUPelY() +   g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ]; 
     UInt uiHeight = pcCU->getHeight(uiAbsPartIdx);
     UInt uiWidth  = pcCU->getWidth  (uiAbsPartIdx);
	for (UInt ch = 0; ch < uiNumValidComponents; ch++)
     {
          ComponentID compID = ComponentID(ch);          
          TCoeff* pcCoef = pcCU->getPRBits(compID);
           const UInt  uiResiStride   = pcPicYuv->getStride( compID); 


            //cout<<compID<<" "<<CUPelX<<" "<<CUPelY<<" "<<uiAbsPartIdx<<" "<<uiWidth<<" "<<uiHeight<<endl;
            //Pel*  piDes         = pcPicYuv->getAddr(compID) +CUPelY * uiResiStride + CUPelX;
            Pel* piDes        = pcPicYuv->getAddr(compID,uiCUAddr, pcCU->getZorderIdxInCtu() +uiAbsPartIdx);
            Pel*  pDes = piDes ;            
            UInt  uiPRMode = 2;//pcCU->getPRMode(uiAbsPartIdx,compID);
            for( UInt uiY = 0; uiY < uiHeight; uiY++  )
            {
              memset(pDes,0,sizeof(Pel)*uiWidth);
              pDes += uiResiStride;
            }
            UInt dstindex;int num;
            bool  vlcf[3] = {false,false,false};// dx & dy   residual  srcindex
            //bool  bMostValue = pcCU->getPaletteFlag(uiAbsPartIdx,compID);//false;
            start = 0 ;
            //if(bMostValue )  prnumposresi[compID][4]++;
            
#if     INTRA_PALETTE_FLAG
            m_pcBinIf->encodeBinEP(bMostValue);
#endif
            switch( uiPRMode )
            {
            case 1:
                m_pcBinIf->encodeBinEP(0);
                m_pcBinIf->encodeBinEP(0);
                break;
            case 2:
                m_pcBinIf->encodeBinEP(0);
                m_pcBinIf->encodeBinEP(1);
                break;
            case 3:
                m_pcBinIf->encodeBinEP(1);
                m_pcBinIf->encodeBinEP(0);
                break;
            case 4:                
                m_pcBinIf->encodeBinEP(1);
                m_pcBinIf->encodeBinEP(1);
                break;
            }
            switch( uiPRMode )
             {
                
                 case 4:
                    TComYuv  *pcTempA;
                    pcTempA= new TComYuv;
                    pcTempA->create( uiWidth , uiHeight, chFmt) ;
                    UInt    uiAStride;
                    uiAStride = pcTempA->getStride (compID);
                    {     
                            for(UInt  index =0 ; index< 4; index++)
                            {
                                 if( (int) uiCUArray[index]<0) continue; 
                                 {
                                     uiCUHandleAddr = uiCUArray[index] ;
                                     //UInt uiCUPelX,uiCUPelY;
                                     //uiCUPelX           = ( uiCUHandleAddr % pcCU->getPic()->getFrameWidthInCU() ) * g_uiMaxCUWidth;
                                     //uiCUPelY           = ( uiCUHandleAddr /   pcCU->getPic()->getFrameWidthInCU() ) * g_uiMaxCUHeight;
                                     //uiCUPelX= uiCUPelX +  g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
                                     //uiCUPelY= uiCUPelY +  g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];             
             
                                     //Pel *pHandleResi = pcPicYuv->getAddr(compID) + uiCUPelY * uiResiStride + uiCUPelX;
                                     Pel* pHandleResi = pcPicYuv->getAddr(compID,uiCUHandleAddr,uiAbsPartIdx + pcCU->getZorderIdxInCtu());
                                     Pel  *pADest =  pcTempA->getAddr(compID);
                                    for (Int y =  0; y < uiHeight; y++ )
                                    {
                                        for(Int x=0;x < uiWidth ; x++)
                                        {
                                            pADest[x] = pHandleResi[x] ;                    
                                        }
                                        pADest += uiAStride;
                                        pHandleResi   += uiResiStride;                
                                   }
                                 }
                            }
                    } 
                    
                    
                    num = 0;
                    for (UInt y=0; y<uiHeight; y++) 
                    {
                            for (UInt x=0; x<uiWidth; x++) 
                            {
                                if( *(pcTempA->getAddr(compID)+y*uiAStride+x) )
                                {                              
                                       
                                        Index = ExpGolombDecode( pcCoef, start) ;
                                        assert(Index>=-1 && Index < INTRA_PR_PALETTE_NUM);
                                       
                                       *( piDes +(y) * uiResiStride+(x) ) =  1;
                                   
                                    //cout<<x << " "<<y<<" "<<*(pcTempA->getAddr(compID)+y*uiAStride+x) <<" "<<*( piDes +(y) * uiResiStride+(x)  )<<endl;
                                    num++;
                                }                            
		                    } 
                     
	                }

                    pcTempA->destroy();
                    delete pcTempA;
                    pcTempA = NULL ;

                 case 2:

                    memset( iBuffer, -1, MAX_BUFFER*2*sizeof(int) );
                    
                    index = 0 ;   bEnd = false ;
                    {
						Index =  ExpGolombDecode(pcCoef, start);
						//assert(Index == (pcCU->getPaletteIndexNum(compID))[compID]);
                        for(int i=0;i<Index;i++) {
                            uiBitsLength = start;
                            iBuffer[i*2    ] =  ExpGolombDecode( pcCoef, start); 
                            iBuffer[i*2+1] =  ExpGolombDecode( pcCoef, start); 
                            index += 2;
                        }

                    }
                    uiBitsLength = start ;
                    n = 0;
                    while ( n< (uiBitsLength)  )
                    {
                        uiSymbol =ReadBit(pcCoef,n);
                        m_pcBinIf->encodeBinEP(uiSymbol);               
                    }

                    {
                        assert( (index % 2)==0 );
            
                        for(n = 2; n< index; n = n +2)
                        {
                            iBuffer[n] += iBuffer[n-2];
                            iBuffer[n+1] += iBuffer[n-1];
                        }


                       for( n=0; n<index; n= n+4) 
                        {                    
                            int dx   = iBuffer[n] ;
                            int dy   = iBuffer[n+1] ;

                            assert( dx< uiWidth );
                            assert( dy< uiHeight ) ;

                             * ( piDes + dy * uiResiStride + dx ) = 1;
                         }
                    } 

                    break;
               case 3:                   
                   
                   Int  iValue, iLength, iValue1;
                   int  indexlist[64*64][4],indexlist2[64*64];
                   memset(indexlist  ,0,4*64*64*sizeof(int));
                   memset(indexlist2,0,   64*64*sizeof(int));

                   Index =  ExpGolombDecode( pcCoef, start) ;
                   assert(Index>=0&& Index <4);
                   uiCUHandleAddr = uiCUArray[ Index] ;
                   assert( (int) uiCUHandleAddr>=0) ;

                   vlcf[2] = ExpGolombDecode( pcCoef, start) ;
                   uiBitsLength = start;
                   dstindex=0;
                   do{
                           indexlist2[dstindex] = ExpGolombDecode( pcCoef, start) ;
                           dstindex++;
                    }while(indexlist2[dstindex-1]!= -1);
                   dstindex--;
                    
                   assert(dstindex);
                    if( !vlcf[2]){ 
                       for(UInt x=0;x<dstindex;x++)
                       {
                           indexlist[x][3] = indexlist2[x];
                       }                   
                     } else {
                       n = 0 ;
                       for(UInt x=0;x<dstindex;x+=2)
                       {
                           iLength = indexlist2[x];
                           assert( iLength>=0);
                           iValue  = indexlist2[x+1] ;
                           for(UInt y=0; y< iLength +1; y++)
                           {
                               indexlist[n][3] =  iValue ;
                               n++;
                           }
                       }
                       dstindex = n;
                     }
                   vlcf[1] =  ExpGolombDecode( pcCoef, start) ;
                   uiBitsLength = start;
                   if( !vlcf[1])
                   { 
                       for(UInt x=0;x<dstindex;x++)
                       {
                           uiBitsLength = start ;
                           indexlist[x][2] = ExpGolombDecode( pcCoef, start) ;
                            {
   
#if LOSSY
                               Base = DQLossyValue(Base);
#endif
                               indexlist[x][2] = -1;
                              //just for position informatin reuse.
                               if( indexlist[x][2]==0 )indexlist[x][2] = -1;
                               

                           } 
                       }                   
                   }
                   else
                   {
                       n=0;
                       do
                       {
                           iLength = ExpGolombDecode( pcCoef, start);
                           assert( (int) iLength >=0);
                           
                           iValue  =  ExpGolombDecode( pcCoef, start);
                            {
                               
                                   iValue = -1;
                           }                       

                           for(UInt y=0; y< iLength+1; y++)
                           {
                               indexlist[n][2] = iValue ;
                               n++;
                           }
                       } while(n < dstindex );
                   }
                    
                   vlcf[0] =  ExpGolombDecode( pcCoef, start) ;
                    uiBitsLength = start;
                   if( !vlcf[0])
                   { 
                       for(UInt x=0;x<dstindex;x++)
                       {
                           indexlist[x][0] = ExpGolombDecode( pcCoef, start) ;
                           indexlist[x][1] = ExpGolombDecode( pcCoef, start) ;
                       }                   
                   }
                   else
                    {
                       n=0;
                       do
                       {
                           iLength = ExpGolombDecode( pcCoef, start);
                           assert( (int) iLength >=0);
                           iValue  =  ExpGolombDecode( pcCoef, start);
                           iValue1  =  ExpGolombDecode( pcCoef, start);
                           for(UInt y=0; y< iLength+1; y++)
                           {
                               indexlist[n][0] = iValue ;
                               indexlist[n][1] = iValue1 ;
                               n++;
                           }
                       } while(n < dstindex );
                     }
                  
                   for(UInt x=1;x<dstindex;x++) {
              //        indexlist[x][2] += indexlist[x-1][2] ;
                        indexlist[x][3] += indexlist[x-1][3] ;
                    }
                   
                   //reconstrction residual data
                   {
                        TComYuv  *pcTemp;
                        pcTemp =  new TComYuv;
                        //UInt  uiSrc1Stride = g_pcPicYuvResi->getStride(compID);
                        //CUPelX           = ( uiCUHandleAddr % pcCU->getPic()->getFrameWidthInCU() ) * g_uiMaxCUWidth;
                        //CUPelY           = ( uiCUHandleAddr /   pcCU->getPic()->getFrameWidthInCU() ) * g_uiMaxCUHeight;
                        //CUPelX= CUPelX +  g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ];
                        //CUPelY= CUPelY +  g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ];
                        //
                        // 
                        // Pel *pSrc1 = pcPicYuv->getAddr(compID) +CUPelY * uiSrc1Stride + CUPelX;
                         Pel *pHandleCUSrc = pcPicYuv->getAddr(compID,uiCUHandleAddr, uiAbsPartIdx + pcCU->getZorderIdxInCtu());
                         //if(compID != COMPONENT_Y) pHandleCUSrc = pcPicYuv->getAddr(COMPONENT_Y,uiCUHandleAddr, uiAbsPartIdx + pcCU->getZorderIdxInCU());
                         pcTemp->create(uiWidth,uiHeight,chFmt);
                         UInt  uiTempStride = pcTemp->getStride(compID);
                         
                         Pel  *pTemp =  pcTemp->getAddr(compID);
                        for (Int y =  0; y < uiHeight; y++ )
                        {
                            for(Int x=0;x < uiWidth ; x++)
                            {
                                pTemp[x] = pHandleCUSrc[x] ;                    
                            }
                            pTemp += uiTempStride;
                            pHandleCUSrc += uiResiStride;                
                        }
		                int srclx = 0; int srcly = 0; int srclv = 0;
		                int srchasleft = 1;
                        Pel  srcpel;
                        int  srclist[3][64*64];
                        int  srcindex = 0 ;
                        memset(srclist,-1,3*64*64*sizeof(int));
                        int cursrclistindex = 0;
                        const UInt    uiSrcStride = pcTemp->getStride (compID);
                        Pel*  piSrc     = pcTemp->getAddr(compID);
                        //Pel*  piSrc     = pcTemp->getAddr(compID, uiAbsPartIdx);
                        Pel*  pSrc      = piSrc ;
                        //found the source list
                        while (srchasleft) {
			                int ndis = 1000;
			                int nx = -1; int ny = -1;
                            pSrc = piSrc ;
			                for (UInt y=0; y<uiHeight; y++) {
				                for (UInt x=0; x<uiWidth; x++) {
                                    assert( pSrc[x] >-256 && pSrc[x]<256);
					                if (pSrc[x]!=0) {
						                int dis = 0;
						                dis+=getG0Bits( (x-srclx));
						                dis+=getG0Bits( (y-srcly));
						                if (dis < ndis) {
							                nx = x;
							                ny = y;
                                            ndis = dis ;
						                }
					                }
				                }
                                pSrc += uiSrcStride;
			                }
                           if(nx!=-1 && ny!=-1) {
                                srcpel =  *(piSrc +ny*uiSrcStride+nx) ;	 
				                srclx = nx; srcly = ny; srclv = srcpel;
                                srclist[0][srcindex] = srclx ;
                                srclist[1][srcindex] = srcly ;
                                srclist[2][srcindex] = srcpel;
                                srcindex++;
			                    *(piSrc +ny*uiSrcStride+nx) =0;
                        }else {
                                srchasleft = 0;
                            }
                    }
                        pcTemp->destroy();
                        delete pcTemp;
                        pcTemp = NULL;
                    
                        for(UInt x=0;x<dstindex;x++)
                        {
                            Int  srclistindex = indexlist[x][3];
                            assert(srclistindex< srcindex);
                            Int  dy = indexlist[x][1]+srclist[1][srclistindex] ;
                            Int  dx = indexlist[x][0]+srclist[0][srclistindex] ;
                            assert(dy >=0 && dx >=0);
                            assert(indexlist[x][2] >-256 && indexlist[x][2] < 256);
                            
                            {
                                *( piDes +(dy)*uiResiStride+(dx) ) = indexlist[x][2] ;
                            }
                            
                        }
                   }
                   //write to bitstream
                   uiBitsLength = start ;
                   n = 0;
                   while ( n< (uiBitsLength)  )
                   {
                        uiSymbol =ReadBit(pcCoef,n);
                        m_pcBinIf->encodeBinEP(uiSymbol);               
                   }
                   break;

               default:
                   assert(false);
                   break;
               }
          }
    
    }

Void TEncSbac::codeRevision(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth)
{
	UInt uiNumValidComponents = pcCU->getPic()->getNumberValidComponents();
	// shortest path
	UInt uiWidth = pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth;
	UInt uiHeight = pcCU->getSlice()->getSPS()->getMaxCUWidth() >> uiDepth;

	TComYuv* pAbnormalResi = new TComYuv;
	pAbnormalResi->create(uiWidth, uiHeight, pcCU->getPic()->getChromaFormat());
	pAbnormalResi->copyFromPicYuv(g_pcYuvAbnormalResi, pcCU->getCtuRsAddr(), pcCU->getZorderIdxInCtu() + uiAbsPartIdx);

	// buffers
	TCoeff pcPosBuffer[4096 * 2];
	UInt puiIdxBuffer[4096];
	UInt uiNumIdx;
	TCoeff pcPosBitsBuf[4096 * 2];
	TCoeff pcIdxBitsBuf [4096];

	//fstream fpos, fidx;

	for (UInt ch = 0; ch < uiNumValidComponents; ch++)
	{
		//char tmp[64];
		//sprintf(tmp, "pos_%d.txt", ch);
		//fpos.open(tmp, ios::app);
		//sprintf(tmp, "idx_%d.txt", ch);
		//fidx.open(tmp, ios::app);

		ComponentID cId = ComponentID(ch);
		// shortest path
		PositionCode_PathLeast(pAbnormalResi, cId, pcPosBuffer,puiIdxBuffer,uiNumIdx);
		// code
		// expgolomb
		UInt uiPosCur = 0;
		UInt uiIdxCur = 0;
		UInt uiRunLength = 1;
		UInt uiIdx = puiIdxBuffer[0];
		ExpGolombEncode(uiNumIdx, pcPosBitsBuf, uiPosCur);
		for (UInt i = 0; i < uiNumIdx; i++)
		{
			// position
			int x = pcPosBuffer[2 * i];
			int y = pcPosBuffer[2 * i + 1];

			//fpos << x << "\t" << y << endl;

			ExpGolombEncode(x, pcPosBitsBuf, uiPosCur);
			ExpGolombEncode(y, pcPosBitsBuf, uiPosCur);
			// index runlength
			if (i + 1 < uiNumIdx && puiIdxBuffer[i] == puiIdxBuffer[i + 1])
			{
				uiRunLength++;
			}
			else
			{
				ExpGolombEncode(uiRunLength, pcIdxBitsBuf, uiIdxCur);
				ExpGolombEncode(uiIdx, pcIdxBitsBuf, uiIdxCur);
				//fidx << uiRunLength << "\t" << uiIdx << endl;
				uiIdx = puiIdxBuffer[i + 1];
				uiRunLength = 1;
			}
		}
		
		// encode position
		Int uiSymbol;
		UInt uiBitsLength = uiPosCur;
		uiPosCur = 0;
		while (uiPosCur < uiBitsLength)
		{
			uiSymbol = ReadBit(pcPosBitsBuf, uiPosCur);
			m_pcBinIf->encodeBinEP(uiSymbol);
		}

		// encode index
		uiBitsLength = uiIdxCur;
		uiIdxCur = 0;
		while (uiIdxCur < uiBitsLength)
		{
			uiSymbol = ReadBit(pcIdxBitsBuf, uiIdxCur);
			m_pcBinIf->encodeBinEP(uiSymbol);
		}
		//fpos.close();
		//fidx.close();
	}

	if (pAbnormalResi)
	{
		delete pAbnormalResi;
		pAbnormalResi = NULL;
	}

}

Void TEncSbac::codePalette(Palette ppPalette)
{
	TCoeff buffer[1024];
	UInt uiCur = 0;
	ExpGolombEncode(ppPalette.m_uiSize, buffer, uiCur);
	for (UInt i = 0; i < ppPalette.m_uiSize; i++)
		ExpGolombEncode(ppPalette.m_pEntry[i], buffer, uiCur);
	UInt uiBitsLength = uiCur;
	uiCur = 0;
	UInt uiSymbol;
	while (uiCur < uiBitsLength)
	{
		uiSymbol = ReadBit(buffer, uiCur);
		m_pcBinIf->encodeBinEP(uiSymbol);
	}
}
#endif

Void TEncSbac::codeCoeffNxN( TComTU &rTu, TCoeff* pcCoef, const ComponentID compID )
{
  TComDataCU* pcCU=rTu.getCU();
  const UInt uiAbsPartIdx=rTu.GetAbsPartIdxTU(compID);
  const TComRectangle &tuRect=rTu.getRect(compID);
  const UInt uiWidth=tuRect.width;
  const UInt uiHeight=tuRect.height;
  const TComSPS &sps=*(pcCU->getSlice()->getSPS());

  DTRACE_CABAC_VL( g_nSymbolCounter++ )
  DTRACE_CABAC_T( "\tparseCoeffNxN()\teType=" )
  DTRACE_CABAC_V( compID )
  DTRACE_CABAC_T( "\twidth=" )
  DTRACE_CABAC_V( uiWidth )
  DTRACE_CABAC_T( "\theight=" )
  DTRACE_CABAC_V( uiHeight )
  DTRACE_CABAC_T( "\tdepth=" )
//  DTRACE_CABAC_V( rTu.GetTransformDepthTotalAdj(compID) )
  DTRACE_CABAC_V( rTu.GetTransformDepthTotal() )
  DTRACE_CABAC_T( "\tabspartidx=" )
  DTRACE_CABAC_V( uiAbsPartIdx )
  DTRACE_CABAC_T( "\ttoCU-X=" )
  DTRACE_CABAC_V( pcCU->getCUPelX() )
  DTRACE_CABAC_T( "\ttoCU-Y=" )
  DTRACE_CABAC_V( pcCU->getCUPelY() )
  DTRACE_CABAC_T( "\tCU-addr=" )
  DTRACE_CABAC_V(  pcCU->getCtuRsAddr() )
  DTRACE_CABAC_T( "\tinCU-X=" )
//  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_V( g_auiRasterToPelX[ g_auiZscanToRaster[rTu.GetAbsPartIdxTU(compID)] ] )
  DTRACE_CABAC_T( "\tinCU-Y=" )
// DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[uiAbsPartIdx] ] )
  DTRACE_CABAC_V( g_auiRasterToPelY[ g_auiZscanToRaster[rTu.GetAbsPartIdxTU(compID)] ] )
  DTRACE_CABAC_T( "\tpredmode=" )
  DTRACE_CABAC_V(  pcCU->getPredictionMode( uiAbsPartIdx ) )
  DTRACE_CABAC_T( "\n" )

  //--------------------------------------------------------------------------------------------------

  if( uiWidth > sps.getMaxTrSize() )
  {
    std::cerr << "ERROR: codeCoeffNxN was passed a TU with dimensions larger than the maximum allowed size" << std::endl;
    assert(false);
    exit(1);
  }

  // compute number of significant coefficients
  UInt uiNumSig = TEncEntropy::countNonZeroCoeffs(pcCoef, uiWidth * uiHeight);

  if ( uiNumSig == 0 )
  {
    std::cerr << "ERROR: codeCoeffNxN called for empty TU!" << std::endl;
    assert(false);
    exit(1);
  }

  //--------------------------------------------------------------------------------------------------

  //set parameters

  const ChannelType  chType            = toChannelType(compID);
  const UInt         uiLog2BlockWidth  = g_aucConvertToBit[ uiWidth  ] + 2;
  const UInt         uiLog2BlockHeight = g_aucConvertToBit[ uiHeight ] + 2;

  const ChannelType  channelType       = toChannelType(compID);
  const Bool         extendedPrecision = sps.getUseExtendedPrecision();

  const Bool         alignCABACBeforeBypass = sps.getAlignCABACBeforeBypass();
  const Int          maxLog2TrDynamicRange  = sps.getMaxLog2TrDynamicRange(channelType);

  Bool beValid;

  {
    Int uiIntraMode = -1;
    const Bool       bIsLuma = isLuma(compID);
    Int isIntra = pcCU->isIntra(uiAbsPartIdx) ? 1 : 0;
    if ( isIntra )
    {
      uiIntraMode = pcCU->getIntraDir( toChannelType(compID), uiAbsPartIdx );

      const UInt partsPerMinCU = 1<<(2*(sps.getMaxTotalCUDepth() - sps.getLog2DiffMaxMinCodingBlockSize()));
      uiIntraMode = (uiIntraMode==DM_CHROMA_IDX && !bIsLuma) ? pcCU->getIntraDir(CHANNEL_TYPE_LUMA, getChromasCorrespondingPULumaIdx(uiAbsPartIdx, rTu.GetChromaFormat(), partsPerMinCU)) : uiIntraMode;
      uiIntraMode = ((rTu.GetChromaFormat() == CHROMA_422) && !bIsLuma) ? g_chroma422IntraAngleMappingTable[uiIntraMode] : uiIntraMode;
    }

    Int transformSkip = pcCU->getTransformSkip( uiAbsPartIdx,compID) ? 1 : 0;
    Bool rdpcm_lossy = ( transformSkip && isIntra && ( (uiIntraMode == HOR_IDX) || (uiIntraMode == VER_IDX) ) ) && pcCU->isRDPCMEnabled(uiAbsPartIdx);

    if ( (pcCU->getCUTransquantBypass(uiAbsPartIdx)) || rdpcm_lossy )
    {
      beValid = false;
      if ( (!pcCU->isIntra(uiAbsPartIdx)) && pcCU->isRDPCMEnabled(uiAbsPartIdx))
      {
        codeExplicitRdpcmMode( rTu, compID);
      }
    }
    else
    {
      beValid = pcCU->getSlice()->getPPS()->getSignHideFlag();
    }
  }

  //--------------------------------------------------------------------------------------------------

  if(pcCU->getSlice()->getPPS()->getUseTransformSkip())
  {
    codeTransformSkipFlags(rTu, compID);
    if(pcCU->getTransformSkip(uiAbsPartIdx, compID) && !pcCU->isIntra(uiAbsPartIdx) && pcCU->isRDPCMEnabled(uiAbsPartIdx))
    {
      //  This TU has coefficients and is transform skipped. Check whether is inter coded and if yes encode the explicit RDPCM mode
      codeExplicitRdpcmMode( rTu, compID);

      if(pcCU->getExplicitRdpcmMode(compID, uiAbsPartIdx) != RDPCM_OFF)
      {
        //  Sign data hiding is avoided for horizontal and vertical explicit RDPCM modes
        beValid = false;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  const Bool  bUseGolombRiceParameterAdaptation = sps.getUseGolombRiceParameterAdaptation();
        UInt &currentGolombRiceStatistic        = m_golombRiceAdaptationStatistics[rTu.getGolombRiceStatisticsIndex(compID)];

  //select scans
  TUEntropyCodingParameters codingParameters;
  getTUEntropyCodingParameters(codingParameters, rTu, compID);

  //----- encode significance map -----

  // Find position of last coefficient
  Int scanPosLast = -1;
  Int posLast;


  UInt uiSigCoeffGroupFlag[ MLS_GRP_NUM ];

  memset( uiSigCoeffGroupFlag, 0, sizeof(UInt) * MLS_GRP_NUM );
  do
  {
    posLast = codingParameters.scan[ ++scanPosLast ];

    if( pcCoef[ posLast ] != 0 )
    {
      // get L1 sig map
      UInt uiPosY   = posLast >> uiLog2BlockWidth;
      UInt uiPosX   = posLast - ( uiPosY << uiLog2BlockWidth );

      UInt uiBlkIdx = (codingParameters.widthInGroups * (uiPosY >> MLS_CG_LOG2_HEIGHT)) + (uiPosX >> MLS_CG_LOG2_WIDTH);
      uiSigCoeffGroupFlag[ uiBlkIdx ] = 1;

      uiNumSig--;
    }
  } while ( uiNumSig > 0 );

  // Code position of last coefficient
  Int posLastY = posLast >> uiLog2BlockWidth;
  Int posLastX = posLast - ( posLastY << uiLog2BlockWidth );
  codeLastSignificantXY(posLastX, posLastY, uiWidth, uiHeight, compID, codingParameters.scanType);

  //===== code significance flag =====
  ContextModel * const baseCoeffGroupCtx = m_cCUSigCoeffGroupSCModel.get( 0, chType );
  ContextModel * const baseCtx = m_cCUSigSCModel.get( 0, 0 ) + getSignificanceMapContextOffset(compID);

  const Int  iLastScanSet  = scanPosLast >> MLS_CG_SIZE;

  UInt c1                  = 1;
  UInt uiGoRiceParam       = 0;
  Int  iScanPosSig         = scanPosLast;

  for( Int iSubSet = iLastScanSet; iSubSet >= 0; iSubSet-- )
  {
    Int numNonZero = 0;
    Int  iSubPos   = iSubSet << MLS_CG_SIZE;
    uiGoRiceParam  = currentGolombRiceStatistic / RExt__GOLOMB_RICE_INCREMENT_DIVISOR;
    Bool updateGolombRiceStatistics = bUseGolombRiceParameterAdaptation; //leave the statistics at 0 when not using the adaptation system
    UInt coeffSigns = 0;

    Int absCoeff[1 << MLS_CG_SIZE];

    Int lastNZPosInCG  = -1;
    Int firstNZPosInCG = 1 << MLS_CG_SIZE;

    Bool escapeDataPresentInGroup = false;

    if( iScanPosSig == scanPosLast )
    {
      absCoeff[ 0 ] = Int(abs( pcCoef[ posLast ] ));
      coeffSigns    = ( pcCoef[ posLast ] < 0 );
      numNonZero    = 1;
      lastNZPosInCG  = iScanPosSig;
      firstNZPosInCG = iScanPosSig;
      iScanPosSig--;
    }

    // encode significant_coeffgroup_flag
    Int iCGBlkPos = codingParameters.scanCG[ iSubSet ];
    Int iCGPosY   = iCGBlkPos / codingParameters.widthInGroups;
    Int iCGPosX   = iCGBlkPos - (iCGPosY * codingParameters.widthInGroups);

    if( iSubSet == iLastScanSet || iSubSet == 0)
    {
      uiSigCoeffGroupFlag[ iCGBlkPos ] = 1;
    }
    else
    {
      UInt uiSigCoeffGroup   = (uiSigCoeffGroupFlag[ iCGBlkPos ] != 0);
      UInt uiCtxSig  = TComTrQuant::getSigCoeffGroupCtxInc( uiSigCoeffGroupFlag, iCGPosX, iCGPosY, codingParameters.widthInGroups, codingParameters.heightInGroups );
      m_pcBinIf->encodeBin( uiSigCoeffGroup, baseCoeffGroupCtx[ uiCtxSig ] );
    }

    // encode significant_coeff_flag
    if( uiSigCoeffGroupFlag[ iCGBlkPos ] )
    {
      const Int patternSigCtx = TComTrQuant::calcPatternSigCtx(uiSigCoeffGroupFlag, iCGPosX, iCGPosY, codingParameters.widthInGroups, codingParameters.heightInGroups);

      UInt uiBlkPos, uiSig, uiCtxSig;
      for( ; iScanPosSig >= iSubPos; iScanPosSig-- )
      {
        uiBlkPos  = codingParameters.scan[ iScanPosSig ];
        uiSig     = (pcCoef[ uiBlkPos ] != 0);
        if( iScanPosSig > iSubPos || iSubSet == 0 || numNonZero )
        {
          uiCtxSig  = TComTrQuant::getSigCtxInc( patternSigCtx, codingParameters, iScanPosSig, uiLog2BlockWidth, uiLog2BlockHeight, chType );
          m_pcBinIf->encodeBin( uiSig, baseCtx[ uiCtxSig ] );
        }
        if( uiSig )
        {
          absCoeff[ numNonZero ] = Int(abs( pcCoef[ uiBlkPos ] ));
          coeffSigns = 2 * coeffSigns + ( pcCoef[ uiBlkPos ] < 0 );
          numNonZero++;
          if( lastNZPosInCG == -1 )
          {
            lastNZPosInCG = iScanPosSig;
          }
          firstNZPosInCG = iScanPosSig;
        }
      }
    }
    else
    {
      iScanPosSig = iSubPos - 1;
    }

    if( numNonZero > 0 )
    {
      Bool signHidden = ( lastNZPosInCG - firstNZPosInCG >= SBH_THRESHOLD );

      const UInt uiCtxSet = getContextSetIndex(compID, iSubSet, (c1 == 0));
      c1 = 1;

      ContextModel *baseCtxMod = m_cCUOneSCModel.get( 0, 0 ) + (NUM_ONE_FLAG_CTX_PER_SET * uiCtxSet);

      Int numC1Flag = min(numNonZero, C1FLAG_NUMBER);
      Int firstC2FlagIdx = -1;
      for( Int idx = 0; idx < numC1Flag; idx++ )
      {
        UInt uiSymbol = absCoeff[ idx ] > 1;
        m_pcBinIf->encodeBin( uiSymbol, baseCtxMod[c1] );
        if( uiSymbol )
        {
          c1 = 0;

          if (firstC2FlagIdx == -1)
          {
            firstC2FlagIdx = idx;
          }
          else //if a greater-than-one has been encountered already this group
          {
            escapeDataPresentInGroup = true;
          }
        }
        else if( (c1 < 3) && (c1 > 0) )
        {
          c1++;
        }
      }

      if (c1 == 0)
      {
        baseCtxMod = m_cCUAbsSCModel.get( 0, 0 ) + (NUM_ABS_FLAG_CTX_PER_SET * uiCtxSet);
        if ( firstC2FlagIdx != -1)
        {
          UInt symbol = absCoeff[ firstC2FlagIdx ] > 2;
          m_pcBinIf->encodeBin( symbol, baseCtxMod[0] );
          if (symbol != 0)
          {
            escapeDataPresentInGroup = true;
          }
        }
      }

      escapeDataPresentInGroup = escapeDataPresentInGroup || (numNonZero > C1FLAG_NUMBER);

      if (escapeDataPresentInGroup && alignCABACBeforeBypass)
      {
        m_pcBinIf->align();
      }

      if( beValid && signHidden )
      {
        m_pcBinIf->encodeBinsEP( (coeffSigns >> 1), numNonZero-1 );
      }
      else
      {
        m_pcBinIf->encodeBinsEP( coeffSigns, numNonZero );
      }

      Int iFirstCoeff2 = 1;
      if (escapeDataPresentInGroup)
      {
        for ( Int idx = 0; idx < numNonZero; idx++ )
        {
          UInt baseLevel  = (idx < C1FLAG_NUMBER)? (2 + iFirstCoeff2 ) : 1;

          if( absCoeff[ idx ] >= baseLevel)
          {
            const UInt escapeCodeValue = absCoeff[idx] - baseLevel;

            xWriteCoefRemainExGolomb( escapeCodeValue, uiGoRiceParam, extendedPrecision, maxLog2TrDynamicRange );

            if (absCoeff[idx] > (3 << uiGoRiceParam))
            {
              uiGoRiceParam = bUseGolombRiceParameterAdaptation ? (uiGoRiceParam + 1) : (std::min<UInt>((uiGoRiceParam + 1), 4));
            }

            if (updateGolombRiceStatistics)
            {
              const UInt initialGolombRiceParameter = currentGolombRiceStatistic / RExt__GOLOMB_RICE_INCREMENT_DIVISOR;

              if (escapeCodeValue >= (3 << initialGolombRiceParameter))
              {
                currentGolombRiceStatistic++;
              }
              else if (((escapeCodeValue * 2) < (1 << initialGolombRiceParameter)) && (currentGolombRiceStatistic > 0))
              {
                currentGolombRiceStatistic--;
              }

              updateGolombRiceStatistics = false;
            }
          }

          if(absCoeff[ idx ] >= 2)
          {
            iFirstCoeff2 = 0;
          }
        }
      }
    }
  }
#if ENVIRONMENT_VARIABLE_DEBUG_AND_TEST
  printSBACCoeffData(posLastX, posLastY, uiWidth, uiHeight, compID, uiAbsPartIdx, codingParameters.scanType, pcCoef, pcCU->getSlice()->getFinalized());
#endif

  return;
}

/** code SAO offset sign
 * \param code sign value
 */
Void TEncSbac::codeSAOSign( UInt code )
{
  m_pcBinIf->encodeBinEP( code );
}

Void TEncSbac::codeSaoMaxUvlc    ( UInt code, UInt maxSymbol )
{
  if (maxSymbol == 0)
  {
    return;
  }

  Int i;
  Bool bCodeLast = ( maxSymbol > code );

  if ( code == 0 )
  {
    m_pcBinIf->encodeBinEP( 0 );
  }
  else
  {
    m_pcBinIf->encodeBinEP( 1 );
    for ( i=0; i<code-1; i++ )
    {
      m_pcBinIf->encodeBinEP( 1 );
    }
    if( bCodeLast )
    {
      m_pcBinIf->encodeBinEP( 0 );
    }
  }
}

/** Code SAO EO class or BO band position
 */
Void TEncSbac::codeSaoUflc       ( UInt uiLength, UInt uiCode )
{
  m_pcBinIf->encodeBinsEP ( uiCode, uiLength );
}

/** Code SAO merge flags
 */
Void TEncSbac::codeSaoMerge       ( UInt uiCode )
{
  m_pcBinIf->encodeBin(((uiCode == 0) ? 0 : 1),  m_cSaoMergeSCModel.get( 0, 0, 0 ));
}

/** Code SAO type index
 */
Void TEncSbac::codeSaoTypeIdx       ( UInt uiCode)
{
  if (uiCode == 0)
  {
    m_pcBinIf->encodeBin( 0, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
  }
  else
  {
    m_pcBinIf->encodeBin( 1, m_cSaoTypeIdxSCModel.get( 0, 0, 0 ) );
    m_pcBinIf->encodeBinEP( uiCode == 1 ? 0 : 1 );
  }
}

Void TEncSbac::codeSAOOffsetParam(ComponentID compIdx, SAOOffset& ctbParam, Bool sliceEnabled, const Int channelBitDepth)
{
  UInt uiSymbol;
  if(!sliceEnabled)
  {
    assert(ctbParam.modeIdc == SAO_MODE_OFF);
    return;
  }
  const Bool bIsFirstCompOfChType = (getFirstComponentOfChannel(toChannelType(compIdx)) == compIdx);

  //type
  if(bIsFirstCompOfChType)
  {
    //sao_type_idx_luma or sao_type_idx_chroma
    if(ctbParam.modeIdc == SAO_MODE_OFF)
    {
      uiSymbol =0;
    }
    else if(ctbParam.typeIdc == SAO_TYPE_BO) //BO
    {
      uiSymbol = 1;
    }
    else
    {
      assert(ctbParam.typeIdc < SAO_TYPE_START_BO); //EO
      uiSymbol = 2;
    }
    codeSaoTypeIdx(uiSymbol);
  }

  if(ctbParam.modeIdc == SAO_MODE_NEW)
  {
    Int numClasses = (ctbParam.typeIdc == SAO_TYPE_BO)?4:NUM_SAO_EO_CLASSES;
    Int offset[4];
    Int k=0;
    for(Int i=0; i< numClasses; i++)
    {
      if(ctbParam.typeIdc != SAO_TYPE_BO && i == SAO_CLASS_EO_PLAIN)
      {
        continue;
      }
      Int classIdx = (ctbParam.typeIdc == SAO_TYPE_BO)?(  (ctbParam.typeAuxInfo+i)% NUM_SAO_BO_CLASSES   ):i;
      offset[k] = ctbParam.offset[classIdx];
      k++;
    }

    const Int  maxOffsetQVal = TComSampleAdaptiveOffset::getMaxOffsetQVal(channelBitDepth);
    for(Int i=0; i< 4; i++)
    {
      codeSaoMaxUvlc((offset[i]<0)?(-offset[i]):(offset[i]),  maxOffsetQVal ); //sao_offset_abs
    }


    if(ctbParam.typeIdc == SAO_TYPE_BO)
    {
      for(Int i=0; i< 4; i++)
      {
        if(offset[i] != 0)
        {
          codeSAOSign((offset[i]< 0)?1:0);
        }
      }

      codeSaoUflc(NUM_SAO_BO_CLASSES_LOG2, ctbParam.typeAuxInfo ); //sao_band_position
    }
    else //EO
    {
      if(bIsFirstCompOfChType)
      {
        assert(ctbParam.typeIdc - SAO_TYPE_START_EO >=0);
        codeSaoUflc(NUM_SAO_EO_TYPES_LOG2, ctbParam.typeIdc - SAO_TYPE_START_EO ); //sao_eo_class_luma or sao_eo_class_chroma
      }
    }

  }
}


Void TEncSbac::codeSAOBlkParam(SAOBlkParam& saoBlkParam, const BitDepths &bitDepths
                              , Bool* sliceEnabled
                              , Bool leftMergeAvail
                              , Bool aboveMergeAvail
                              , Bool onlyEstMergeInfo // = false
                              )
{

  Bool isLeftMerge = false;
  Bool isAboveMerge= false;

  if(leftMergeAvail)
  {
    isLeftMerge = ((saoBlkParam[COMPONENT_Y].modeIdc == SAO_MODE_MERGE) && (saoBlkParam[COMPONENT_Y].typeIdc == SAO_MERGE_LEFT));
    codeSaoMerge( isLeftMerge?1:0  ); //sao_merge_left_flag
  }

  if( aboveMergeAvail && !isLeftMerge)
  {
    isAboveMerge = ((saoBlkParam[COMPONENT_Y].modeIdc == SAO_MODE_MERGE) && (saoBlkParam[COMPONENT_Y].typeIdc == SAO_MERGE_ABOVE));
    codeSaoMerge( isAboveMerge?1:0  ); //sao_merge_left_flag
  }

  if(onlyEstMergeInfo)
  {
    return; //only for RDO
  }

  if(!isLeftMerge && !isAboveMerge) //not merge mode
  {
    for(Int compIdx=0; compIdx < MAX_NUM_COMPONENT; compIdx++)
    {
      codeSAOOffsetParam(ComponentID(compIdx), saoBlkParam[compIdx], sliceEnabled[compIdx], bitDepths.recon[toChannelType(ComponentID(compIdx))]);
    }
  }
}

/*!
 ****************************************************************************
 * \brief
 *   estimate bit cost for CBP, significant map and significant coefficients
 ****************************************************************************
 */
Void TEncSbac::estBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType )
{
  estCBFBit( pcEstBitsSbac );

  estSignificantCoeffGroupMapBit( pcEstBitsSbac, chType );

  // encode significance map
  estSignificantMapBit( pcEstBitsSbac, width, height, chType );

  // encode last significant position
  estLastSignificantPositionBit( pcEstBitsSbac, width, height, chType );

  // encode significant coefficients
  estSignificantCoefficientsBit( pcEstBitsSbac, chType );

  memcpy(pcEstBitsSbac->golombRiceAdaptationStatistics, m_golombRiceAdaptationStatistics, (sizeof(UInt) * RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS));
}

/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost for each CBP bit
 ****************************************************************************
 */
Void TEncSbac::estCBFBit( estBitsSbacStruct* pcEstBitsSbac )
{
  ContextModel *pCtx = m_cCUQtCbfSCModel.get( 0 );

  for( UInt uiCtxInc = 0; uiCtxInc < (NUM_QT_CBF_CTX_SETS * NUM_QT_CBF_CTX_PER_SET); uiCtxInc++ )
  {
    pcEstBitsSbac->blockCbpBits[ uiCtxInc ][ 0 ] = pCtx[ uiCtxInc ].getEntropyBits( 0 );
    pcEstBitsSbac->blockCbpBits[ uiCtxInc ][ 1 ] = pCtx[ uiCtxInc ].getEntropyBits( 1 );
  }

  pCtx = m_cCUQtRootCbfSCModel.get( 0 );

  for( UInt uiCtxInc = 0; uiCtxInc < 4; uiCtxInc++ )
  {
    pcEstBitsSbac->blockRootCbpBits[ uiCtxInc ][ 0 ] = pCtx[ uiCtxInc ].getEntropyBits( 0 );
    pcEstBitsSbac->blockRootCbpBits[ uiCtxInc ][ 1 ] = pCtx[ uiCtxInc ].getEntropyBits( 1 );
  }
}


/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient group map
 ****************************************************************************
 */
Void TEncSbac::estSignificantCoeffGroupMapBit( estBitsSbacStruct* pcEstBitsSbac, ChannelType chType )
{
  Int firstCtx = 0, numCtx = NUM_SIG_CG_FLAG_CTX;

  for ( Int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++ )
  {
    for( UInt uiBin = 0; uiBin < 2; uiBin++ )
    {
      pcEstBitsSbac->significantCoeffGroupBits[ ctxIdx ][ uiBin ] = m_cCUSigCoeffGroupSCModel.get(  0, chType, ctxIdx ).getEntropyBits( uiBin );
    }
  }
}


/*!
 ****************************************************************************
 * \brief
 *    estimate SAMBAC bit cost for significant coefficient map
 ****************************************************************************
 */
Void TEncSbac::estSignificantMapBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType )
{
  //--------------------------------------------------------------------------------------------------

  //set up the number of channels and context variables

  const UInt firstComponent = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));
  const UInt lastComponent  = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));

  //----------------------------------------------------------

  Int firstCtx = MAX_INT;
  Int numCtx   = MAX_INT;

  if      ((width == 4) && (height == 4))
  {
    firstCtx = significanceMapContextSetStart[chType][CONTEXT_TYPE_4x4];
    numCtx   = significanceMapContextSetSize [chType][CONTEXT_TYPE_4x4];
  }
  else if ((width == 8) && (height == 8))
  {
    firstCtx = significanceMapContextSetStart[chType][CONTEXT_TYPE_8x8];
    numCtx   = significanceMapContextSetSize [chType][CONTEXT_TYPE_8x8];
  }
  else
  {
    firstCtx = significanceMapContextSetStart[chType][CONTEXT_TYPE_NxN];
    numCtx   = significanceMapContextSetSize [chType][CONTEXT_TYPE_NxN];
  }

  //--------------------------------------------------------------------------------------------------

  //fill the data for the significace map

  for (UInt component = firstComponent; component <= lastComponent; component++)
  {
    const UInt contextOffset = getSignificanceMapContextOffset(ComponentID(component));

    if (firstCtx > 0)
    {
      for( UInt bin = 0; bin < 2; bin++ ) //always get the DC
      {
        pcEstBitsSbac->significantBits[ contextOffset ][ bin ] = m_cCUSigSCModel.get( 0, 0, contextOffset ).getEntropyBits( bin );
      }
    }

    // This could be made optional, but would require this function to have knowledge of whether the
    // TU is transform-skipped or transquant-bypassed and whether the SPS flag is set
    for( UInt bin = 0; bin < 2; bin++ )
    {
      const Int ctxIdx = significanceMapContextSetStart[chType][CONTEXT_TYPE_SINGLE];
      pcEstBitsSbac->significantBits[ contextOffset + ctxIdx ][ bin ] = m_cCUSigSCModel.get( 0, 0, (contextOffset + ctxIdx) ).getEntropyBits( bin );
    }

    for ( Int ctxIdx = firstCtx; ctxIdx < firstCtx + numCtx; ctxIdx++ )
    {
      for( UInt uiBin = 0; uiBin < 2; uiBin++ )
      {
        pcEstBitsSbac->significantBits[ contextOffset + ctxIdx ][ uiBin ] = m_cCUSigSCModel.get(  0, 0, (contextOffset + ctxIdx) ).getEntropyBits( uiBin );
      }
    }
  }

  //--------------------------------------------------------------------------------------------------
}


/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */

Void TEncSbac::estLastSignificantPositionBit( estBitsSbacStruct* pcEstBitsSbac, Int width, Int height, ChannelType chType )
{
  //--------------------------------------------------------------------------------------------------.

  //set up the number of channels

  const UInt firstComponent = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));
  const UInt lastComponent  = ((isLuma(chType)) ? (COMPONENT_Y) : (COMPONENT_Cb));

  //--------------------------------------------------------------------------------------------------

  //fill the data for the last-significant-coefficient position

  for (UInt componentIndex = firstComponent; componentIndex <= lastComponent; componentIndex++)
  {
    const ComponentID component = ComponentID(componentIndex);

    Int iBitsX = 0, iBitsY = 0;

    Int blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY;
    getLastSignificantContextParameters(ComponentID(component), width, height, blkSizeOffsetX, blkSizeOffsetY, shiftX, shiftY);

    Int ctx;

    const ChannelType channelType = toChannelType(ComponentID(component));

    ContextModel *const pCtxX = m_cCuCtxLastX.get( 0, channelType );
    ContextModel *const pCtxY = m_cCuCtxLastY.get( 0, channelType );
    Int          *const lastXBitsArray = pcEstBitsSbac->lastXBits[channelType];
    Int          *const lastYBitsArray = pcEstBitsSbac->lastYBits[channelType];

    //------------------------------------------------

    //X-coordinate

    for (ctx = 0; ctx < g_uiGroupIdx[ width - 1 ]; ctx++)
    {
      Int ctxOffset = blkSizeOffsetX + (ctx >>shiftX);
      lastXBitsArray[ ctx ] = iBitsX + pCtxX[ ctxOffset ].getEntropyBits( 0 );
      iBitsX += pCtxX[ ctxOffset ].getEntropyBits( 1 );
    }

    lastXBitsArray[ctx] = iBitsX;

    //------------------------------------------------

    //Y-coordinate

    for (ctx = 0; ctx < g_uiGroupIdx[ height - 1 ]; ctx++)
    {
      Int ctxOffset = blkSizeOffsetY + (ctx >>shiftY);
      lastYBitsArray[ ctx ] = iBitsY + pCtxY[ ctxOffset ].getEntropyBits( 0 );
      iBitsY += pCtxY[ ctxOffset ].getEntropyBits( 1 );
    }

    lastYBitsArray[ctx] = iBitsY;

  } //end of component loop

  //--------------------------------------------------------------------------------------------------
}


/*!
 ****************************************************************************
 * \brief
 *    estimate bit cost of significant coefficient
 ****************************************************************************
 */
Void TEncSbac::estSignificantCoefficientsBit( estBitsSbacStruct* pcEstBitsSbac, ChannelType chType )
{
  ContextModel *ctxOne = m_cCUOneSCModel.get(0, 0);
  ContextModel *ctxAbs = m_cCUAbsSCModel.get(0, 0);

  const UInt oneStartIndex = ((isLuma(chType)) ? (0)                     : (NUM_ONE_FLAG_CTX_LUMA));
  const UInt oneStopIndex  = ((isLuma(chType)) ? (NUM_ONE_FLAG_CTX_LUMA) : (NUM_ONE_FLAG_CTX));
  const UInt absStartIndex = ((isLuma(chType)) ? (0)                     : (NUM_ABS_FLAG_CTX_LUMA));
  const UInt absStopIndex  = ((isLuma(chType)) ? (NUM_ABS_FLAG_CTX_LUMA) : (NUM_ABS_FLAG_CTX));

  for (Int ctxIdx = oneStartIndex; ctxIdx < oneStopIndex; ctxIdx++)
  {
    pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 0 ] = ctxOne[ ctxIdx ].getEntropyBits( 0 );
    pcEstBitsSbac->m_greaterOneBits[ ctxIdx ][ 1 ] = ctxOne[ ctxIdx ].getEntropyBits( 1 );
  }

  for (Int ctxIdx = absStartIndex; ctxIdx < absStopIndex; ctxIdx++)
  {
    pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 0 ] = ctxAbs[ ctxIdx ].getEntropyBits( 0 );
    pcEstBitsSbac->m_levelAbsBits[ ctxIdx ][ 1 ] = ctxAbs[ ctxIdx ].getEntropyBits( 1 );
  }
}

/**
 - Initialize our context information from the nominated source.
 .
 \param pSrc From where to copy context information.
 */
Void TEncSbac::xCopyContextsFrom( const TEncSbac* pSrc )
{
  memcpy(m_contextModels, pSrc->m_contextModels, m_numContextModels*sizeof(m_contextModels[0]));
  memcpy(m_golombRiceAdaptationStatistics, pSrc->m_golombRiceAdaptationStatistics, (sizeof(UInt) * RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS));
}

Void  TEncSbac::loadContexts ( const TEncSbac* pSrc)
{
  xCopyContextsFrom(pSrc);
}

/** Performs CABAC encoding of the explicit RDPCM mode
 * \param rTu current TU data structure
 * \param compID component identifier
 */
Void TEncSbac::codeExplicitRdpcmMode( TComTU &rTu, const ComponentID compID )
{
  TComDataCU *cu = rTu.getCU();
  const TComRectangle &rect = rTu.getRect(compID);
  const UInt absPartIdx   = rTu.GetAbsPartIdxTU(compID);
  const UInt tuHeight = g_aucConvertToBit[rect.height];
  const UInt tuWidth  = g_aucConvertToBit[rect.width];

  assert(tuHeight == tuWidth);
  assert(tuHeight < 4);

  UInt explicitRdpcmMode = cu->getExplicitRdpcmMode(compID, absPartIdx);

  if( explicitRdpcmMode == RDPCM_OFF )
  {
    m_pcBinIf->encodeBin (0, m_explicitRdpcmFlagSCModel.get (0, toChannelType(compID), 0));
  }
  else if( explicitRdpcmMode == RDPCM_HOR || explicitRdpcmMode == RDPCM_VER )
  {
    m_pcBinIf->encodeBin (1, m_explicitRdpcmFlagSCModel.get (0, toChannelType(compID), 0));
    if(explicitRdpcmMode == RDPCM_HOR)
    {
      m_pcBinIf->encodeBin ( 0, m_explicitRdpcmDirSCModel.get(0, toChannelType(compID), 0));
    }
    else
    {
      m_pcBinIf->encodeBin ( 1, m_explicitRdpcmDirSCModel.get(0, toChannelType(compID), 0));
    }
  }
  else
  {
    assert(0);
  }
}

UInt TEncSbac::xWriteTruncMsbP1( UInt uiSymbol, ContextModel* pcSCModel, UInt uiMax, UInt uiCtxT, UChar *ucCtxLut)
{
  if ( uiMax == 0 )
    return 0;

  UInt uiMsbP1;
  for (uiMsbP1 = 0; uiSymbol > 0; uiMsbP1++)
  {
    uiSymbol >>= 1;
    if ( uiMsbP1 > uiCtxT )
    {
      m_pcBinIf->encodeBinEP(1);
    }
    else
    m_pcBinIf->encodeBin(1, uiMsbP1 <= uiCtxT? pcSCModel[ucCtxLut[uiMsbP1]] : pcSCModel[ucCtxLut[uiCtxT]]);
  }
  assert ( uiMsbP1 <= uiMax );
  if ( uiMsbP1 < uiMax )
  {
    if ( uiMsbP1 > uiCtxT )
    {
      m_pcBinIf->encodeBinEP(0);
    }
    else
    m_pcBinIf->encodeBin(0, uiMsbP1 <= uiCtxT? pcSCModel[ucCtxLut[uiMsbP1]] : pcSCModel[ucCtxLut[uiCtxT]]);
  }
  return uiMsbP1;
}

Void TEncSbac::xWriteTruncMsbP1RefinementBits ( UInt uiSymbol, ContextModel* pcSCModel, UInt uiMax, UInt uiCtxT, UChar *ucCtxLut )
{
  if (uiMax==0)
    return;

  UInt uiMsbP1 = xWriteTruncMsbP1( uiSymbol, pcSCModel, g_getMsbP1Idx(uiMax), uiCtxT, ucCtxLut );
  if ( uiMsbP1 > 1)
  {
    UInt uiNumBins = g_getMsbP1Idx(uiMax);
    if ( uiMsbP1 < uiNumBins)
    {

      UInt uiBits = uiMsbP1-1;
      m_pcBinIf->encodeBinsEP( uiSymbol & ((1 << uiBits) - 1), uiBits );
    }
    else
    {
      UInt curValue = 1 << (uiNumBins-1);
      xWriteTruncBinCode(uiSymbol-curValue, uiMax+1-curValue);
    }
  }
}



//! \}
